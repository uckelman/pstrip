
#include <cstring>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>

#include <libpff.h>
#include <libpff/mapi.h>

#include "json_writer.h"

template <typename L, typename R> std::string operator+(L left, R right) {
  std::ostringstream os;
  os << left << right;
  return os.str();
}

void handle_item(libpff_item_t* item, const std::string& path, JSON_writer& json);

typedef boost::shared_ptr<libpff_file_t> FilePtr;
typedef boost::shared_ptr<libpff_item_t> ItemPtr;
typedef boost::shared_ptr<libpff_multi_value_t> MultiValuePtr;

class libpff_error: public std::exception {
public:
  libpff_error(libpff_error_t*& error, uint32_t line) {
    std::stringstream ss;
    ss << line << ": ";

    char buf[MAXLEN];
    libpff_error_sprint(error, buf, MAXLEN);
    libpff_error_free(&error);
    ss << buf;

    msg = ss.str();
  }

  libpff_error(const std::string& s, uint32_t line) {
    std::stringstream ss;
    ss << line << ": " << s;
    msg = ss.str();
  }

  virtual ~libpff_error() throw() {}

  virtual const char* what() const throw() { return msg.c_str(); }

private:
  static const size_t MAXLEN = 1024;

  std::string msg;
};

libpff_file_t* create_file(const char* filename) {
  libpff_file_t* file = 0;
  libpff_error_t* error = 0;

  if (libpff_file_initialize(&file, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  if (libpff_file_open(file, filename, LIBPFF_OPEN_READ, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  return file;
}

void destroy_file(libpff_file_t* file) {
  libpff_error_t* error = 0;

  if (libpff_file_close(file, &error) != 0) {
    throw libpff_error(error, __LINE__);
  }

  if (libpff_file_free(&file, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }
}

libpff_item_t* get_root(libpff_file_t* file) {
  libpff_item_t* root = 0;
  libpff_error_t* error = 0;

  if (libpff_file_get_root_item(file, &root, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  return root;
}

libpff_item_t* get_child(libpff_item_t* parent, int pos) {
  libpff_item_t* child = 0;
  libpff_error_t* error = 0;

  if (libpff_item_get_sub_item(parent, pos, &child, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  return child;
}

libpff_item_t* get_unknowns(libpff_item_t* folder) {
  libpff_item_t* unknowns = 0;
  libpff_error_t* error = 0;

  if (libpff_folder_get_unknowns(folder, &unknowns, &error) == -1) {
    throw libpff_error(error, __LINE__);
  }

  return unknowns;
}

libpff_item_t* get_orphan(libpff_file_t* file, int pos) {
  libpff_item_t* orphan = 0;
  libpff_error_t* error = 0;

  if (libpff_file_get_orphan_item(file, pos, &orphan, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  return orphan;
}

libpff_item_t* get_recovered(libpff_file_t* file, int pos) {
  libpff_item_t* rec = 0;
  libpff_error_t* error = 0;

  if (libpff_file_get_recovered_item(file, pos, &rec, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  return rec;
}

void destroy_item(libpff_item_t* item) {
  libpff_error_t* error = 0;

  if (libpff_item_free(&item, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }
}

libpff_multi_value_t* get_multivalue(libpff_item_t* item, uint32_t s, uint32_t etype, uint8_t flags) {
  libpff_multi_value_t* mv = 0;
  libpff_error_t* error = 0;

  if (libpff_item_get_entry_multi_value(item, s, etype, &mv, flags, &error) == -1) {
    throw libpff_error(error, __LINE__);
  }

  return mv;
}

void destroy_multivalue(libpff_multi_value_t* mv) {
  libpff_error_t* error = 0;

  if (libpff_multi_value_free(&mv, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }
}

std::string item_type_string(uint32_t itype) {
  switch (itype) {
  case LIBPFF_ITEM_TYPE_UNDEFINED:
    return "UNDEFINED";
  case LIBPFF_ITEM_TYPE_ACTIVITY:
    return "ACTIVITY";
  case LIBPFF_ITEM_TYPE_APPOINTMENT:
    return "APPOINTMENT";
  case LIBPFF_ITEM_TYPE_ATTACHMENT:
    return "ATTACHMENT";
  case LIBPFF_ITEM_TYPE_ATTACHMENTS:
    return "ATTACHMENTS";
  case LIBPFF_ITEM_TYPE_COMMON:
    return "COMMON";
  case LIBPFF_ITEM_TYPE_CONFIGURATION:
    return "CONFIGURATION";
  case LIBPFF_ITEM_TYPE_CONFLICT_MESSAGE:
    return "CONFLICT_MESSAGE";
  case LIBPFF_ITEM_TYPE_CONTACT:
    return "CONTACT";
  case LIBPFF_ITEM_TYPE_DISTRIBUTION_LIST:
    return "DISTRIBUTION_LIST";
  case LIBPFF_ITEM_TYPE_DOCUMENT:
    return "DOCUMENT";
  case LIBPFF_ITEM_TYPE_EMAIL:
    return "EMAIL";
  case LIBPFF_ITEM_TYPE_EMAIL_SMIME:
    return "EMAIL_SMIME";
  case LIBPFF_ITEM_TYPE_FAX:
    return "FAX";
  case LIBPFF_ITEM_TYPE_FOLDER:
    return "FOLDER";
  case LIBPFF_ITEM_TYPE_MEETING:
    return "MEETING";
  case LIBPFF_ITEM_TYPE_MMS:
    return "MMS";
  case LIBPFF_ITEM_TYPE_NOTE:
    return "NOTE";
  case LIBPFF_ITEM_TYPE_POSTING_NOTE:
    return "POSTING_NOTE";
  case LIBPFF_ITEM_TYPE_RECIPIENTS:
    return "RECIPIENTS";
  case LIBPFF_ITEM_TYPE_RSS_FEED:
    return "RSS_FEED";
  case LIBPFF_ITEM_TYPE_SHARING:
    return "SHARING";
  case LIBPFF_ITEM_TYPE_SMS:
    return "SMS";
  case LIBPFF_ITEM_TYPE_SUB_ASSOCIATED_CONTENTS:
    return "SUB_ASSOCIATED_CONTENTS";
  case LIBPFF_ITEM_TYPE_SUB_FOLDERS:
    return "SUB_FOLDERS";
  case LIBPFF_ITEM_TYPE_SUB_MESSAGES:
    return "SUB_MESSAGES";
  case LIBPFF_ITEM_TYPE_TASK:
    return "TASK";
  case LIBPFF_ITEM_TYPE_TASK_REQUEST:
    return "TASK_REQUEST";
  case LIBPFF_ITEM_TYPE_VOICEMAIL:
    return "VOICEMAIL";
  case LIBPFF_ITEM_TYPE_UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNRECOGNIZED";
  }
}

std::string entry_type_string(uint32_t etype) {
  switch (etype) {
  case LIBPFF_ENTRY_TYPE_MESSAGE_IMPORTANCE:
    return "MESSAGE_IMPORTANCE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_CLASS:
    return "MESSAGE_CLASS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_PRIORITY:
    return "MESSAGE_PRIORITY";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENSITIVITY:
    return "MESSAGE_SENSITIVITY";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SUBJECT:
    return "MESSAGE_SUBJECT";
  case LIBPFF_ENTRY_TYPE_MESSAGE_CLIENT_SUBMIT_TIME:
    return "MESSAGE_CLIENT_SUBMIT_TIME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENT_REPRESENTING_SEARCH_KEY:
    return "MESSAGE_SENT_REPRESENTING_SEARCH_KEY";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_BY_ENTRY_IDENTIFIER:
    return "MESSAGE_RECEIVED_BY_ENTRY_IDENTIFIER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_BY_NAME:
    return "MESSAGE_RECEIVED_BY_NAME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENT_REPRESENTING_ENTRY_IDENTIFIER:
    return "MESSAGE_SENT_REPRESENTING_ENTRY_IDENTIFIER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENT_REPRESENTING_NAME:
    return "MESSAGE_SENT_REPRESENTING_NAME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_REPRESENTING_ENTRY_IDENTIFIER:
    return "MESSAGE_RECEIVED_REPRESENTING_ENTRY_IDENTIFIER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_REPRESENTING_NAME:
    return "MESSAGE_RECEIVED_REPRESENTING_NAME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_REPLY_RECIPIENT_ENTRIES:
    return "MESSAGE_REPLY_RECIPIENT_ENTRIES";
  case LIBPFF_ENTRY_TYPE_MESSAGE_REPLY_RECIPIENT_NAMES:
    return "MESSAGE_REPLY_RECIPIENT_NAMES";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_BY_SEARCH_KEY:
    return "MESSAGE_RECEIVED_BY_SEARCH_KEY";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_REPRESENTING_SEARCH_KEY:
    return "MESSAGE_RECEIVED_REPRESENTING_SEARCH_KEY";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENT_REPRESENTING_ADDRESS_TYPE:
    return "MESSAGE_SENT_REPRESENTING_ADDRESS_TYPE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENT_REPRESENTING_EMAIL_ADDRESS:
    return "MESSAGE_SENT_REPRESENTING_EMAIL_ADDRESS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_CONVERSATION_TOPIC:
    return "MESSAGE_CONVERSATION_TOPIC";
  case LIBPFF_ENTRY_TYPE_MESSAGE_CONVERSATION_INDEX:
    return "MESSAGE_CONVERSATION_INDEX";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_BY_ADDRESS_TYPE:
    return "MESSAGE_RECEIVED_BY_ADDRESS_TYPE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_BY_EMAIL_ADDRESS:
    return "MESSAGE_RECEIVED_BY_EMAIL_ADDRESS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_REPRESENTING_ADDRESS_TYPE:
    return "MESSAGE_RECEIVED_REPRESENTING_ADDRESS_TYPE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_RECEIVED_REPRESENTING_EMAIL_ADDRESS:
    return "MESSAGE_RECEIVED_REPRESENTING_EMAIL_ADDRESS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_TRANSPORT_HEADERS:
    return "MESSAGE_TRANSPORT_HEADERS";
  case LIBPFF_ENTRY_TYPE_RECIPIENT_TYPE:
    return "RECIPIENT_TYPE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENDER_ENTRY_IDENTIFIER:
    return "MESSAGE_SENDER_ENTRY_IDENTIFIER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENDER_NAME:
    return "MESSAGE_SENDER_NAME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENDER_SEARCH_KEY:
    return "MESSAGE_SENDER_SEARCH_KEY";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENDER_ADDRESS_TYPE:
    return "MESSAGE_SENDER_ADDRESS_TYPE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SENDER_EMAIL_ADDRESS:
    return "MESSAGE_SENDER_EMAIL_ADDRESS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_DISPLAY_TO:
    return "MESSAGE_DISPLAY_TO";
  case LIBPFF_ENTRY_TYPE_MESSAGE_DELIVERY_TIME:
    return "MESSAGE_DELIVERY_TIME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_FLAGS:
    return "MESSAGE_FLAGS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_SIZE:
    return "MESSAGE_SIZE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_STATUS:
    return "MESSAGE_STATUS";
  case LIBPFF_ENTRY_TYPE_ATTACHMENT_SIZE:
    return "ATTACHMENT_SIZE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_INTERNET_ARTICLE_NUMBER:
    return "MESSAGE_INTERNET_ARTICLE_NUMBER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_PERMISSION:
    return "MESSAGE_PERMISSION";
  case LIBPFF_ENTRY_TYPE_MESSAGE_URL_COMPUTER_NAME_SET:
    return "MESSAGE_URL_COMPUTER_NAME_SET";
  case LIBPFF_ENTRY_TYPE_MESSAGE_TRUST_SENDER:
    return "MESSAGE_TRUST_SENDER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_BODY_PLAIN_TEXT:
    return "MESSAGE_BODY_PLAIN_TEXT";
  case LIBPFF_ENTRY_TYPE_MESSAGE_BODY_COMPRESSED_RTF:
    return "MESSAGE_BODY_COMPRESSED_RTF";
  case LIBPFF_ENTRY_TYPE_MESSAGE_BODY_HTML:
    return "MESSAGE_BODY_HTML";
  case LIBPFF_ENTRY_TYPE_EMAIL_EML_FILENAME:
    return "EMAIL_EML_FILENAME";
  case LIBPFF_ENTRY_TYPE_DISPLAY_NAME:
    return "DISPLAY_NAME";
  case LIBPFF_ENTRY_TYPE_ADDRESS_TYPE:
    return "ADDRESS_TYPE";
  case LIBPFF_ENTRY_TYPE_EMAIL_ADDRESS:
    return "EMAIL_ADDRESS";
  case LIBPFF_ENTRY_TYPE_MESSAGE_CREATION_TIME:
    return "MESSAGE_CREATION_TIME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_MODIFICATION_TIME:
    return "MESSAGE_MODIFICATION_TIME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_STORE_VALID_FOLDER_MASK:
    return "MESSAGE_STORE_VALID_FOLDER_MASK";
  case LIBPFF_ENTRY_TYPE_FOLDER_TYPE:
    return "FOLDER_TYPE";
  case LIBPFF_ENTRY_TYPE_NUMBER_OF_CONTENT_ITEMS:
    return "NUMBER_OF_CONTENT_ITEMS";
  case LIBPFF_ENTRY_TYPE_NUMBER_OF_UNREAD_CONTENT_ITEMS:
    return "NUMBER_OF_UNREAD_CONTENT_ITEMS";
  case LIBPFF_ENTRY_TYPE_HAS_SUB_FOLDERS:
    return "HAS_SUB_FOLDERS";
  case LIBPFF_ENTRY_TYPE_CONTAINER_CLASS:
    return "CONTAINER_CLASS";
  case LIBPFF_ENTRY_TYPE_NUMBER_OF_ASSOCIATED_CONTENT:
    return "NUMBER_OF_ASSOCIATED_CONTENT";
  case LIBPFF_ENTRY_TYPE_ATTACHMENT_DATA_OBJECT:
    return "ATTACHMENT_DATA_OBJECT";
  case LIBPFF_ENTRY_TYPE_ATTACHMENT_FILENAME_SHORT:
    return "ATTACHMENT_FILENAME_SHORT";
  case LIBPFF_ENTRY_TYPE_ATTACHMENT_METHOD:
    return "ATTACHMENT_METHOD";
  case LIBPFF_ENTRY_TYPE_ATTACHMENT_FILENAME_LONG:
    return "ATTACHMENT_FILENAME_LONG";
  case LIBPFF_ENTRY_TYPE_ATTACHMENT_RENDERING_POSITION:
    return "ATTACHMENT_RENDERING_POSITION";
  case LIBPFF_ENTRY_TYPE_CONTACT_CALLBACK_PHONE_NUMBER:
    return "CONTACT_CALLBACK_PHONE_NUMBER";
  case LIBPFF_ENTRY_TYPE_CONTACT_GENERATIONAL_ABBREVIATION:
    return "CONTACT_GENERATIONAL_ABBREVIATION";
  case LIBPFF_ENTRY_TYPE_CONTACT_GIVEN_NAME:
    return "CONTACT_GIVEN_NAME";
  case LIBPFF_ENTRY_TYPE_CONTACT_BUSINESS_PHONE_NUMBER_1:
    return "CONTACT_BUSINESS_PHONE_NUMBER_1";
  case LIBPFF_ENTRY_TYPE_CONTACT_HOME_PHONE_NUMBER:
    return "CONTACT_HOME_PHONE_NUMBER";
  case LIBPFF_ENTRY_TYPE_CONTACT_INITIALS:
    return "CONTACT_INITIALS";
  case LIBPFF_ENTRY_TYPE_CONTACT_SURNAME:
    return "CONTACT_SURNAME";
  case LIBPFF_ENTRY_TYPE_CONTACT_POSTAL_ADDRESS:
    return "CONTACT_POSTAL_ADDRESS";
  case LIBPFF_ENTRY_TYPE_CONTACT_COMPANY_NAME:
    return "CONTACT_COMPANY_NAME";
  case LIBPFF_ENTRY_TYPE_CONTACT_JOB_TITLE:
    return "CONTACT_JOB_TITLE";
  case LIBPFF_ENTRY_TYPE_CONTACT_DEPARTMENT_NAME:
    return "CONTACT_DEPARTMENT_NAME";
  case LIBPFF_ENTRY_TYPE_CONTACT_OFFICE_LOCATION:
    return "CONTACT_OFFICE_LOCATION";
  case LIBPFF_ENTRY_TYPE_CONTACT_PRIMARY_PHONE_NUMBER:
    return "CONTACT_PRIMARY_PHONE_NUMBER";
  case LIBPFF_ENTRY_TYPE_CONTACT_BUSINESS_PHONE_NUMBER_2:
    return "CONTACT_BUSINESS_PHONE_NUMBER_2";
  case LIBPFF_ENTRY_TYPE_CONTACT_MOBILE_PHONE_NUMBER:
    return "CONTACT_MOBILE_PHONE_NUMBER";
  case LIBPFF_ENTRY_TYPE_CONTACT_BUSINESS_FAX_NUMBER:
    return "CONTACT_BUSINESS_FAX_NUMBER";
  case LIBPFF_ENTRY_TYPE_CONTACT_COUNTRY:
    return "CONTACT_COUNTRY";
  case LIBPFF_ENTRY_TYPE_CONTACT_LOCALITY:
    return "CONTACT_LOCALITY";
  case LIBPFF_ENTRY_TYPE_CONTACT_TITLE:
    return "CONTACT_TITLE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_BODY_CODEPAGE:
    return "MESSAGE_BODY_CODEPAGE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_CODEPAGE:
    return "MESSAGE_CODEPAGE";
  case LIBPFF_ENTRY_TYPE_RECIPIENT_DISPLAY_NAME:
    return "RECIPIENT_DISPLAY_NAME";
  case LIBPFF_ENTRY_TYPE_FOLDER_CHILD_COUNT:
    return "FOLDER_CHILD_COUNT";
  case LIBPFF_ENTRY_TYPE_SUB_ITEM_IDENTIFIER:
    return "SUB_ITEM_IDENTIFIER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_STORE_PASSWORD_CHECKSUM:
    return "MESSAGE_STORE_PASSWORD_CHECKSUM";
  case LIBPFF_ENTRY_TYPE_ADDRESS_FILE_UNDER:
    return "ADDRESS_FILE_UNDER";
  case LIBPFF_ENTRY_TYPE_TASK_STATUS:
    return "TASK_STATUS";
  case LIBPFF_ENTRY_TYPE_TASK_PERCENTAGE_COMPLETE:
    return "TASK_PERCENTAGE_COMPLETE";
  case LIBPFF_ENTRY_TYPE_TASK_START_DATE:
    return "TASK_START_DATE";
  case LIBPFF_ENTRY_TYPE_TASK_DUE_DATE:
    return "TASK_DUE_DATE";
  case LIBPFF_ENTRY_TYPE_TASK_ACTUAL_EFFORT:
    return "TASK_ACTUAL_EFFORT";
  case LIBPFF_ENTRY_TYPE_TASK_TOTAL_EFFORT:
    return "TASK_TOTAL_EFFORT";
  case LIBPFF_ENTRY_TYPE_TASK_VERSION:
    return "TASK_VERSION";
  case LIBPFF_ENTRY_TYPE_TASK_IS_COMPLETE:
    return "TASK_IS_COMPLETE";
  case LIBPFF_ENTRY_TYPE_TASK_IS_RECURRING:
    return "TASK_IS_RECURRING";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_BUSY_STATUS:
    return "APPOINTMENT_BUSY_STATUS";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_LOCATION:
    return "APPOINTMENT_LOCATION";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_START_TIME:
    return "APPOINTMENT_START_TIME";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_END_TIME:
    return "APPOINTMENT_END_TIME";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_DURATION:
    return "APPOINTMENT_DURATION";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_IS_RECURRING:
    return "APPOINTMENT_IS_RECURRING";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_RECURRENCE_PATTERN:
    return "APPOINTMENT_RECURRENCE_PATTERN";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_TIMEZONE_DESCRIPTION:
    return "APPOINTMENT_TIMEZONE_DESCRIPTION";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_FIRST_EFFECTIVE_TIME:
    return "APPOINTMENT_FIRST_EFFECTIVE_TIME";
  case LIBPFF_ENTRY_TYPE_APPOINTMENT_LAST_EFFECTIVE_TIME:
    return "APPOINTMENT_LAST_EFFECTIVE_TIME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_REMINDER_TIME:
    return "MESSAGE_REMINDER_TIME";
  case LIBPFF_ENTRY_TYPE_MESSAGE_IS_REMINDER:
    return "MESSAGE_IS_REMINDER";
  case LIBPFF_ENTRY_TYPE_MESSAGE_IS_PRIVATE:
    return "MESSAGE_IS_PRIVATE";
  case LIBPFF_ENTRY_TYPE_MESSAGE_REMINDER_SIGNAL_TIME:
    return "MESSAGE_REMINDER_SIGNAL_TIME";
  default:
    return "UNRECOGNIZED";
  }
}

template <typename L, typename G> void write_binary_value(
  L length_getter,
  G value_getter,
  libpff_item_t* item,
  uint32_t si,
  uint32_t etype,
  uint8_t flags,
  const std::string& key,
  JSON_writer& json)
{
  libpff_error_t* error = 0;
  size_t len;
  switch (length_getter(item, si, etype, &len, flags, &error)) {
  case -1:
    throw libpff_error(error, __LINE__);
  case  0:
    break;
  case  1:
    boost::scoped_array<uint8_t> buf(new uint8_t[len]);
    if (value_getter(item, si, etype, buf.get(), len, flags, &error) != 1) {
      throw libpff_error(error, __LINE__);
    }

    json.scalar_write(key, buf.get(), len);
  }
}

template <typename L, typename G> void write_string_value(
  L length_getter,
  G value_getter,
  libpff_item_t* item,
  uint32_t si,
  uint32_t etype,
  uint8_t flags,
  const std::string& key,
  JSON_writer& json)
{
  libpff_error_t* error = 0;
  size_t len;
  switch (length_getter(item, si, etype, &len, flags, &error)) {
  case -1:
    throw libpff_error(error, __LINE__);
  case  0:
    break;
  case  1:
    {
      boost::scoped_array<uint8_t> buf(new uint8_t[len]);
      if (value_getter(item, si, etype, buf.get(), len, flags, &error) != 1) {
        throw libpff_error(error, __LINE__);
      }

      json.scalar_write(key, (const char*) buf.get());
    }
    break;
  }
}

template <typename U, typename S, typename G> void write_numeric_value(
  G getter,
  libpff_item_t* item,
  uint32_t si,
  uint32_t etype,
  uint8_t flags,
  const std::string& key,
  JSON_writer& json)
{
  libpff_error_t* error = 0;
  U val;
  switch (getter(item, si, etype, &val, flags, &error)) {
  case -1:
    throw libpff_error(error, __LINE__);
  case  0:
    break;
  case  1:
    json.scalar_write(key, (S) val);
    break;
  }
}

void write_single_value(
  libpff_item_t* item,
  uint32_t si,
  uint32_t etype,
  uint32_t vtype,
  uint8_t flags,
  JSON_writer& json)
{
  const std::string key(entry_type_string(etype));

  switch (vtype) {
  case LIBPFF_VALUE_TYPE_UNSPECIFIED:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_NULL:
    json.null_write(key);
    break;
  case LIBPFF_VALUE_TYPE_INTEGER_16BIT_SIGNED:
    write_numeric_value<uint16_t, int16_t>(
      &libpff_item_get_entry_value_16bit,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_INTEGER_32BIT_SIGNED:
    write_numeric_value<uint32_t, int32_t>(
      &libpff_item_get_entry_value_32bit,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_FLOAT_32BIT:
  case LIBPFF_VALUE_TYPE_DOUBLE_64BIT:
    write_numeric_value<double, double>(
      &libpff_item_get_entry_value_floating_point,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_CURRENCY:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_APPLICATION_TIME:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_ERROR:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_BOOLEAN:
    write_numeric_value<uint8_t, bool>(
      &libpff_item_get_entry_value_boolean,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_OBJECT:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_INTEGER_64BIT_SIGNED:
    write_numeric_value<uint64_t, int64_t>(
      &libpff_item_get_entry_value_64bit,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_STRING_ASCII:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_STRING_UNICODE:
    write_string_value(
      &libpff_item_get_entry_value_utf8_string_size,
      &libpff_item_get_entry_value_utf8_string,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_FILETIME:
    write_numeric_value<uint64_t, uint64_t>(
      &libpff_item_get_entry_value_filetime,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_GUID:
    // FIXME: will this be a printable string?
    write_string_value(
      &libpff_item_get_entry_value_size,
      &libpff_item_get_entry_value_guid,
      item, si, etype, flags, key, json
    );
    break;
  case LIBPFF_VALUE_TYPE_SERVER_IDENTIFIER:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_RESTRICTION:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_RULE_ACTION:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_BINARY_DATA:
    write_binary_value(
      &libpff_item_get_entry_value_binary_data_size,
      &libpff_item_get_entry_value_binary_data,
      item, si, etype, flags, key, json
    );
    break;
  }
}

void write_multi_value(
  libpff_item_t* item,
  uint32_t si,
  uint32_t etype,
  uint32_t vtype,
  uint8_t flags,
  JSON_writer& json)
{
// FIXME: This is way broken.
// FIXME: include value index in error messages

  libpff_error_t* error = 0;

  const std::string key(entry_type_string(etype));

  MultiValuePtr mvp(get_multivalue(item, si, etype, flags), &destroy_multivalue);
  if (!mvp) {
    throw libpff_error("unable to retrieve multi-value entry", __LINE__);
  }

  libpff_multi_value_t* mv = mvp.get();

  int vcount;
  if (libpff_multi_value_get_number_of_values(mv, &vcount, &error) == -1) {
    throw libpff_error(error, __LINE__);
  }

  json.array_member_open(key);

  switch (vtype) {
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_INTEGER_16BIT_SIGNED:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_INTEGER_32BIT_SIGNED:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_FLOAT_32BIT:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_DOUBLE_64BIT:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_CURRENCY:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_APPLICATION_TIME:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_INTEGER_64BIT_SIGNED:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_STRING_ASCII:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_STRING_UNICODE:
    for (int i = 0; i < vcount; ++i) {
      size_t len;
      switch (libpff_multi_value_get_value_utf8_string_size(mv, i, &len, &error)) {
      case -1:
        throw libpff_error(error, __LINE__);
      case  0:
        break;
      case  1:
        {
          boost::scoped_array<uint8_t> buf(new uint8_t[len]);
          if (libpff_multi_value_get_value_utf8_string(mv, i, buf.get(), len, &error) != 1) {
            throw libpff_error(error, __LINE__);
          }

          json.scalar_write(key, (const char*) buf.get());
        }
        break;
      }
    }
    break;
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_FILETIME:
    for (int i = 0; i < vcount; ++i) {
      uint64_t ts;
      switch (libpff_multi_value_get_value_filetime(mv, i, &ts, &error)) {
      case -1:
        throw libpff_error(error, __LINE__);
      case  0:
        break;
      case  1:
        json.scalar_write(key, ts);
        break;
      }
    }
    break;
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_GUID:
    throw libpff_error(key, __LINE__);
  case LIBPFF_VALUE_TYPE_MULTI_VALUE_BINARY_DATA:
    throw libpff_error(key, __LINE__);
  }

  json.array_member_close();
}

void write_entry(
  libpff_item_t* item,
  uint32_t si,
  uint32_t etype,
  uint32_t vtype,
  uint8_t flags,
  JSON_writer& json)
{
  if (vtype & LIBPFF_VALUE_TYPE_MULTI_VALUE_FLAG) {
//    write_multi_value(item, si, etype, vtype, flags, json);
    write_multi_value(item, si, etype, vtype, flags | LIBPFF_ENTRY_VALUE_FLAG_IGNORE_NAME_TO_ID_MAP, json);
  }
  else {
    write_single_value(item, si, etype, vtype, flags, json);
  }
}

void handle_item_value(libpff_item_t* item, uint32_t s, uint32_t e, const std::string& path, JSON_writer& json) {
  libpff_error_t* error = 0;

  uint32_t etype;
  uint32_t vtype;
  libpff_name_to_id_map_entry_t* nkey = 0;

  if (libpff_item_get_entry_type(item, s, e, &etype, &vtype, &nkey, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  json.scalar_write("entry type", etype);
  json.scalar_write("value type", vtype);

  try {
    if (nkey) {
      uint8_t ntype;
      if (libpff_name_to_id_map_entry_get_type(nkey, &ntype, &error) != 1) {
        throw libpff_error(error, __LINE__);
      }

      if (ntype == LIBPFF_NAME_TO_ID_MAP_ENTRY_TYPE_NUMERIC) {
        uint32_t nnum;
        if (libpff_name_to_id_map_entry_get_number(nkey, &nnum, &error) != 1) {
          throw libpff_error(error, __LINE__);
        }

// TODO: what is this?
        json.scalar_write("maps to entry type", nnum);
      }
      else if (ntype == LIBPFF_NAME_TO_ID_MAP_ENTRY_TYPE_STRING) {
        size_t len;
        if (libpff_name_to_id_map_entry_get_utf8_string_size(nkey, &len, &error) != 1) {
          throw libpff_error(error, __LINE__);
        }

        boost::scoped_array<uint8_t> buf(new uint8_t[len]);
        if (libpff_name_to_id_map_entry_get_utf8_string(nkey, buf.get(), len, &error) != 1) {
          throw libpff_error(error, __LINE__);
        }

// TODO: what is this?
        json.scalar_write("maps to entry", (const char*) buf.get());
      }
    }
  }
  catch (const libpff_error& ex) {
    std::cerr << "Error: " << path << '[' << s << "][" << e << "]: "
              << ex.what() << std::endl;
  }

  uint32_t matched_vtype = LIBPFF_VALUE_TYPE_UNSPECIFIED;
  uint8_t* vdata = 0;
  size_t len;

  if (libpff_item_get_entry_value(
    item, s, etype, &matched_vtype, &vdata, &len,
    LIBPFF_ENTRY_VALUE_FLAG_MATCH_ANY_VALUE_TYPE |
    LIBPFF_ENTRY_VALUE_FLAG_IGNORE_NAME_TO_ID_MAP, &error) != 1)
  {
    throw libpff_error(error, __LINE__);
  }

  if (vtype != matched_vtype) {
    // the matched type is only interesting in case of a mismatch
    // FIMXE: maybe this should print an error?
    json.scalar_write("matched value type", matched_vtype);
  }

  try {
    write_entry(item, s, etype, vtype, 0, json);
  }
  catch (const libpff_error& ex) {
    std::cerr << "Error: " << path << '[' << s << "][" << e << "]: "
              << ex.what() << std::endl;
  }
}

void handle_item_values(libpff_item_t* item, const std::string& path, JSON_writer& json) {
  libpff_error_t* error = 0;

  // number of sets
  uint32_t sets;
  if (libpff_item_get_number_of_sets(item, &sets, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  json.scalar_write("number of sets", sets);

  // number of entries per set
  uint32_t entries;
  if (libpff_item_get_number_of_entries(item, &entries, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }

  json.scalar_write("entries per set", entries);

  if (sets > 0) {
    json.array_member_open("sets");

    // iterate over sets
    for (uint32_t s = 0; s < sets; ++s) {
      json.array_open();

      // iterate over entries
      for (uint32_t e = 0; e < entries; ++e) {
        json.object_open();

        try {
          handle_item_value(item, s, e, path, json);
        }
        catch (const libpff_error& ex) {
          std::cerr << "Error: " << path << '[' << s << "][" << e << "]: "
                    << ex.what() << std::endl;
        }

        json.object_close();
      }

      json.array_close();
    }

    json.array_member_close();
  }
}

template <typename C, typename G> void handle_items_loop(C item_count_getter, G item_getter, const std::string& path, JSON_writer& json) {
  try {
    libpff_error_t* error = 0;

    int num;
    if (item_count_getter(&num, &error) != 1) {
      throw libpff_error(error, __LINE__);
    }

    if (num > 0) {
      for (int i = 0; i < num; ++i) {
        std::string cpath(path + '/' + i);
        try {
          ItemPtr itemp(item_getter(i), &destroy_item);
          handle_item(itemp.get(), cpath, json);
        }
        catch (const libpff_error& e) {
          std::cerr << "Error: " << cpath << ": " << e.what() << std::endl;
        }
      }
    }
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << path << ": " << e.what() << std::endl;
  }
}

void handle_subitems(libpff_item_t* item, const std::string& path, JSON_writer& json) {
  handle_items_loop(
    boost::bind(&libpff_item_get_number_of_sub_items, item, _1, _2),
    boost::bind(&get_child, item, _1),
    path, json
  );
}

void handle_unknowns(libpff_item_t* folder, const std::string& path, JSON_writer& json) {
  // TODO: These are known unknowns, in the Rumsfeldian sense.
  // I.e., we know that we have no idea wtf these are.
  try {
    ItemPtr unknownsp(get_unknowns(folder), &destroy_item);
    if (unknownsp) {
      handle_item(unknownsp.get(), path + "/unknowns", json);
    }
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << path << ": " << e.what() << std::endl;
  }
}

template <typename T, typename G> T get_attrib(G getter) {
  libpff_error_t* error = 0;

  T value;
  if (getter(&value, &error) != 1) {
    throw libpff_error(error, __LINE__);
  }
  return value;
}

template <typename T, typename G> void write_attrib(G getter, const std::string& key, const std::string& path, JSON_writer& json) {
  try {
    T value = get_attrib<T>(getter);
    json.scalar_write(key, value);
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << path << ':' << key << ": "
                           << e.what() << std::endl;
  }
}

void handle_item(libpff_item_t* item, const std::string& path, JSON_writer& json) {
  json.object_open();

  // path
  json.scalar_write("path", path);

  // item type
  uint8_t itype = LIBPFF_ITEM_TYPE_UNDEFINED;
  try {
    itype = get_attrib<uint8_t>(
      boost::bind(&libpff_item_get_type, item, _1, _2)
    );
    json.scalar_write("item type", (uint32_t) itype);
    json.scalar_write("item type name", item_type_string(itype));
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << path << ": " << e.what() << std::endl;
  }

  // identifier
  write_attrib<uint32_t>(
    boost::bind(&libpff_item_get_identifier, item, _1, _2),
    "identifier", path, json
  );

  // item values
  try {
    handle_item_values(item, path, json);
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << path << ": " << e.what() << std::endl;
  }

  json.object_close();
  json.reset();

  // process children
  handle_subitems(item, path, json);

  // process unknowns, for folders only
  if (itype == LIBPFF_ITEM_TYPE_FOLDER) {
    handle_unknowns(item, path, json);
  }
}

void handle_tree(libpff_file_t* file, const std::string& filename, JSON_writer& json) {
  ItemPtr rootp(get_root(file), &destroy_item);
  handle_subitems(rootp.get(), '/' + filename, json);
}

void handle_orphans(libpff_file_t* file, const std::string& filename, JSON_writer& json) {
  handle_items_loop(
    boost::bind(&libpff_file_get_number_of_orphan_items, file, _1, _2),
    boost::bind(&get_orphan, file, _1),
    '/' + filename + "/orphans", json
  );
}

void handle_recovered(libpff_file_t* file, const std::string& filename, JSON_writer& json) {
  std::string path( '/' + filename + "/recovered");

  try {
/*
  TODO: Not sure we're using this correctly.

  TODO: which flags to use?
    LIBPFF_RECOVERY_FLAG_IGNORE_ALLOCATION_DATA
    LIBPFF_RECOVERY_FLAG_SCAN_FOR_FRAGMENTS
*/
    libpff_error_t* error = 0;
    if (libpff_file_recover_items(file, 0, &error) == -1) {
      throw libpff_error(error, __LINE__);
    }

    handle_items_loop(
      boost::bind(&libpff_file_get_number_of_recovered_items, file, _1, _2),
      boost::bind(&get_recovered, file, _1),
      path, json
    );
  }
  catch (const libpff_error& e) {
    std::cerr << "Error: " << path << ": " << e.what() << std::endl;
  }
}

int main(int argc, char** argv) {
  try {
    if (argc != 2) {
      throw std::runtime_error("wrong number of arguments");
    }

    // setup
    FilePtr filep(create_file(argv[1]), &destroy_file);
    libpff_file_t* file = filep.get();

    std::string filename(std::max(strchr(argv[1], '/') + 1, argv[1]));

    // process the file
    JSON_writer json(std::cout);

    handle_tree(file, filename, json);
    handle_orphans(file, filename, json);
    handle_recovered(file, filename, json);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
