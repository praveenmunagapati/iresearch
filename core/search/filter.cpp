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

#include "filter.hpp"

NS_LOCAL

//////////////////////////////////////////////////////////////////////////////
/// @class emtpy_query
/// @brief represent a query returns empty result set 
//////////////////////////////////////////////////////////////////////////////
class empty_query final : public irs::filter::prepared {
 public:
  virtual irs::doc_iterator::ptr execute(
      const irs::sub_reader&,
      const irs::order::prepared&) const override {
    return irs::doc_iterator::empty();
  }
}; // empty_query

NS_END // LOCAL

NS_ROOT

// -----------------------------------------------------------------------------
// --SECTION--                                                            filter
// -----------------------------------------------------------------------------

filter::filter(const type_id& type) NOEXCEPT
  : boost_(boost::no_boost()), type_(&type) {
}

filter::~filter() {}

filter::prepared::ptr filter::prepared::empty() {
  return filter::prepared::make<empty_query>();
}

filter::prepared::prepared(attribute_store&& attrs)
  : attrs_(std::move(attrs)) {
}

filter::prepared::~prepared() {}

// -----------------------------------------------------------------------------
// --SECTION--                                                             empty
// -----------------------------------------------------------------------------

DEFINE_FILTER_TYPE(irs::empty);
DEFINE_FACTORY_DEFAULT(irs::empty);

empty::empty(): filter(empty::type()) {
}

filter::prepared::ptr empty::prepare(
    const index_reader&,
    const order::prepared&,
    boost_t
) const {
  static filter::prepared::ptr instance = std::make_shared<empty_query>();
  return instance;
}

NS_END // ROOT

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------
