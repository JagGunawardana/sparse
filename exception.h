#ifndef _exception_h
#define _exception_h

#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/get_error_info.hpp>

struct MatrixException: virtual std::exception, virtual boost::exception {};

typedef boost::error_info<struct tag_errno_code, std::string> error_string;
typedef boost::error_info<struct tag_errno_code, int> error_code;

#define ALL_OK 0x00
// Error codes

#endif

