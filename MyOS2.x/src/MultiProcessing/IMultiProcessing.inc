#ifdef INCLUDE_PHASE
	#include "Exceptions/MyOSExceptions.h"
	
namespace MyOS { namespace MultiProcessing {	
	using MyOS::Exceptions::OutOfMemoryException;
	
	class Process;
}}
	
#else

	virtual Process& createProcess( u32 allocBlockOrder ) 
		throw (OutOfMemoryException) = 0;

#endif
