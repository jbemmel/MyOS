#ifndef FRAMEPOOLMANAGER_H
#define FRAMEPOOLMANAGER_H

#include "myosresult.h"
#include "PTE.h"
#include "DRINGLIST.h"
#include "FramePool.h"
#include "PageDirectory.h"

#include "Exceptions/MyOSExceptions.h"

namespace MyOS {

namespace IO {
    class OStream;     // for XML printing
}

namespace MemoryManagement {

class Page;

using MyOS::Exceptions::OutOfMemoryException;

/**
*   Singleton, manages all pools of 1024 frames each
*
*   Typically this class receives an allocation request and forwards
*   it to one of the pools
* 
*  TODO: Separate management of physical vs linear memory
*        The 'kernelpages' bitmap is e.g. not for physical!
*/
class FramePoolManager
{
   /// Doubly-linked list of all pools
   DRINGLIST< FramePool > poolring;

   /// bitmap of available kernel pages @ 0xFF800000 - 0xFFC00000
   AtomicBitArray< 1024 > kernelpages;

   /// Total amount of available physical memory, in bytes
   size_t totalPhysicalMemory;

   /// Determines available physical memory during system initialization
   physadr_t testMemory() INITSECTION;

public:
   FramePoolManager() INITSECTION;
   void initMemory() INITSECTION;

   /// Returns the number of free kernel frames (linear page addresses)
   inline u32 getFreeKernelFrames() const { return kernelpages.getFree(); }

   /// Returns the total available physical memory (in bytes)
   inline size_t getPhysMemorySize() const { return totalPhysicalMemory; }

   /// Returns the number of physical pages free
   u32 getFreePages() const;

   // Same as for FramePool, this class selects an available pool and redirects
   physadr_t allocateFrame();
   physadr_t allocateFrameRange( u8 count /* max 32, for 32*4K = 128K memory */ );
   inline void releaseFrame( physadr_t pte );
   inline void releaseFrameRange( physadr_t pteStart, u8 count );

   // kernel linear memory
   Page* kallocatePage( pteflags_t flags = E_PAGE_KERNEL ) 
      throw (OutOfMemoryException);
      
   Page* kallocateRange( size_t bytes, pteflags_t flags = E_PAGE_KERNEL );
   Page* mapMMIO( physadr_t start, size_t bytes, pteflags_t flags = E_PAGE_MMIO );
   Page* kallocate1MBPages( size_t bytes );		// will round to next page boundary

   void kfreePage( Page* page );
   void kfreeRange( Page* start, size_t bytes );
   void unmapMMIO( Page* start, size_t bytes );
   void kfree1MBPages( Page* start, size_t bytes );

   /// Generates an XML listing to output
   myos_result_t listTables( IO::OStream &output ) const;
   
   // Creates a stack with a guard page (reserved linear space without frame)
   linadr_t createStack( size_t pages /*=1*/ );
   void freeStack( linadr_t stack, size_t pages );
};

inline void
FramePoolManager::releaseFrame( physadr_t pte )
{
    // NOTE: concurrent with allocateFrame...
    FramePool::getPoolForFrame(pte).releaseFrame( pte );
}

inline void
    FramePoolManager::releaseFrameRange( physadr_t pte, u8 count )
{
    // NOTE: concurrent with allocateFrame...
    FramePool::getPoolForFrame(pte).releaseFrameRange( pte, count );
}

}}	// namespace

#endif
