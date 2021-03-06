////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 by EMC Corporation, All Rights Reserved
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is EMC Corporation
///
/// @author Andrey Abramov
/// @author Vasiliy Nabatchikov
////////////////////////////////////////////////////////////////////////////////

#if !defined(_MSC_VER)
  #include <dlfcn.h> // for RTLD_NEXT
#endif

#include <signal.h> // for signal(...)/raise(...)

#include "tests_shared.hpp"
#include "tests_config.hpp"

#include <boost/filesystem.hpp>

#include <ctime>
#include <formats/formats.hpp>

#include <utils/attributes.hpp>

#include "index/doc_generator.hpp"
#include "utils/log.hpp"
#include "utils/network_utils.hpp"
#include "utils/bitset.hpp"
#include "utils/runtime_utils.hpp"

#include <cmdline.h>

#ifdef _MSC_VER
  // +1 for \0 at end of string
  #define mkdtemp(templ) 0 == _mkdir(0 == _mktemp_s(templ, strlen(templ) + 1) ? templ : nullptr) ? templ : nullptr
#endif

/* -------------------------------------------------------------------
* iteration_tracker
* ------------------------------------------------------------------*/

struct iteration_tracker : ::testing::Environment {
  virtual void SetUp() {
    ++iteration;
  }

  static uint32_t iteration;
};

uint32_t iteration_tracker::iteration = (std::numeric_limits<uint32_t>::max)();

/* -------------------------------------------------------------------
* test_base
* ------------------------------------------------------------------*/

namespace {
  namespace fs = boost::filesystem;
}

const std::string IRES_HELP("help");
const std::string IRES_LOG_LEVEL("ires_log_level");
const std::string IRES_LOG_STACK("ires_log_stack");
const std::string IRES_OUTPUT("ires_output");
const std::string IRES_OUTPUT_PATH("ires_output_path");
const std::string IRES_RESOURCE_DIR("ires_resource_dir");

const std::string test_base::test_results("test_detail.xml");

fs::path test_base::exec_path_;
fs::path test_base::exec_dir_;
fs::path test_base::exec_file_;
fs::path test_base::out_dir_;
fs::path test_base::resource_dir_;
fs::path test_base::res_dir_;
fs::path test_base::res_path_;
std::string test_base::test_name_;
int test_base::argc_;
char** test_base::argv_;
decltype(test_base::argv_ires_output_) test_base::argv_ires_output_;

std::string test_base::temp_file() {
  return fs::unique_path().string();
}

uint32_t test_base::iteration() {
  return iteration_tracker::iteration;
}

std::string test_base::resource( const std::string& name ) {
  return fs::path( resource_dir_ ).append( name ).string();
}

void test_base::SetUp() {
  namespace tst = ::testing;
  const tst::TestInfo* ti = tst::UnitTest::GetInstance()->current_test_info();

  fs::path iter_dir(res_dir_);
  if (::testing::FLAGS_gtest_repeat > 1 || ::testing::FLAGS_gtest_repeat < 0) {
    iter_dir.append(
      std::string("iteration ").append(std::to_string(iteration()))
    );
  }

  (test_case_dir_ = iter_dir).append(ti->test_case_name());
  (test_dir_ = test_case_dir_).append(ti->name());
  fs::create_directories(test_dir_);
}

void test_base::prepare(const cmdline::parser& parser) {
  if (parser.exist(IRES_HELP)) {
    return;
  }

  make_directories();

  {
    auto log_level = parser.get<irs::logger::level_t>(IRES_LOG_LEVEL);

    irs::logger::output_le(log_level, stderr); // set desired log level

    if (parser.get<bool>(IRES_LOG_STACK)) {
      irs::logger::stack_trace_level(log_level); // force enable stack trace
    }
  }

  if (parser.exist(IRES_OUTPUT)) {
    std::unique_ptr<char*[]> argv(new char*[2 + argc_]);
    std::memcpy(argv.get(), argv_, sizeof(char*)*(argc_));    
    argv_ires_output_.append("--gtest_output=xml:").append(res_path_.string());
    argv[argc_++] = &argv_ires_output_[0];

    /* let last argv equals to nullptr */
    argv[argc_] = nullptr;
    argv_ = argv.release();
  }
}

void test_base::make_directories() {
  exec_path_ = fs::path( argv_[0] );
  exec_file_ = exec_path_.filename();
  exec_dir_ = exec_path_.parent_path();
  test_name_ = exec_file_.replace_extension().string();

  if (out_dir_.empty()) {
    out_dir_ = exec_dir_;
  }

  std::cout << "launching: " << exec_path_.string() << std::endl;
  std::cout << "options:" << std::endl;
  std::cout << "\t" << IRES_OUTPUT_PATH << ": " << out_dir_.string() << std::endl;
  std::cout << "\t" << IRES_RESOURCE_DIR << ": " << resource_dir_.string() << std::endl;

  out_dir_ = ::boost::filesystem::canonical(out_dir_).make_preferred();
  (res_dir_ = out_dir_).append( test_name_ );  

  // add timestamp to res_dir_
  {
    std::tm tinfo;

    if (iresearch::localtime(tinfo, std::time(nullptr))) {
      char buf[21]{};

      strftime(buf, sizeof buf, "_%Y_%m_%d_%H_%M_%S", &tinfo);
      res_dir_.concat(buf, buf + sizeof buf - 1);
    } else {
      res_dir_.concat("_unknown");
    }
  }

  // add temporary string to res_dir_
  {
    char templ[] = "_XXXXXX";

    res_dir_.concat(templ, templ + sizeof templ - 1);
  }

  auto res_dir_templ = res_dir_.string();

  res_dir_ = mkdtemp(&(res_dir_templ[0]));
  (res_path_ = res_dir_).append(test_results);
}

