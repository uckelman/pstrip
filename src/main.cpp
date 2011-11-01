
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "main.h"

typedef boost::shared_ptr<libpff_file_t> FilePtr;
typedef boost::shared_ptr<libpff_item_t> ItemPtr;
typedef boost::shared_ptr<libpff_multi_value_t> MultiValuePtr;

class libpff_error: public std::exception {
public:
  libpff_error(libpff_error_t*& error) {
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

libpff_item_t* get_unknowns(libpff_item_t* folder) {
  libpff_item_t* unknowns = 0;
  libpff_error_t* error = 0;

  if (libpff_folder_get_unknowns(folder, &unknowns, &error) == -1) {
    throw libpff_error(error);
  }

  return unknowns;
}

void destroy_item(libpff_item_t* item) {
  libpff_error_t* error = 0;

  if (libpff_item_free(&item, &error) != 1) {
    throw libpff_error(error);
  }
}

libpff_multi_value_t* get_multivalue(libpff_item_t* item, uint32_t s, uint32_t etype) {
  libpff_multi_value_t* mv = 0;
  libpff_error_t* error = 0;

  if (libpff_item_get_entry_multi_value(item, s, etype, &mv, LIBPFF_ENTRY_VALUE_FLAG_IGNORE_NAME_TO_ID_MAP, &error) != 1) {
    throw libpff_error(error);
  }

  return mv;
}

void destroy_multivalue(libpff_multi_value_t* mv) {
  libpff_error_t* error = 0;
  
  if (libpff_multi_value_free(&mv, &error) != 1) {
    throw libpff_error(error);
  }
}

void handle_item_value(libpff_item_t* item, uint32_t s, uint32_t e, JSON_writer& json) {
  libpff_error_t* error = 0;

  uint32_t etype;
  uint32_t vtype;
  libpff_name_to_id_map_entry_t* nkey = 0;

  if (libpff_item_get_entry_type(item, s, e, &etype, &vtype, &nkey, &error) != 1) {
    throw libpff_error(error);
  }

  json.scalar_write("entry type", etype);
  json.scalar_write("value type", vtype);

  try {
    if (nkey) {
      uint8_t ntype;
      if (libpff_name_to_id_map_entry_get_type(nkey, &ntype, &error) != 1) {
        throw libpff_error(error);
      }

      if (ntype == LIBPFF_NAME_TO_ID_MAP_ENTRY_TYPE_NUMERIC) {
        uint32_t nnum;
        if (libpff_name_to_id_map_entry_get_number(nkey, &nnum, &error) != 1) {
          throw libpff_error(error);
        }

        json.scalar_write("maps to entry type", nnum);
      }
      else if (ntype == LIBPFF_NAME_TO_ID_MAP_ENTRY_TYPE_STRING) {
        size_t len;
        if (libpff_name_to_id_map_entry_get_utf8_string_size(nkey, &len, &error) != 1) {
          throw libpff_error(error);
        }

        boost::scoped_array<uint8_t> name(new uint8_t[len+1]);
        if (libpff_name_to_id_map_entry_get_utf8_string(nkey, name.get(), len, &error) != 1) {
          throw libpff_error(error);
        }

        json.scalar_write("maps to entry", (const char*) name.get());
      }
    }
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  uint32_t matched_vtype = LIBPFF_VALUE_TYPE_UNSPECIFIED;
  uint8_t* vdata = 0;
  size_t len;

  if (libpff_item_get_entry_value(item, s, etype, &matched_vtype, &vdata, &len, LIBPFF_ENTRY_VALUE_FLAG_MATCH_ANY_VALUE_TYPE | LIBPFF_ENTRY_VALUE_FLAG_IGNORE_NAME_TO_ID_MAP, &error) != 1) {
    throw libpff_error(error);
  }

  json.scalar_write("matched value type", matched_vtype);

  // export_handle_print_data
  json.scalar_write("value", vdata, len);

/*
  if (vtype & 0x1000) {
    MultiValuePtr mvp(get_multivalue(item, s, etype), &destroy_multivalue);
    // FIXME: Do something here? See multi_value functions...
  }
*/
}

void handle_item_values(libpff_item_t* item, JSON_writer& json) {
  // export_handle_export_item_values

  libpff_error_t* error = 0;

  // number of sets
  uint32_t sets;
  if (libpff_item_get_number_of_sets(item, &sets, &error) != 1) {
    throw libpff_error(error);
  }

  json.scalar_write("sets", sets);

  // number of entries per set
  uint32_t entries;
  if (libpff_item_get_number_of_entries(item, &entries, &error) != 1) {
    throw libpff_error(error);
  }

  json.scalar_write("entries per set", entries);

  // iterate over sets
  for (uint32_t s = 0; s < sets; ++s) {
    json.object_open();
    json.scalar_write("set", s);

    // iterate over entries
    for (uint32_t e = 0; e < entries; ++e) {
      json.object_open();
      json.scalar_write("entry", e);

      try {
        handle_item_value(item, s, e, json);
      }
      catch (const libpff_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
      }

      json.object_close();
    }

    json.object_close();
  }
}

void handle_folder(libpff_item_t* folder, JSON_writer& json) {
  // export_handle_export_folder

  libpff_error_t* error = 0;

  json.scalar_write("type", "folder");

  // id
  try {
    uint32_t id;
    if (libpff_item_get_identifier(folder, &id, &error) != 1) {
      throw libpff_error(error);
    }
    json.scalar_write("identifier", id);
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  // name
  try {
    size_t len;
    switch (libpff_folder_get_utf8_name_size(folder, &len, &error)) {
    case -1:
      throw libpff_error(error);
    case  0:
      // name does not exist
      break;
    case  1:
      if (len > 0) {
        boost::scoped_array<uint8_t> name(new uint8_t[len+1]);
        if (libpff_folder_get_utf8_name(folder, name.get(), len, &error) != 1) {
          throw libpff_error(error);
        }
        json.scalar_write("name", (const char*) name.get());
      }
    }
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl; 
  }

  // item values
  try { 
    handle_item_values(folder, json);
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl; 
  }

  /*
  // unknowns
  try {
    // export_handle_export_unknowns
    ItemPtr unknownsp(get_unknowns(folder), &destroy_item);
    if (unknownsp) {
      json.object_open();

      libpff_item_t* unknowns = unknownsp.get();

      uint32_t ucount;
      if (libpff_item_get_number_of_sets(unknowns, &ucount, &error) == -1) {
        throw libpff_error(error);
      }

      for (uint32_t u = 0; u < ucount; ++u) {
        // FIXME: do something here?
      }

      json.object_close();
    }
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  */

  try {
    handle_subitems(folder, json);
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}

void handle_subitems(libpff_item_t* item, JSON_writer& json) {
  libpff_error_t* error = 0;

  int children;
  if (libpff_item_get_number_of_sub_items(item, &children, &error) != 1) {
    throw libpff_error(error);
  }

  for (int i = 0; i < children; ++i) {
    try {
      ItemPtr childp(get_child(item, i), &destroy_item);
      handle_item(childp.get(), json);
    }
    catch (const libpff_error& e) {
      std::cerr << "Error: " << e.what() << std::endl; 
    }
  }
}

void handle_item(libpff_item_t* item, JSON_writer& json) {
  // export_handle_export_items

  json.object_open();

  libpff_error_t* error = 0;

  try {
    // item type
    uint8_t type;
    if (libpff_item_get_type(item, &type, &error) != 1) {
      throw libpff_error(error);
    }
 
    std::string typestr;

    switch (type) {
    case LIBPFF_ITEM_TYPE_UNDEFINED:
      typestr = "undefined";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_ACTIVITY:
      typestr = "activity";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_APPOINTMENT:
      typestr = "appointment";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_ATTACHMENT:
      typestr = "attachment";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_ATTACHMENTS:
      typestr = "attachments";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_COMMON:
      typestr = "common";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_CONFIGURATION:
      typestr = "configuration";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_CONFLICT_MESSAGE:
      typestr = "conflict message";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_CONTACT:
      typestr = "contact";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_DISTRIBUTION_LIST:
      typestr = "distribution list";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_DOCUMENT:
      typestr = "document";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_EMAIL:
      typestr = "email";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_EMAIL_SMIME:
      typestr = "SMIME email";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_FAX:
      typestr = "fax";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_FOLDER:
      handle_folder(item, json);
      break;
    case LIBPFF_ITEM_TYPE_MEETING:
      typestr = "meeting";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_MMS:
      typestr = "mms";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_NOTE:
      typestr = "note";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_POSTING_NOTE:
      typestr = "posting note";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_RECIPIENTS:
      typestr = "recipients";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_RSS_FEED:
      typestr = "RSS feed";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_SHARING:
      typestr = "sharing";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_SMS:
      typestr = "SMS";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_SUB_ASSOCIATED_CONTENTS:
      typestr = "associated contents";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_SUB_FOLDERS:
      typestr = "folders";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_SUB_MESSAGES:
      typestr = "messages";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_TASK:
      typestr = "task";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_TASK_REQUEST:
      typestr = "request";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_VOICEMAIL:
      typestr = "voicemail";
      json.scalar_write("type", typestr);
      break;
    case LIBPFF_ITEM_TYPE_UNKNOWN:
      typestr = "unknown";
      json.scalar_write("type", typestr);
      break;
    }

    // process children
    try {
      handle_subitems(item, json);
    }
    catch (const libpff_error& e) {
      std::cerr << "Error: " << e.what() << std::endl; 
    }
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    json.scalar_write("bad", "bad!"); 
  }

  json.object_close();
}

void handle_root(libpff_item_t* root, JSON_writer& json) {
  json.object_open();

  try {
    handle_subitems(root, json);
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << e.what() << std::endl; 
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
    handle_root(root, json);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
