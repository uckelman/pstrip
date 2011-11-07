#pragma once

#include <ostream>
#include <stack>
#include <string>

class indent {
public:
  indent(unsigned int d): depth(d) {}

  std::ostream& operator()(std::ostream& out) const {
    for (unsigned int i = 0; i < depth; ++i) {
      out << ' ';
    }
    return out;
  }

private:
  unsigned int depth;
};

std::ostream& operator<<(std::ostream& out, indent func);

class quote {
public:
  quote(const std::string& s): str(s) {}

  quote(const char* s): str(s) {}

  std::ostream& operator()(std::ostream& out) const {
    out << '"';

// TODO: handle U+0000 - U+001F properly
    const std::string::const_iterator e(str.end());
    for (std::string::const_iterator i(str.begin()); i != e; ++i) {
      switch (*i) {
      case '"':
      case '\\':
        out << '\\' << *i;
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << *i;
      }
    }

    return out << '"';
  }

private:
  std::string str;
};

std::ostream& operator<<(std::ostream& out, quote func);

class JSON_writer {
public:
  JSON_writer(std::ostream& o);

  void object_open();
  void object_close();

  void array_open();
  void array_close();

  void object_member_open(const std::string& key);
  void object_member_close();

  void array_member_open(const std::string& key);
  void array_member_close();

  void key_write(const std::string& key);

  void value_write_null();
  void value_write_true();
  void value_write_false();

  template <typename T> void value_write(const T& value) {
    out << value;
  }

  void value_write(const std::string& value);
  void value_write(char* value);
  void value_write(const char* value);
  void value_write(const unsigned char* value, size_t length);

  template <typename T> void object_member_write(const std::string& key,
                                                 const T& value)
  {
    key_write(key);
    value_write(value);
  }

  void object_member_write_null(const std::string& key);
  void object_member_write_true(const std::string& key);
  void object_member_write_false(const std::string& key);

  void object_member_write(const std::string& key, char* value);
  void object_member_write(const std::string& key, const char* value);

  void object_member_write(const std::string& key,
                           const unsigned char* value, size_t length);

  template <typename T> void array_member_write(const T& value) {
    next_element();
    value_write(value);
  }

  void array_member_write_null();
  void array_member_write_true();
  void array_member_write_false();

  void array_member_write(const std::string& value);
  void array_member_write(char* value);
  void array_member_write(const char* value);
  void array_member_write(const unsigned char* value, size_t length);

  void reset();

private:
  void next_element();
  void write_key(const std::string& key);

  void scope_open(char delim);
  void scope_close(char delim);

  void member_scope_open(char delim);
  void member_scope_close(char delim);

  std::ostream& out;
  unsigned int depth;
  std::stack<bool> first_child;

  static const char* KVSEP;
};
