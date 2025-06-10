#ifndef APPEXCEPTION_H
#define APPEXCEPTION_H

#include <stdexcept>
#include <string>

class AppException : public std::runtime_error {
public:
    std::string className;
    std::string fileName;
    int lineNumber;

    AppException(const std::string& message,
                 const std::string& cls,
                 const std::string& file,
                 int line)
        : std::runtime_error(message), className(cls), fileName(file), lineNumber(line) {}

    std::string fullMessage() const {
        return "Error in class: " + className + ", file: " + fileName +
               ", line: " + std::to_string(lineNumber) + " => " + what();
    }
};

#endif // APPEXCEPTION_H
