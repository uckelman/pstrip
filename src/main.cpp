
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <libpff.h>

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

std::ostream& operator<<(std::ostream& out, indent func) {
  return func(out);
}

class quote {
public:
  quote(const std::string& s): str(s) {}

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

std::ostream& operator<<(std::ostream& out, quote func) {
  return func(out);
}

class JSON_writer {
public:
  JSON_writer(std::ostream& o): out(o), depth(0), first_child(true) {} 

  void object_open() {
    next_element(); 
    out << indent(depth++) << "{\n";
    first_child = true;
  }

  void object_close() {
    out << '\n' << indent(--depth) << '}';
  }

  template <typename T> void scalar_write(const std::string& key, const T& value) {
    next_element();
    out << indent(depth) << quote(key) << " : " << value;
  }

  void scalar_write(const std::string& key, const std::string& value) {
    next_element();
    out << indent(depth) << quote(key) << " : " << quote(value);
  }

private:
  void next_element() {
    if (first_child) {
      first_child = false;
    }
    else {
      out << ",\n";
    }
  }

  std::ostream& out;
  unsigned int depth;
  bool first_child;
};

typedef boost::shared_ptr<libpff_file_t> FilePtr;
typedef boost::shared_ptr<libpff_item_t> ItemPtr;

class libpff_error: public std::exception {
public:
  libpff_error(libpff_error_t* error) {
    libpff_error_sprint(error, msg, MAXLEN);
    libpff_error_free(&error);
  }

  virtual ~libpff_error() throw() {}

  virtual const char* what() const throw() { return msg; }

private:
  static const size_t MAXLEN = 1024;

