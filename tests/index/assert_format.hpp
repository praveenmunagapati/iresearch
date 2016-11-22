//
// IResearch search engine 
// 
// Copyright � 2016 by EMC Corporation, All Rights Reserved
// 
// This software contains the intellectual property of EMC Corporation or is licensed to
// EMC Corporation from third parties. Use of this software and the intellectual property
// contained therein is expressly limited to the terms and conditions of the License
// Agreement under which it is provided by or on behalf of EMC.
// 

#ifndef IRESEARCH_ASSERT_FORMAT_H
#define IRESEARCH_ASSERT_FORMAT_H

#include "doc_generator.hpp"
#include "index/field_meta.hpp"
#include "utils/block_pool.hpp"
#include "formats/formats.hpp"

namespace tests {

/* -------------------------------------------------------------------
* FREQUENCY BASED DATA MODEL
* ------------------------------------------------------------------*/

/* -------------------------------------------------------------------
* position
* ------------------------------------------------------------------*/

struct position {
  position( uint32_t pos, uint32_t start,
            uint32_t end, const iresearch::bytes_ref& pay );

  bool operator<( const position& rhs ) const { return pos < rhs.pos; }

  uint32_t pos;
  uint32_t start;
  uint32_t end;
  iresearch::bstring payload;
};

/* -------------------------------------------------------------------
* posting
* ------------------------------------------------------------------*/

class posting {
 public:
  posting(iresearch::doc_id_t id);

  void add(uint32_t pos, uint32_t offs_start, const iresearch::attributes& attrs);

  const std::set<position>& positions() const { return positions_; }
  iresearch::doc_id_t id() const { return id_; }
  size_t size() const { return positions_.size(); }

  bool operator<( const posting& rhs ) const { return id_ < rhs.id_; }

  private:
  std::set<position> positions_;
  iresearch::doc_id_t id_;
};

/* -------------------------------------------------------------------
* term
* ------------------------------------------------------------------*/

struct term {
  term(const iresearch::bytes_ref& data);

  posting& add(iresearch::doc_id_t id);

  bool operator<( const term& rhs ) const;

  uint64_t docs_count() const { return postings.size(); }

  std::set<posting> postings;
  iresearch::bstring value;
};

/* -------------------------------------------------------------------
* field
* ------------------------------------------------------------------*/

class field : public iresearch::field_meta {
 public:
  field(
    const iresearch::string_ref& name,
    const iresearch::flags& features,
    iresearch::field_id id
  );

  field( field&& rhs );

  field& operator=( field&& rhs );

  term& add(const iresearch::bytes_ref& term);
  term* find(const iresearch::bytes_ref& term);
  size_t remove(const iresearch::bytes_ref& t);

  std::set<term> terms;
  std::unordered_set<iresearch::doc_id_t> docs;
  uint32_t pos;
  uint32_t offs;
};

/* -------------------------------------------------------------------
* index_segment
* ------------------------------------------------------------------*/

class index_segment: iresearch::util::noncopyable {
 public:
  typedef std::map<iresearch::string_ref, field> field_map_t;
  typedef field_map_t::const_iterator iterator;

  index_segment();
  index_segment( index_segment&& rhs );
  index_segment& operator=( index_segment&& rhs );
   
  size_t doc_count() const { return count_; }
  size_t size() const { return fields_.size(); }

  const iresearch::document_mask& doc_mask() const { return doc_mask_; }
  const field_map_t& fields() const { return fields_; }

  bool find(const iresearch::string_ref& name, const iresearch::bytes_ref& term) {
    field* fld = find( name );
    return fld && fld->find(term);
  }

  const field* find(size_t id) const {
    return id_to_field_.at(id);
  }

  field* find(const iresearch::string_ref& name) {
    auto it = fields_.find( name );
    return it == fields_.end()?nullptr:&it->second;
  }

  const field* find(const iresearch::string_ref& name) const {
    auto it = fields_.find( name );
    return it == fields_.end()?nullptr:&it->second;
  }

