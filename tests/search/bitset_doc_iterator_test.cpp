////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017 ArangoDB GmbH, Cologne, Germany
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
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Andrey Abramov
/// @author Vasiliy Nabatchikov
////////////////////////////////////////////////////////////////////////////////

#include "tests_shared.hpp"
#include "utils/bitset.hpp"
#include "search/bitset_doc_iterator.hpp"

#ifndef IRESEARCH_DLL

TEST(bitset_iterator_test, next) {
  {
    // empty bitset
    irs::bitset bs;
    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(0, cost->estimate());

    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // non-empty bitset
  {
    irs::bitset bs(13);
    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(0, cost->estimate());

    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // dense bitset
  {
    const size_t size = 73;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_FALSE(irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size, cost->estimate());

    for (auto i = 0; i < size; ++i) {
      ASSERT_TRUE(it.next());
      ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::min() + i, it.value());
    }
    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

  }

  // sparse bitset
  {
    const size_t size = 176;
    irs::bitset bs(size);

    // set every second bit
    for (auto i = 0; i < size; ++i) {
      bs.reset(i, i%2);
    }

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size/2, cost->estimate());

    for (auto i = 1; i < size; i+=2) {
      ASSERT_TRUE(it.next());
      ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::min() + i, it.value());
    }
    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }
}

TEST(bitset_iterator_test, seek) {
  {
    // empty bitset
    irs::bitset bs;
    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(0, cost->estimate());

    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.seek(1)));
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // non-empty bitset
  {
    irs::bitset bs(13);
    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(0, cost->estimate());

    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));

    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.seek(1)));
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // dense bitset
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_FALSE(irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size, cost->estimate());

    for (auto expected_doc = irs::type_limits<irs::type_t::doc_id_t>::min(); expected_doc <= size; ++expected_doc) {
      ASSERT_EQ(expected_doc, it.seek(expected_doc));
      ASSERT_EQ(expected_doc, it.value());
    }
    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // dense bitset, seek backwards
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_FALSE(irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size, cost->estimate());

    for (ptrdiff_t expected_doc = size; expected_doc >= irs::type_limits<irs::type_t::doc_id_t>::min(); --expected_doc) {
      ASSERT_EQ(expected_doc, it.seek(expected_doc));
      ASSERT_EQ(expected_doc, it.value());
    }
    ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::min(), it.value());
    ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::min(), it.seek(irs::type_limits<irs::type_t::doc_id_t>::invalid()));
  }

  // seek after the last document
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::eof(), it.seek(size+1));
  }

  // seek to the last document
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    ASSERT_EQ(size, it.seek(size));
  }

  // seek to 'eof'
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::eof(), it.seek(irs::type_limits<irs::type_t::doc_id_t>::eof()));
  }

  // seek before the first document
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::min(), it.seek(irs::type_limits<irs::type_t::doc_id_t>::invalid()));
  }

  // sparse bitset
  {
    const size_t size = 176;
    irs::bitset bs(size);

    // set every second bit
    for (auto i = 0; i < size; ++i) {
      bs.reset(i, i%2);
    }

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size/2, cost->estimate());

    for (auto i = 1; i < size; i+=2) {
      const auto expected_doc = irs::type_limits<irs::type_t::doc_id_t>::min() + i;
      ASSERT_EQ(expected_doc, it.seek(expected_doc-1));
      ASSERT_EQ(expected_doc, it.value());
      ASSERT_EQ(expected_doc, it.seek(expected_doc));
      ASSERT_EQ(expected_doc, it.value());
    }
    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // sparse bitset, seek backwards
  {
    const size_t size = 176;
    irs::bitset bs(size);

    // set every second bit
    for (auto i = 0; i < size; ++i) {
      bs.reset(i, i%2);
    }

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size/2, cost->estimate());

    for (ptrdiff_t i = size; i > 0; i-=2) {
      ASSERT_EQ(i, it.seek(i));
      ASSERT_EQ(i, it.seek(i-1));
      ASSERT_EQ(i, it.value());
    }
  }
}

TEST(bitset_iterator_test, seek_next) {
  // dense bitset
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_FALSE(irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size, cost->estimate());

    const size_t steps = 5;
    for (auto expected_doc = irs::type_limits<irs::type_t::doc_id_t>::min(); expected_doc <= size; ++expected_doc) {
      ASSERT_EQ(expected_doc, it.seek(expected_doc));
      ASSERT_EQ(expected_doc, it.value());

      for (auto j = 1; j <= steps && it.next(); ++j) {
        ASSERT_EQ(expected_doc + j, it.value());
      }
    }
    ASSERT_FALSE(it.next());
    ASSERT_TRUE(irs::type_limits<irs::type_t::doc_id_t>::eof(it.value()));
  }

  // dense bitset, seek backwards
  {
    const size_t size = 173;
    irs::bitset bs(size);

    // set all bits
    irs::bitset::word_t data[] {
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0),
      ~irs::bitset::word_t(0)
    };

    bs.memset(data);

    irs::bitset_doc_iterator it(bs);
    ASSERT_FALSE(irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size, cost->estimate());

    const size_t steps = 5;
    for (ptrdiff_t expected_doc = size; expected_doc >= irs::type_limits<irs::type_t::doc_id_t>::min(); --expected_doc) {
      ASSERT_EQ(expected_doc, it.seek(expected_doc));
      ASSERT_EQ(expected_doc, it.value());

      for (auto j = 1; j <= steps && it.next(); ++j) {
        ASSERT_EQ(expected_doc + j, it.value());
      }
    }
    ASSERT_EQ(irs::type_limits<irs::type_t::doc_id_t>::min(), it.seek(irs::type_limits<irs::type_t::doc_id_t>::invalid()));
  }

  // sparse bitset, seek+next
  {
    const size_t size = 176;
    irs::bitset bs(size);

    // set every second bit
    for (auto i = 0; i < size; ++i) {
      bs.reset(i, i%2);
    }

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size/2, cost->estimate());

    size_t steps = 5;
    for (size_t i = irs::type_limits<irs::type_t::doc_id_t>::min(); i < size; i+=2) {
      const auto expected_doc = irs::type_limits<irs::type_t::doc_id_t>::min() + i;
      ASSERT_EQ(expected_doc, it.seek(i));
      ASSERT_EQ(expected_doc, it.value());

      for (auto j = 1; j <= steps && it.next(); ++j) {
        ASSERT_EQ(expected_doc + 2*j, it.value());
      }
    }
  }

  // sparse bitset, seek backwards+next
  {
    const size_t size = 176;
    irs::bitset bs(size);

    // set every second bit
    for (auto i = 0; i < size; ++i) {
      bs.reset(i, i%2);
    }

    irs::bitset_doc_iterator it(bs);
    ASSERT_TRUE(!irs::type_limits<irs::type_t::doc_id_t>::valid(it.value()));

    auto& attrs = it.attributes();
    auto& cost = attrs.get<irs::cost>();
    ASSERT_TRUE(bool(cost));
    ASSERT_EQ(size/2, cost->estimate());

    size_t steps = 5;
    for (ptrdiff_t i = size; i > 0; i-=2) {
      ASSERT_EQ(i, it.seek(i));
      ASSERT_EQ(i, it.value());

      for (auto j = 1; j <= steps && it.next(); ++j) {
        ASSERT_EQ(i + 2*j, it.value());
      }
    }
  }
}

#endif