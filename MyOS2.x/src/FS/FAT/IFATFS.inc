#ifdef INCLUDE_PHASE

#include "FS/IMountedFS.h"

#else

// @return a handle to a mounted FS instance, to be unmounted when done
virtual IMountedFS& mount( Devices::IBlockDevice& toDevice ) 
	throw (FSException) = 0;

#endif
