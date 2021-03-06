cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

set (IResearch_TARGET_NAME 
  "iresearch"
  CACHE INTERNAL
  ""
)

project(${IResearch_TARGET_NAME})

# attach additional cmake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if (NOT MSVC)
  # put GCC version into GCC_VERSION variable
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
endif()

################################################################################
### compile options
################################################################################

option(USE_VALGRIND "Use workarounds to avoid false positives in valgrind" OFF)

if (USE_VALGRIND)
  add_definitions(-DIRESEARCH_VALGRIND)
endif()

################################################################################
### find 3rd party libraries
################################################################################

# find Boost
find_package(BoostLocal
  REQUIRED
)

# set pthread library
if (NOT MSVC)
  set(PTHREAD_LIBRARY pthread)
endif()

# set gcov library
if (NOT MSVC AND (CMAKE_BUILD_TYPE STREQUAL "Coverage"))
  set(GCOV_LIBRARY gcov)
endif()

#find BFD
find_package(BFD
  #OPTIONAL
)

if (BFD_FOUND)
  add_definitions(-DUSE_LIBBFD)
else()
  set(BFD_INCLUDE_DIR "")
  set(BFD_SHARED_LIBS "")
  set(BFD_STATIC_LIBS "")
  set(BFD_SHARED_LIB_RESOURCES "")
endif()

# find LZ4
find_package(Lz4
  REQUIRED
)

# find ICU
find_package(ICU
  REQUIRED
)

# find Snowball
find_package(Snowball
  REQUIRED
)

# find Unwind
find_package(Unwind
  #OPTIONAL
)

if (Unwind_FOUND)
  add_definitions(-DUSE_LIBUNWIND)
else()
  set(Unwind_INCLUDE_DIR "")
  set(Unwind_SHARED_LIBS "")
  set(Unwind_STATIC_LIBS "")
  set(Unwind_SHARED_LIB_RESOURCES "")
endif()

# set external dirs
set(EXTERNAL_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/external
  CACHE INTERNAL 
  ""
)

set(IResearch_INCLUDE_DIR 
  "${PROJECT_SOURCE_DIR}/core"
  CACHE INTERNAL
  ""
)

# set output directories
set(EXECUTABLE_OUTPUT_PATH
  ${CMAKE_BINARY_DIR}/bin
  CACHE PATH
  "Executable output path"
)

set(LIBRARY_OUTPUT_PATH
  ${CMAKE_BINARY_DIR}/bin
  CACHE PATH 
  "Library output path"
)

mark_as_advanced( 
  EXECUTABLE_OUTPUT_PATH 
  LIBRARY_OUTPUT_PATH
)

add_definitions(-DUNICODE -D_UNICODE)

add_subdirectory(core)
add_subdirectory(external)
add_subdirectory(tests)
add_subdirectory(utils)

# setup code coverage
if (NOT MSVC AND (CMAKE_BUILD_TYPE STREQUAL "Coverage"))
  include(CodeCoverage)
  
  # exclude directories from coverage report
  SET(LCOV_EXCLUDE "external/*;boost/*")

  setup_target_for_coverage(
    ${IResearch_TARGET_NAME}-coverage 
    $<TARGET_FILE:${IResearchTests_TARGET_NAME}-static>
    coverage
    "--ires_output"
  )
  add_dependencies(${IResearch_TARGET_NAME}-coverage
    ${IResearchTests_TARGET_NAME}
  )

  if (PYTHON_EXECUTABLE AND GCOVR_PATH)   
    # exclude directories from coverage report
    SET(GCOVR_EXTRA_ARGS "-e;${PROJECT_SOURCE_DIR}/external/")

    setup_target_for_coverage_cobertura(
      ${IResearch_TARGET_NAME}-coverage-cobertura
      $<TARGET_FILE:${IResearchTests_TARGET_NAME}-static>
      coverage
      "--ires_output"
    )
    add_dependencies(${IResearch_TARGET_NAME}-coverage-cobertura
      ${IResearchTests_TARGET_NAME}
    )
  endif()
