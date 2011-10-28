
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <libpff.h>


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

libpff_item_t* create_root(libpff_file_t* file) {
  libpff_item_t* root = 0;
  libpff_error_t* error = 0;

  if (libpff_file_get_root_item(file, &root, &error) != 1) {
    throw libpff_error(error);
  }
  
  return root;
}

void destroy_item(libpff_item_t* item) {
  libpff_error_t* error = 0;

  if (libpff_item_free(&item, &error) != 1) {
    throw libpff_error(error);
  }
}

int main(int argc, char** argv) {
  try {
    if (argc != 2) {
      throw std::runtime_error("wrong number of arguments");
    }

    // setup
    libpff_error_t* error = 0;

    boost::shared_ptr<libpff_file_t> filep(create_file(argv[1]), &destroy_file);
    libpff_file_t* file = filep.get();

    // find the root
    boost::shared_ptr<libpff_item_t> rootp(create_root(file), &destroy_item);
    libpff_item_t* root = rootp.get();

    int childcount;
    if (libpff_item_get_number_of_sub_items(root, &childcount, &error) != 1) {
      throw libpff_error(error);
    }

    std::cout << childcount << std::endl;

  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
