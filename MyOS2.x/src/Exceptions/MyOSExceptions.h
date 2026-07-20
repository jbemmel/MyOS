#ifndef MYOSEXCEPTIONS_H
#define MYOSEXCEPTIONS_H

#ifndef __GNUC__
// Visual Studio does not support exception specifications
#pragma warning(disable:4290)
#endif

// #include "debug.h"   // FILELINE

/**
 * Note: There is not much point to use global static exceptions to throw,
 * since '__cxa_allocate_exception' is called anyway...
 */
namespace MyOS { namespace Exceptions {

class OutOfMemoryException {
   const char* location;   
public:
   inline OutOfMemoryException( const char* l ) : location(l) {}
   const char* getLocation() const { return location; }
};

#define OUTOFMEMORY OutOfMemoryException( __FILE__ )

/// Thrown when IThread::exit() is called
class ThreadExitException {
   int exitcode;
public:
   ThreadExitException( int ec ) : exitcode(ec) {}
   int getExitCode() const { return exitcode; }
};   
   
}} // namespace

#endif