  template<typename Iterator>
  void add(Iterator begin, Iterator end) {
    // reset field per-document state
    for (auto it = begin; it != end; ++it) {
      auto* field_data = find((*it).name());

      if (!field_data) {
        continue;
      }

      field_data->pos = 0;
      field_data->offs = 0;
    }

    for (; begin != end; ++begin) {
      add(*begin);
    }

    ++count_;
  }

  void clear() {
    fields_.clear();
    count_ = 0;
  }

 private:
  void add( const ifield& f );

  std::vector<const field*> id_to_field_;
  field_map_t fields_;
  size_t count_;
  iresearch::document_mask doc_mask_;
};

/* -------------------------------------------------------------------
* FORMAT DEFINITION
* ------------------------------------------------------------------*/

namespace detail {

class term_reader : public iresearch::term_reader {
 public:
  term_reader(const tests::field& data):
    data_(data), min_(data_.terms.begin()->value), max_(data_.terms.rbegin()->value) {
  }

  virtual iresearch::seek_term_iterator::ptr iterator() const override;
  virtual const iresearch::flags& features() const override;
  virtual size_t size() const override;
  virtual uint64_t docs_count() const override;
  virtual const iresearch::bytes_ref& (min)() const override;
  virtual const iresearch::bytes_ref& (max)() const override;

 private:
  const tests::field& data_;
  iresearch::bytes_ref max_;
  iresearch::bytes_ref min_;
};

} // detail

/* -------------------------------------------------------------------
 * index_meta_writer 
 * ------------------------------------------------------------------*/

struct index_meta_writer: public iresearch::index_meta_writer {
  virtual std::string filename(
    const iresearch::index_meta& meta
  ) const override;
  virtual bool prepare(
    iresearch::directory& dir, 
    iresearch::index_meta& meta
  ) override;
  virtual void commit() override;
  virtual void rollback() NOEXCEPT override;
};

/* -------------------------------------------------------------------
 * index_meta_reader
 * ------------------------------------------------------------------*/

struct index_meta_reader : public iresearch::index_meta_reader {
  virtual bool last_segments_file(
    const iresearch::directory& dir, std::string& out
  ) const override;
  virtual void read(
    const iresearch::directory& dir,
    iresearch::index_meta& meta,
    const iresearch::string_ref& filename = iresearch::string_ref::nil
  ) override;
};

/* -------------------------------------------------------------------
 * segment_meta_writer 
 * ------------------------------------------------------------------*/

struct segment_meta_writer : public iresearch::segment_meta_writer {
  virtual std::string filename(
    const iresearch::segment_meta& meta
  ) const override;
  virtual void write(
    iresearch::directory& dir,
    const iresearch::segment_meta& meta
  ) override;
};

/* -------------------------------------------------------------------
 * segment_meta_reader
 * ------------------------------------------------------------------*/

struct segment_meta_reader : public iresearch::segment_meta_reader {
  virtual void read(
    const iresearch::directory& dir,
    iresearch::segment_meta& meta,
    const iresearch::string_ref& filename = iresearch::string_ref::nil
  ) override;
};

/* -------------------------------------------------------------------
 * document_mask_writer
 * ------------------------------------------------------------------*/

class document_mask_writer: public iresearch::document_mask_writer {
 public:
  document_mask_writer(const index_segment& data);
  virtual std::string filename(
    const iresearch::segment_meta& meta
  ) const override;
  void prepare(
    iresearch::directory& dir,
    const iresearch::segment_meta& meta
  ) override;

  void begin(uint32_t count) override;
  void write(const iresearch::doc_id_t& doc_id) override;
  void end() override;

 private:
  const index_segment& data_;
};

/* -------------------------------------------------------------------
 * field_meta_writer
 * ------------------------------------------------------------------*/

class field_meta_writer : public iresearch::field_meta_writer {
 public:
  field_meta_writer( const index_segment& data );

  void prepare(const iresearch::flush_state& state) override;
  void write(
    iresearch::field_id id, 
    const std::string& name, 
    const iresearch::flags& features,
    iresearch::field_id norm) override;
  void end() override;

 private:
  const index_segment& data_;
};

/* -------------------------------------------------------------------
* field_reader
* ------------------------------------------------------------------*/

class field_reader : public iresearch::field_reader {
 public:
  field_reader( const index_segment& data );