endif()

# testing
enable_testing()
add_test(
  iresearch-tests
  ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX}
)

# testing auto build not working
# due to the following bug in cmake
# http://public.kitware.com/Bug/view.php?id=8774
# here is the workaround:
add_custom_target(iresearch-check
  COMMAND ${CMAKE_CTEST_COMMAND} 
  DEPENDS iresearch-tests
)

# setup target for memory allocation profiling
add_custom_target(iresearch-tests-malloc
  COMMAND valgrind --tool=massif --depth=100 --max-snapshots=500 --time-unit=ms --detailed-freq=1 --massif-out-file=massif.out ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --ires_output
  COMMAND ms_print --x=500 --y=100 massif.out > massif.log
  DEPENDS iresearch-tests
)

# setup target for memory allocation profiling
add_custom_target(iresearch-tests-malloc-s
  COMMAND valgrind --tool=massif --depth=100 --max-snapshots=500 --time-unit=ms --detailed-freq=1 --massif-out-file=massif.out ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --ires_output
  COMMAND ms_print --x=500 --y=100 massif.out > massif.log
  DEPENDS iresearch-tests
)

# setup target for memory allocation profiling
add_custom_target(iresearch-tests-malloc-fast
  COMMAND valgrind --tool=massif --depth=100 --max-snapshots=500 --time-unit=ms --detailed-freq=1 --massif-out-file=massif.out ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*:-:*europ*:-:*eco*:-:*postings_seek*:-:*profile* --ires_output
  COMMAND ms_print --x=500 --y=100 massif.out > massif.log
  DEPENDS iresearch-tests
)

# setup target for memory allocation profiling
add_custom_target(iresearch-tests-malloc-s-fast
  COMMAND valgrind --tool=massif --depth=100 --max-snapshots=500 --time-unit=ms --detailed-freq=1 --massif-out-file=massif.out ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*:-:*europ*:-:*eco*:-:*postings_seek*:-:*profile* --ires_output
  COMMAND ms_print --x=500 --y=100 massif.out > massif.log
  DEPENDS iresearch-tests
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --track-origins=yes --read-var-info=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --ires_output
  DEPENDS iresearch-tests
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-s
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --track-origins=yes --read-var-info=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --ires_output
  DEPENDS iresearch-tests-static
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-fast
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --track-origins=yes --read-var-info=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*:-:*europ*:-:*eco*:-:*postings_seek*:-:*profile* --ires_output
  DEPENDS iresearch-tests
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-s-fast
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --track-origins=yes --read-var-info=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*:-:*europ*:-:*eco*:-:*postings_seek*:-:*profile* --ires_output
  DEPENDS iresearch-tests-static
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-shallow
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --ires_output
  DEPENDS iresearch-tests
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-s-shallow
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --ires_output
  DEPENDS iresearch-tests-static
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-shallow-fast
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*:-:*europ*:-:*eco*:-:*postings_seek*:-:*profile* --ires_output
  DEPENDS iresearch-tests
)

# setup target for memory leak detection
add_custom_target(iresearch-tests-memleak-s-shallow-fast
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --leak-check=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*:-:*europ*:-:*eco*:-:*postings_seek*:-:*profile* --ires_output
  DEPENDS iresearch-tests-static
)

# setup target for thread race detection
add_custom_target(iresearch-tests-threadrace
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --tool=helgrind --read-var-info=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*_mt --ires_output
  DEPENDS iresearch-tests
)

# setup target for thread race detection
add_custom_target(iresearch-tests-threadrace-s
  COMMAND valgrind --xml=yes --xml-file=valgrind.xml --tool=helgrind --read-var-info=yes --num-callers=64 ${EXECUTABLE_OUTPUT_PATH}/iresearch-tests-s${CMAKE_EXECUTABLE_SUFFIX} --gtest_filter=*_mt --ires_output
  DEPENDS iresearch-tests-static
)
