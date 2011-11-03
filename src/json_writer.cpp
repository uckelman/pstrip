#include <algorithm>
#include <iterator>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include "json_writer.h"

const char* JSON_writer::KVSEP = " : ";

std::ostream& operator<<(std::ostream& out, indent func) {
  return func(out);
}

std::ostream& operator<<(std::ostream& out, quote func) {
  return func(out);
}

JSON_writer::JSON_writer(std::ostream& o): out(o), depth(0) {
  first_child.push(true);
}

void JSON_writer::object_open() { scope_open('{'); }

void JSON_writer::object_close() { scope_close('}'); }

void JSON_writer::array_open() { scope_open('['); }

void JSON_writer::array_close() { scope_close(']'); }

void JSON_writer::scope_open(char delim) {
  next_element();
  out << indent(depth++) << delim << '\n';
  first_child.push(true);
}

void JSON_writer::scope_close(char delim) {
  if (!first_child.top()) {
    out << '\n';
  }

  out << indent(--depth) << delim;
  first_child.pop();
}

void JSON_writer::member_scope_open(char delim) {
  out << delim << '\n';
  ++depth;
  first_child.push(true);
}

void JSON_writer::member_scope_close(char delim) { scope_close(delim); }

void JSON_writer::object_member_open(const std::string& key) {
  next_element();
  write_key(key);
  member_scope_open('{');
}

void JSON_writer::object_member_close() { member_scope_close('}'); }

void JSON_writer::array_member_open(const std::string& key) {
  next_element();
  write_key(key);
  member_scope_open('[');
}

void JSON_writer::array_member_close() { member_scope_close(']'); }

void JSON_writer::null_write(const std::string& key) {
  next_element();
  write_key(key);
  out << "null";
}

void JSON_writer::scalar_write(const std::string& key, const std::string& value)
{
  next_element();
  write_key(key);
  out << quote(value);
}

void JSON_writer::scalar_write(const std::string& key, char* value)
{
  scalar_write(key, std::string(value));
}

void JSON_writer::scalar_write(const std::string& key, const char* value)
{
  scalar_write(key, std::string(value));
}

typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<const char*, 6, 8> > base64_iterator;

void JSON_writer::scalar_write(const std::string& key,
                               const unsigned char* value, size_t length) {
  next_element();
  out << indent(depth) << quote(key) << KVSEP << '"';

  std::copy(base64_iterator(value),
            base64_iterator(value + length),
            std::ostream_iterator<char>(out));

  // base64_iterator doesn't pad, so we have to do it
  switch (length % 3) {
  case 2:
    out << '=';
  case 1:
    out << '=';
  case 0:
    break;
  }

  out << '"';
}

void JSON_writer::reset() {
  out << '\n';

  while (!first_child.empty()) { first_child.pop(); }
  
  first_child.push(true);
  depth = 0;
}

void JSON_writer::next_element() {
  if (first_child.top()) {
    first_child.top() = false;
  }
  else {
    out << ",\n";
  }
}

void JSON_writer::write_key(const std::string& key) {
  out << indent(depth) << quote(key) << KVSEP;
}
