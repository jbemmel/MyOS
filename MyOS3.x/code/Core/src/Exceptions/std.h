#ifndef STD_H
#define STD_H

#include "defs.h"

namespace std {

    // No return function
    EXPORT void terminate() NORETURN;
    EXPORT void unexpected();

    /// If you write a replacement %terminate handler, it must be of this type.
    typedef void (*terminate_handler) ();

    typedef void (*unexpected_handler) ();

    bool uncaught_exception() throw();

    class type_info;
}

#endif
