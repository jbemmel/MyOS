#ifdef INCLUDE_PHASE
#include "../memtypes.h"
#include "Exceptions/MyOSExceptions.h"

using MyOS::Exceptions::OutOfMemoryException;

#else
/**
 * Allocates a region of virtual memory, and physical memory to back it up
 */
virtual linadr_t allocate( size_t order ) throw (OutOfMemoryException) = 0;
virtual linadr_t allocateNoThrow( size_t order ) = 0;

virtual void free( linadr_t vmem, size_t order ) = 0;

/** one page versions, optimized for this common case. 
    Note that 'mapping' currently does not include PTE bits */
virtual linadr_t allocPage( physadr_t& mapping ) throw (OutOfMemoryException) = 0;
virtual linadr_t allocPageNoThrow( physadr_t& mapping ) = 0;
virtual void freePage( linadr_t vmem ) = 0;

/**
 * Specialized functions
 */
 
/**
 * Like allocate, but allocates an unmapped 'guard page' as a first page
 * to catch stack overflows
 */ 
virtual linadr_t allocateStack( size_t order ) throw (OutOfMemoryException) = 0;
virtual void freeStack( linadr_t stack, size_t order ) = 0;

virtual linadr_t mapMMIO( physadr_t physicalStart, u32 byteCount ) = 0;
virtual void unmapMMIO( linadr_t adr, u32 byteCount ) = 0;

/**
 * Allocates ISA-DMA compatible memory, i.e. below 1Mb and not crossing 64Kb alignment
 */
virtual linadr_t allocISADMA( u32 pageOrder, physadr_t &physAdr ) = 0;
virtual void freeISADMA( linadr_t mem, u32 pageOrder ) = 0;

#endif
