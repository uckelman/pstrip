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

void JSON_writer::object_open() { scope_open('{'); }

void JSON_writer::object_close() { scope_close('}'); }

void JSON_writer::list_open() { scope_open('['); }

void JSON_writer::list_close() { scope_close(']'); }

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

void JSON_writer::scalar_write(const std::string& key, const std::string& value)
{
  next_element();
  out << indent(depth) << quote(key) << KVSEP << quote(value);
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
// FIXME: does this pad correctly?
  std::copy(base64_iterator(value),
            base64_iterator(value + length),
            std::ostream_iterator<char>(out));
//  std::copy(value, value + length, std::ostream_iterator<char>(out));
  out << '"';
}

void JSON_writer::next_element() {
  if (first_child.top()) {
    first_child.top() = false;
  }
  else {
    out << ",\n";
  }
}
