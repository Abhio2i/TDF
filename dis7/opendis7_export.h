#pragma once

#if defined(_WIN32)
  #ifdef OPENDIS7_BUILD
    #define OPENDIS7_EXPORT __declspec(dllexport)
  #else
    #define OPENDIS7_EXPORT __declspec(dllimport)
  #endif
#else
  #define OPENDIS7_EXPORT
#endif