void test_base::parse_command_line(cmdline::parser& cmd) {
  static const auto log_level_reader = [](const std::string &s)->irs::logger::level_t {
    static const std::map<std::string, irs::logger::level_t> log_levels = {
      { "FATAL", irs::logger::level_t::IRL_FATAL },
      { "ERROR", irs::logger::level_t::IRL_ERROR },
      { "WARN", irs::logger::level_t::IRL_WARN },
      { "INFO", irs::logger::level_t::IRL_INFO },
      { "DEBUG", irs::logger::level_t::IRL_DEBUG },
      { "TRACE", irs::logger::level_t::IRL_TRACE },
    };
    auto itr = log_levels.find(s);

    if (itr == log_levels.end()) {
      throw std::invalid_argument(s);
    }

    return itr->second;
  };

  cmd.add(IRES_HELP, '?', "print this message");
  cmd.add(IRES_LOG_LEVEL, 0, "threshold log level <FATAL|ERROR|WARN|INFO|DEBUG|TRACE>", false, irs::logger::level_t::IRL_FATAL, log_level_reader);
  cmd.add(IRES_LOG_STACK, 0, "always log stack trace", false, false);
  cmd.add(IRES_OUTPUT, 0, "generate an XML report");
  cmd.add(IRES_OUTPUT_PATH, 0, "output directory", false, out_dir_);
  cmd.add(IRES_RESOURCE_DIR, 0, "resource directory", false, fs::path(IResearch_test_resource_dir));

  if (!cmd.parse(argc_, argv_)) {
    std::cout << cmd.error_full() << std::endl;
    std::cout << cmd.usage() << std::endl;
    exit(1);
  }

  if (cmd.exist(IRES_HELP)) {
    std::cout << cmd.usage() << std::endl;
    return;
  }

  resource_dir_ = cmd.get<fs::path>(IRES_RESOURCE_DIR);
  out_dir_ = cmd.get<fs::path>(IRES_OUTPUT_PATH);
}

int test_base::initialize(int argc, char* argv[]) {
  argc_ = argc;
  argv_ = argv;
  ::testing::AddGlobalTestEnvironment(new iteration_tracker());
  ::testing::InitGoogleTest(&argc_, argv_);

  cmdline::parser cmd;

  parse_command_line(cmd);
  prepare(cmd);

  return RUN_ALL_TESTS();
}

void flush_timers(std::ostream &out) {
  std::map<std::string, std::pair<size_t, size_t>> ordered_stats;

  iresearch::timer_utils::visit([&ordered_stats](const std::string& key, size_t count, size_t time)->bool {
    std::string key_str = key;

#if defined(__GNUC__)
    if (key_str.compare(0, strlen("virtual "), "virtual ") == 0) {
      key_str = key_str.substr(strlen("virtual "));
    }

    size_t i;

    if (std::string::npos != (i = key_str.find(' ')) && key_str.find('(') > i) {
      key_str = key_str.substr(i + 1);
    }
#elif defined(_MSC_VER)
    size_t i;

    if (std::string::npos != (i = key_str.find("__cdecl "))) {
      key_str = key_str.substr(i + strlen("__cdecl "));
    }
#endif

    ordered_stats.emplace(key_str, std::make_pair(count, time));
    return true;
  });

  for (auto& entry: ordered_stats) {
    auto& key = entry.first;
    auto& count = entry.second.first;
    auto& time = entry.second.second;
    out << key << "\tcalls:" << count << ",\ttime: " << time/1000 << " us,\tavg call: " << time/1000/(double)count << " us"<< std::endl;
  }
}

void stack_trace_handler(int sig) {
  // reset to default handler
  signal(sig, SIG_DFL);
  // print stack trace
  iresearch::logger::stack_trace(iresearch::logger::IRL_FATAL); // IRL_FATAL is logged to stderr above
  // re-signal to default handler (so we still get core dump if needed...)
  raise(sig);
}

void install_stack_trace_handler() {
  signal(SIGILL, stack_trace_handler);
  signal(SIGSEGV, stack_trace_handler);
  signal(SIGABRT, stack_trace_handler);

  #ifndef _MSC_VER
    signal(SIGBUS, stack_trace_handler);
  #endif
}

#ifndef _MSC_VER
  // override GCC 'throw' handler to print stack trace before throw
  extern "C" {
    #if defined(__APPLE__)
      void __cxa_throw(void* ex, struct std::type_info* info, void(*dest)(void *)) {
        static void(*rethrow)(void*,struct std::type_info*,void(*)(void*)) =
          (void(*)(void*,struct std::type_info*,void(*)(void*)))dlsym(RTLD_NEXT, "__cxa_throw");

        IR_FRMT_DEBUG("exception type: %s", reinterpret_cast<const std::type_info*>(info)->name());
        IR_STACK_TRACE();
        rethrow(ex, info, dest);
      }
    #else
      void __cxa_throw(void* ex, void* info, void(*dest)(void*)) {
        static void(*rethrow)(void*,void*,void(*)(void*)) __attribute__ ((noreturn)) =
          (void(*)(void*,void*,void(*)(void*)))dlsym(RTLD_NEXT, "__cxa_throw");

        IR_FRMT_DEBUG("exception type: %s", reinterpret_cast<const std::type_info*>(info)->name());
        IR_STACK_TRACE();
        rethrow(ex, info, dest);
      }
    #endif
  }
#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                              main
// -----------------------------------------------------------------------------

int main( int argc, char* argv[] ) {
  install_stack_trace_handler();

  const int code = test_base::initialize( argc, argv );

  std::cout << "Path to test result directory: " 
            << test_base::test_results_dir() 
            << std::endl;

  return code;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------