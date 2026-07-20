#include "defs.h"
#include "debug.h"

#include "std.h"

// GCC 3.0.1 defines __cxa_pure_virtual, earlier versions used '__pure_virtual'
extern "C" EXPORT void __cxa_pure_virtual() NORETURN;

// Used for cleanup of static variables
extern "C" EXPORT int atexit(void (*function)(void));

extern "C" EXPORT void abort();

// GCC 4.0 needs this
void __cxa_pure_virtual() {
    BUG("__cxa_pure_virtual called");
    while(1);
}

/**
 * Called to register functions to execute at exit, ignored
 */
int atexit( void (*function)(void) ) {
    return -1;
}

void abort()
{
    BUG( "abort() called" );
    while(1);
}

namespace std {

    void terminate() {
        BUG( "terminate() called" );
    }

    void unexpected() {
        BUG( "unexpected() called" );
    }
}

namespace __cxxabiv1 {

void
__unexpected (std::unexpected_handler handler)
{
  handler();
  std::terminate ();
}

} // namespace
