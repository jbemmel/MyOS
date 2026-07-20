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
namespace MyOS { 

namespace Core { class IInterface; }

// @todo get rid of this namespace
namespace Exceptions {

/* Thrown when abort() is called, this causes the current thread to die
class AbortException {
    
public:
    inline AbortException() throw() {}
};*/

class CastException 
{
    const Core::IInterface& casted;
    const char* const castedToUUID;
    
public:
    inline CastException( const Core::IInterface& _casted, const char* const uuid ) throw() 
        : casted(_casted), castedToUUID(uuid) {}
};

/* Thrown when IThread::exit() is called
class ThreadExitException {
   int exitcode;
public:
   ThreadExitException( int ec ) : exitcode(ec) {}
   int getExitCode() const { return exitcode; }
};*/   
   
}} // namespace

#endif
