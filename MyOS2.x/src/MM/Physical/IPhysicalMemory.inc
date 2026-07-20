#ifdef INCLUDE_PHASE

#include "myosresult.h"
#include "../memtypes.h"

#else

/**
 * Internal interface for physical memory allocation (paging and other things)
 */
public:
   // Allocates physical frames and maps them to the specified linear range
   virtual physadr_t mapRegion( linadr_t startaddr, size_t order, pteflags_t atts ) = 0;  
   virtual void releaseRegion( linadr_t startaddr, size_t order ) = 0;     

   virtual u32 getFreeFrames() const = 0;   
   virtual physadr_t getPhysicalAddress( linadr_t ofAddr ) const = 0;

   virtual void mapMMIO( linadr_t linear, physadr_t physicalStart, u32 pageCount ) = 0;
   virtual void unmapMMIO( linadr_t linear, u32 pageCount ) = 0;

   virtual physadr_t mapDMA( linadr_t linear, u32 pageOrder ) = 0;
   virtual void unmapDMA( linadr_t linear, u32 pageOrder ) = 0;

   virtual void setAttributes( linadr_t forAdr, size_t pages, pteflags_t flags ) const = 0;

   // 'createAlias' better ?
   virtual void copyMapping( linadr_t dest, linadr_t src, size_t pagecount, pteflags_t atts ) = 0;

#endif
