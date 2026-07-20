#ifdef INCLUDE_PHASE

#include "Exceptions/MyOSExceptions.h"
#include "NVPAIR.h"

namespace MyOS { namespace MultiThreading {

class Thread;

class IRunnable {
public:
	virtual int run( NVPAIR params[] ) = 0;
};

}}	// namespace

#else

virtual Thread& createThread( u32 stackorder, u32 allocOrder=0 ) 
	throw (Exceptions::OutOfMemoryException) = 0;

#endif
