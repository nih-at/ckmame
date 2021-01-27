
#include "Exception.h"

#include "util.h"

Exception::Exception(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    message = string_format_v(format, ap);
    va_end(ap);
}

Exception Exception::append_detail(const std::string &str) {
    message += ": " + str;
    
    return *this;
}

Exception Exception::append_system_error(int code) {
    if (code == -1) {
        code = errno;
    }
    
    return append_detail(strerror(code));
}


const char *Exception::what() const throw() {
    return message.c_str();
}