  char msg[MAXLEN];
};

libpff_file_t* create_file(const char* filename) {
  libpff_file_t* file = 0;
  libpff_error_t* error = 0;

  if (libpff_file_initialize(&file, &error) != 1) {
    throw libpff_error(error);
  }
  
  if (libpff_file_open(file, filename, LIBPFF_OPEN_READ, &error) != 1) {
    throw libpff_error(error);
  }

  return file;
}

void destroy_file(libpff_file_t* file) {
  libpff_error_t* error = 0;

  if (libpff_file_close(file, &error) != 0) {
    throw libpff_error(error);
  }

  if (libpff_file_free(&file, &error) != 1) {
    throw libpff_error(error);
  }
}

libpff_item_t* get_root(libpff_file_t* file) {
  libpff_item_t* root = 0;
  libpff_error_t* error = 0;

  if (libpff_file_get_root_item(file, &root, &error) != 1) {
    throw libpff_error(error);
  }
  
  return root;
}

libpff_item_t* get_child(libpff_item_t* parent, int pos) {
  libpff_item_t* child = 0;
  libpff_error_t* error = 0;

  if (libpff_item_get_sub_item(parent, pos, &child, &error) != 1) {
    throw libpff_error(error);
  }
  
  return child;
}

void destroy_item(libpff_item_t* item) {
  libpff_error_t* error = 0;

  if (libpff_item_free(&item, &error) != 1) {
    throw libpff_error(error);
  }
}

void traverse(libpff_item_t* item, JSON_writer& json) {
  json.object_open();

  libpff_error_t* error = 0;

  // process this item
  uint8_t type;
  if (libpff_item_get_type(item, &type, &error) != 1) {
    libpff_error_fprint(error, stderr);
    type = LIBPFF_ITEM_TYPE_UNDEFINED;
  }

  std::string typestr;

// HERE: working from export_handle_export_item

  switch (type) {
  case LIBPFF_ITEM_TYPE_UNDEFINED:
    typestr = "undefined";
    break;
  case LIBPFF_ITEM_TYPE_ACTIVITY:
    typestr = "activity";
    break;
  case LIBPFF_ITEM_TYPE_APPOINTMENT:
    typestr = "appointment";
    break;
  case LIBPFF_ITEM_TYPE_ATTACHMENT:
    typestr = "attachment";
    break;
  case LIBPFF_ITEM_TYPE_ATTACHMENTS:
    typestr = "attachments";
    break;
  case LIBPFF_ITEM_TYPE_COMMON:
    typestr = "common";
    break;
  case LIBPFF_ITEM_TYPE_CONFIGURATION:
    typestr = "configuration";
    break;
  case LIBPFF_ITEM_TYPE_CONFLICT_MESSAGE:
    typestr = "conflict message";
    break;
  case LIBPFF_ITEM_TYPE_CONTACT:
    typestr = "contact";
    break;
  case LIBPFF_ITEM_TYPE_DISTRIBUTION_LIST:
    typestr = "distribution list";
    break;
  case LIBPFF_ITEM_TYPE_DOCUMENT:
    typestr = "document";
    break;
  case LIBPFF_ITEM_TYPE_EMAIL:
    typestr = "email";
    break;
  case LIBPFF_ITEM_TYPE_EMAIL_SMIME:
    typestr = "SMIME email";
    break;
  case LIBPFF_ITEM_TYPE_FAX:
    typestr = "fax";
    break;
  case LIBPFF_ITEM_TYPE_FOLDER:
    {
      typestr = "folder";

      uint32_t id;
      if (libpff_item_get_identifier(item, &id, &error) != 1) {
        throw libpff_error(error);
      }

      json.scalar_write("identifier", id);

// TODO: this seems to produce junk
      size_t len;
      libpff_folder_get_utf8_name_size(item, &len, 0);

      boost::scoped_array<uint8_t> name(new uint8_t[len+1]);
      libpff_folder_get_utf8_name(item, name.get(), len, 0);

      json.scalar_write("name", name.get());
    }
    break;
  case LIBPFF_ITEM_TYPE_MEETING:
    typestr = "meeting";
    break;
  case LIBPFF_ITEM_TYPE_MMS:
    typestr = "mms";
    break;
  case LIBPFF_ITEM_TYPE_NOTE:
    typestr = "note";
    break;
  case LIBPFF_ITEM_TYPE_POSTING_NOTE:
    typestr = "posting note";
    break;
  case LIBPFF_ITEM_TYPE_RECIPIENTS:
    typestr = "recipients";
    break;
  case LIBPFF_ITEM_TYPE_RSS_FEED:
    typestr = "RSS feed";
    break;
  case LIBPFF_ITEM_TYPE_SHARING:
    typestr = "sharing";
    break;
  case LIBPFF_ITEM_TYPE_SMS:
    typestr = "SMS";
    break;
  case LIBPFF_ITEM_TYPE_SUB_ASSOCIATED_CONTENTS:
    typestr = "associated contents";
    break;
  case LIBPFF_ITEM_TYPE_SUB_FOLDERS:
    typestr = "folders";
    break;
  case LIBPFF_ITEM_TYPE_SUB_MESSAGES:
    typestr = "messages";
    break;
  case LIBPFF_ITEM_TYPE_TASK:
    typestr = "task";
    break;
  case LIBPFF_ITEM_TYPE_TASK_REQUEST:
    typestr = "request";
    break;
  case LIBPFF_ITEM_TYPE_VOICEMAIL:
    typestr = "voicemail";
    break;
  case LIBPFF_ITEM_TYPE_UNKNOWN:
    typestr = "unknown";
    break;
  }

  json.scalar_write("type", typestr);

  // process children
  int childnum;
  if (libpff_item_get_number_of_sub_items(item, &childnum, &error) != 1) {
    throw libpff_error(error);
  }

  for (int i = 0; i < childnum; ++i) {
    ItemPtr childp(get_child(item, i), &destroy_item);
    traverse(childp.get(), json);
  }

  json.object_close();
}

int main(int argc, char** argv) {
  try {
    if (argc != 2) {
      throw std::runtime_error("wrong number of arguments");
    }

    // setup
    FilePtr filep(create_file(argv[1]), &destroy_file);
    libpff_file_t* file = filep.get();

    // find the root
    ItemPtr rootp(get_root(file), &destroy_item);
    libpff_item_t* root = rootp.get();

    // process the tree
    JSON_writer json(std::cout);
    traverse(root, json);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
