// ExceptionMacros.h
#ifndef EXCEPTION_MACROS_H
#define EXCEPTION_MACROS_H

#include "core/Debug/AppException.h"

#define THROW_APP_EXCEPTION(msg, cls) throw AppException(msg, cls, __FILE__, __LINE__)

#endif // EXCEPTION_MACROS_H
