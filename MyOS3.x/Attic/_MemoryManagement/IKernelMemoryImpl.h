#ifndef IKERNELMEMORYIMPL_H
#define IKERNELMEMORYIMPL_H

#include "MemoryManagement/IKernelMemory.h"

#include "FramePoolManager.h"
#include "BlockAllocator.h"

namespace MyOS { namespace MemoryManagement {

class IKernelMemoryImpl : public IKernelMemory {
  
public:
   IKernelMemoryImpl( MyOS::Core::IComponent& c ) INITSECTION;
   bool init( IContext& c, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& c );

   // native functions from .inc
   virtual linadr_t allocate( MyOS::Core::IComponent &requestor, size_t bytes, kmflags_t flags )
      throw (OutOfMemoryException);
   virtual void free( linadr_t address, size_t bytes, kmflags_t flags );
   virtual linadr_t mapMMIO( physadr_t physicalStart, size_t bytes, linadr_t hint );
   virtual void releaseMMIO( linadr_t linstart, size_t bytes );
   virtual physadr_t getMapping( const linadr_t forAdr ) const;
   virtual pteflags_t getAttributes( const linadr_t forAdr ) const;
   virtual void setAttributes( linadr_t address, size_t bytes, pteflags_t newatts ) const;
   virtual void mapRegion( linadr_t dest, linadr_t src, size_t pagecount, pteflags_t atts );
   virtual void unmapRegion( linadr_t dest, size_t pagecount );
   virtual linadr_t getBIOSPage();

   // TODO: Redesign virtual / physical memory interfaces
   virtual linadr_t createStack( size_t pages /*=1*/ );
   virtual void freeStack( linadr_t stack, size_t pages );
   
   virtual myos_result_t get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const;
   virtual myos_result_t put( const char* const call, NVPAIR parameters[], IO::IStream&  input );

   virtual  myos_result_t listTables( IO::OStream& ) const;
   virtual  myos_result_t getStatistics( IO::OStream& ) const;

   inline FramePoolManager& getFPM() { return theFramePoolManager; }

private:
   FramePoolManager theFramePoolManager;

   /// An allocator for normal kernel memory
   BlockAllocator generalAllocator;

   /// An allocator especially for DMA memory (ethernet packets etc)
   BlockAllocator dmaAllocator;

   // BIOS page 0, mapped on demand
   linadr_t biosPage0;

   // Some statistics
   u32 bytesAllocated, bytesFreed, nAllocations, nFrees;

   // Core memory usage, to detect leaks
   u32 recordedUsage;   
};
   
}} // namespaces
#endif
