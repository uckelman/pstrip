
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

libpff_file_t* create_file() {
  libpff_file_t* file = 0;
  libpff_error_t* error = 0;

  if (libpff_file_initialize(&file, &error) != 1) {
    throw libpff_error(error);
  }
  
  return file;
}

void destroy_file(libpff_file_t* file) {
  libpff_error_t* error = 0;
  if (libpff_file_free(&file, &error) != 1) {
    throw libpff_error(error);
  }
}

int main(int argc, char** argv) {
  try {
    if (argc != 2) {
      throw std::runtime_error("wrong number of arguments");
    }

    libpff_error_t* error = 0;

    boost::shared_ptr<libpff_file_t> file(create_file(), &destroy_file);

    if (libpff_file_open(file.get(), argv[1], LIBPFF_OPEN_READ, &error) != 1) {
      throw libpff_error(error);
    }

// do something here

    if (libpff_file_close(file.get(), &error) != 1) {
      throw libpff_error(error);
    }
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
