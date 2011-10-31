#include "json_writer.h"

std::ostream& operator<<(std::ostream& out, indent func) {
  return func(out);
}

std::ostream& operator<<(std::ostream& out, quote func) {
  return func(out);
}

void JSON_writer::object_open() {
  next_element(); 
  out << indent(depth++) << "{\n";
  first_child = true;
}

void JSON_writer::object_close() {
  out << '\n' << indent(--depth) << '}';
}

template <> void JSON_writer::scalar_write<std::string>(
  const std::string& key,
  const std::string& value)
{
  next_element();
  out << indent(depth) << quote(key) << " : " << quote(value);
}

void JSON_writer::next_element() {
  if (first_child) {
    first_child = false;
  }
  else {
    out << ",\n";
  }
}
