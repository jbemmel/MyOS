#ifndef IPHYSICALMEMORYIMPL_H
#define IPHYSICALMEMORYIMPL_H

#include "MM/IPhysicalMemory.h"

#define BBA_FIXED_MEM
//#define OLD_FREE
#include "MM/BitBuddyAllocator.hpp"
#undef BBA_FIXED_MEM

/**
 * Implementation for physical MM
 */
namespace MyOS { namespace MM { 

class IPhysicalMemoryImpl : public IPhysicalMemory 
{      
public:
   IPhysicalMemoryImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   inline void deinit( Context::IContext& context ) {}

   virtual physadr_t mapRegion( linadr_t startaddr, size_t order, pteflags_t atts )
       throw (MM::OutOfMemoryException);
   
   virtual void releaseRegion( linadr_t startaddr, size_t order );

   virtual u32 getFreeFrames() const;
   virtual physadr_t getPhysicalAddress( linadr_t ofAddr ) const;
   virtual pteflags_t getAttributes( linadr_t ofAddr ) const;

   virtual void mapMMIO( linadr_t linear, physadr_t physicalStart, u32 pageOrder );
   virtual void unmapMMIO( linadr_t linear, u32 pageOrder );

   virtual physadr_t mapDMA( linadr_t linear, u32 pageOrder );
   virtual void unmapDMA( linadr_t linear, u32 pageOrder );

   virtual void setAttributes( linadr_t forAdr, size_t pages, pteflags_t flags ) const;
      
   virtual void copyMapping( linadr_t dest, linadr_t src, size_t pagecount, pteflags_t atts );
   virtual void clearMapping( linadr_t dest, size_t pagecount );

   // XML
   virtual Core::IScriptable* getScriptable() const throw();
   virtual  myos_result_t listTables( IO::OStream& out ) const;
   virtual  myos_result_t getStatistics( IO::OStream& out ) const;

   // Called after system init has finished
   void freeInitPages();
   
private:

   // PBT, PageDirectory can call this method too
   friend class PBT; 

   inline physadr_t allocateFrame() {
      //Locking::LockHelper lock( memlock );
      return pages.allocate(0); 
   }
   inline void releaseFrame( physadr_t frame ) {
      //Locking::LockHelper lock( memlock );      
      pages.free(frame,0); 
   }

   u32 totalPhysicalMemory;

   // Separate memory for below 1Mb pages, for (floppy) DMA
   u32 dmaMemory512Kb;
      
   unsigned filler[2];
   
   // Max. physical mem supported 1Gb, mapped to 0xff400000 linear
   typedef BitBuddyAllocator<30,12,0xFF400000> ALLOCATOR;
   ALLOCATOR pages;  // should be aligned to 32 bytes!
};
  
}} // namespace

#endif
