/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause
 * one of the tests to be selected at all times
 */
#include "IContext.h"
#include "debug.h"
#include "../TestComponent.h"

namespace MyOS { namespace Tests {

struct SomeException {
    int value;

    inline SomeException( int i ) throw() : value(i) {}
};

static int exceptionThrower() throw (SomeException) __attribute__ ((noinline));
static bool doThrowTest() __attribute__ ((noinline));
static bool doThrowUnknownTest() __attribute__ ((noinline));

static bool doThrowTest() {
    try {
       exceptionThrower();
    } catch (SomeException &e) {
       PRINTK( "\n#2f#Exception caught, value=%d", e.value );
       ASSERTION( e.value == 1234, E_ERROR );
       throw e;
   } catch (...) {
       PRINTK( "\n#2f#Some exception caught" );
   }
}

static bool doThrowUnknownTest() {
    try {
       doThrowTest();
    } /* catch ( SomeException &e ) {
       PRINTK( "\n#2f#Some exception re-caught" );
       throw e;
    } */ catch ( ... ) {
       PRINTK( "\n#2f#Some exception re-caught" );
       throw;
    }
}

/**
 * A test to ensure that Thread::usSleep( time ) actually sleeps for <time> us
 *
 */
bool doTest( IContext& context, NVPAIR params[] )
{
   PRINTK( "\nStarting exception test..." );

   try {
       doThrowUnknownTest();
   } catch ( SomeException &e ) {
       PRINTK( "\n#2f#Final re-caught, testing NPE exceptions..." );

       // Test PageFaults
       u32 *mem = 0;
       *mem = 1234;

   } catch (...) {
       PRINTK( "\n#2f#Final unknown re-caught" );
   }
   return false;
}

int exceptionThrower() throw (SomeException) {
    PRINTK( "\nexceptionThrower" );
    throw SomeException(1234);
}

}}
