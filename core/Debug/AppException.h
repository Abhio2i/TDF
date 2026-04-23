// =============================================================================
// FILE:        AppException.h
// MODULE:      Exception Handling
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the AppException class, a custom exception type derived
//              from std::runtime_error. Captures additional context including
//              class name, file name, and line number where the exception
//              occurred, providing a full error message for debugging.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef APPEXCEPTION_H
#define APPEXCEPTION_H

#include <stdexcept>
#include <string>

// =============================================================================
// CLASS: AppException
//
// DESCRIPTION: Enhanced runtime exception that stores the originating class,
//              file, and line number. Provides a fullMessage() method that
//              formats all diagnostic information into a single string.
// =============================================================================
class AppException : public std::runtime_error {
public:
    // =========================================================================
    // SECTION: Exception Context
    // DESCRIPTION: Metadata about where the exception was thrown.
    // =========================================================================
    std::string className;   //!< Name of the class where exception occurred
    std::string fileName;    //!< Source file name
    int lineNumber;          //!< Line number in the source file

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
