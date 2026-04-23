// =============================================================================
// FILE:        ExceptionMacros.h
// MODULE:      Exception Handling Macros
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines convenience macros for throwing AppException with
//              automatic file name and line number capture.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef EXCEPTION_MACROS_H
#define EXCEPTION_MACROS_H

#include "core/Debug/AppException.h"

// =============================================================================
// MACRO: THROW_APP_EXCEPTION
// DESCRIPTION: Throws an AppException with the given message and class name,
//              automatically inserting __FILE__ and __LINE__.
// =============================================================================
#define THROW_APP_EXCEPTION(msg, cls) throw AppException(msg, cls, __FILE__, __LINE__)

#endif // EXCEPTION_MACROS_H
