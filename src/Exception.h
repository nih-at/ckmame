

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <filesystem>
#include <string>

class Exception : public std::exception {
public:
    Exception(const std::string &message_) : message(message_) { }
    Exception(const char *format, ...) __attribute__ ((format (printf, 2, 3)));

    Exception append_detail(const std::string &str);
    Exception append_system_error(int code = -1); // default: use current errno
    Exception append_filesystem_error(const std::error_code &code);

    virtual const char* what() const throw();

private:
    std::string message;
};

#endif // EXCEPTION_H