  virtual void prepare(const iresearch::reader_state& state) override;
  virtual const iresearch::term_reader* terms(iresearch::field_id field) const override;
  virtual size_t size() const override;
  
  const index_segment& data() const {
    return data_;
  }

 private:
  std::vector<iresearch::term_reader::ptr> readers_;
  const index_segment& data_;
};

/* -------------------------------------------------------------------
 * field_writer
 * ------------------------------------------------------------------*/

class field_writer : public iresearch::field_writer {
 public:
  field_writer(const index_segment& data, const iresearch::flags& features = iresearch::flags());

  /* returns features which should be checked
   * in "write" method */
  iresearch::flags features() const { return features_; }

  /* sets features which should be checked
   * in "write" method */
  void features(const iresearch::flags& features) { features_ = features; }

  virtual void prepare(const iresearch::flush_state& state) override;
  virtual void write(iresearch::field_id id, const iresearch::flags& expected_field, iresearch::term_iterator& actual_term) override;
  virtual void end() override;

 private:
  field_reader readers_;
  iresearch::flags features_;
};

/* -------------------------------------------------------------------
 * stored_fields_writer 
 * ------------------------------------------------------------------*/

struct ifield;

class stored_fields_writer : public iresearch::stored_fields_writer {
 public:
  explicit stored_fields_writer(const index_segment& data);

  virtual void prepare(
    iresearch::directory& dir,
    const iresearch::string_ref& seg_name
  ) override;

  virtual bool write(const iresearch::serializer&) override;
  virtual void end(const iresearch::serializer*) override;
  virtual void finish() override;
  virtual void reset() override;

 private:
  const index_segment& data_;
};

/* -------------------------------------------------------------------
 * format
 * ------------------------------------------------------------------*/

class format : public iresearch::format {
 public:
  DECLARE_FORMAT_TYPE();
  DECLARE_FACTORY_DEFAULT();
  format();
  format(const index_segment& data);

  virtual iresearch::index_meta_writer::ptr get_index_meta_writer() const override;
  virtual iresearch::index_meta_reader::ptr get_index_meta_reader() const override;

  virtual iresearch::segment_meta_writer::ptr get_segment_meta_writer() const override;
  virtual iresearch::segment_meta_reader::ptr get_segment_meta_reader() const override;

  virtual document_mask_writer::ptr get_document_mask_writer() const override;
  virtual iresearch::document_mask_reader::ptr get_document_mask_reader() const override;

  virtual iresearch::field_meta_reader::ptr get_field_meta_reader() const override;
  virtual iresearch::field_meta_writer::ptr get_field_meta_writer() const override;

  virtual iresearch::field_writer::ptr get_field_writer(bool volatile_attributes = false) const override;
  virtual iresearch::field_reader::ptr get_field_reader() const override;

  virtual iresearch::stored_fields_writer::ptr get_stored_fields_writer() const override;
  virtual iresearch::stored_fields_reader::ptr get_stored_fields_reader() const override;
  
  virtual iresearch::column_meta_writer::ptr get_column_meta_writer() const override;
  virtual iresearch::column_meta_reader::ptr get_column_meta_reader() const override;

  virtual iresearch::columnstore_writer::ptr get_columnstore_writer() const override;
  virtual iresearch::columnstore_reader::ptr get_columnstore_reader() const override;

 private:
  static const index_segment DEFAULT_SEGMENT;
  const index_segment& data_;
};

typedef std::vector<index_segment> index_t;

void assert_term(
  const iresearch::term_iterator& expected_term,
  const iresearch::term_iterator& actual_term,
  const iresearch::flags& features);

void assert_terms_next(
  const iresearch::term_reader& expected_term_reader,
  const iresearch::term_reader& actual_term_reader,
  const iresearch::flags& features);

void assert_terms_seek(
  const iresearch::term_reader& expected_term_reader,
  const iresearch::term_reader& actual_term_reader,
  const iresearch::flags& features,
  size_t lookahead = 10); // number of steps to iterate after the seek

void assert_index(
  const iresearch::directory& dir,
  iresearch::format::ptr codec,
  const index_t& index,
  const iresearch::flags& features,
  size_t skip = 0 // no not validate the first 'skip' segments
);

} // tests

#endif
