#include "../IVirtualMemoryImpl.h"
#include "MM/MMComponent.h"
#include "Init/memlayout.h"
#include "GDT.h"
#include "MM/Physical/Page.h" // isZero

#include "debug.h"

namespace MyOS { namespace MM {

#define PHYS MMComponent::getInstance().impl2

IVirtualMemoryImpl::IVirtualMemoryImpl( MyOS::Core::IComponent& c )
: IVirtualMemory( c, VERSION(1,0) )
{

}

bool 
IVirtualMemoryImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // switch to own GDT
   GDT::initialize(); 
 
   // determine end-of-mapped-memory marker (round up to end of page)
   u32 endOfUsedMemory = (u32) &_FREE_KERNEL_SPACE_START_ - 1;
   size_t firstPageIndex = getPDTIndex(&_COMPRESSED_START_LINEAR_);
   size_t lastPageIndex = getPDTIndex( (void*) endOfUsedMemory);

   // mark all used kernel pages (linear frames, physical done in FramePool)
   // NOTE: This frees UNCOMPRESSED pages, and also marks the stack page
   for (u32 i=firstPageIndex;i<=lastPageIndex; ++i ) {
      kernelpages.markUsed( i );
   }
   // DPRINTK( "\nmarked %d kernelpages", lastPageIndex - firstPageIndex + 1 ); 

   // show initial state for debug
   // MyOS::Devices::LPT::LPTStream printer;
   // this->listPools( printer );
   // this->listTables( printer );
   
   // Clean up some garbage...
   // memset_aligned( (void*) 0xFF800000, 0, (u32) &_COMPRESSED_START_LINEAR_ - 0xFF800000 );   
   // DPRINTK( "\nClean size=%x", (u32) &_COMPRESSED_START_LINEAR_ - 0xFF800000 );

	return true;
}

void 
IVirtualMemoryImpl::deinit( IContext& context )
{

}

// virtual 
myos_result_t
IVirtualMemoryImpl::getStatistics( IO::OStream& out ) const {
    out.printf( "\nNumber of free kernel pages: %d", kernelpages.getFree() );  
    return E_MYOS_SUCCESS;
}

// virtual 
linadr_t 
IVirtualMemoryImpl::allocateNoThrow( size_t order ) 
{
   ASSERTION( order <= 5, E_ERROR ); // implementation limit 
 
   u32 startindex = kernelpages.getMulti( 1UL << order );
   if (startindex != kernelpages.E_FULL) {
      linadr_t a = (linadr_t) ((startindex << 12) + 0xFF800000);
      if (PHYS.mapRegion( a, order, E_PAGE_KERNEL )) {
         // ASSERTION( ((Page*)a)->isZero(), E_ERROR );
         
         // to see if it matters
         // memset_aligned( a, 0, npages*_4KB );
         
         return a;         
      } else {
         kernelpages.releaseMulti( startindex, 1UL << order );     
      }
   }
   return 0;
}

// virtual 
linadr_t 
IVirtualMemoryImpl::allocate( size_t order ) 
   throw (OutOfMemoryException) 
{
   linadr_t a = allocateNoThrow( order );   
   if (RARELY(a==0)) {
   // DPRINTK( "\n#4f#Out of memory! Throwing exception (nothrow=%b)...", zone&E_ZONE_NOTHROW );    
      throw OUTOFMEMORY;
   } else return a;
}

// virtual 
void 
IVirtualMemoryImpl::free( linadr_t start, size_t order )
{
   // DPRINTK( "\nIVirtualMemoryImpl::free start=%x order=%d", start, order );
 
   PHYS.releaseRegion( start, order );
   kernelpages.releaseMulti( getPDTIndex(start), 1UL << order );  // !after!
}

// virtual 
linadr_t 
IVirtualMemoryImpl::allocPageNoThrow( physadr_t& mapping ) 
{
   u32 startindex = kernelpages.getIndex();  // only 1
   if (startindex != kernelpages.E_FULL) {
      linadr_t a = (linadr_t) ((startindex << 12) + 0xFF800000);
      if ((mapping = PHYS.mapRegion( a, 0, E_PAGE_KERNEL ))) {
         // ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, a );
         
         // to see if it matters... YES!
         // memset_aligned( a, 0, sizeof(Page) );
         
         return a;         
      } else {
         kernelpages.releaseIndex( startindex );     
      }
   }
   // DPRINTK( "\n#4f#Out of memory! Throwing exception (nothrow=%b)...", zone&E_ZONE_NOTHROW );
   return 0;
}

linadr_t 
IVirtualMemoryImpl::allocPage( physadr_t& mapping ) throw (OutOfMemoryException)
{
   linadr_t a = allocPageNoThrow(mapping);   
   if (RARELY(a==0)) {
   // DPRINTK( "\n#4f#Out of memory! Throwing exception (nothrow=%b)...", zone&E_ZONE_NOTHROW );    
      throw OUTOFMEMORY;
   } else return a;   
}

// virtual 
void 
IVirtualMemoryImpl::freePage( linadr_t vmem )
{
   PHYS.releaseRegion( vmem, 0 );
   kernelpages.releaseIndex( getPDTIndex(vmem) );  // !after! 
}


// virtual 
linadr_t 
IVirtualMemoryImpl::allocateStack( size_t order ) throw (OutOfMemoryException) 
{
   ASSERTION( order <= 5, E_ERROR ); // limit  
 
   u32 startindex = kernelpages.getMulti( (1UL<<order)+1 );
   if (startindex != kernelpages.E_FULL) {
      ++startindex;  // first page not allocated
      linadr_t a = (linadr_t) ((startindex << 12) + 0xFF800000);

      if (PHYS.mapRegion( a, order, E_PAGE_KERNEL )) {
         return a;         
      } else {
         kernelpages.releaseMulti( startindex, (1UL<<order)+1 );
      }
   }

   DPRINTK( "\n#4f#Out of memory for stack! Throwing exception..." );   
   throw OUTOFMEMORY;
}

// virtual 
void 
IVirtualMemoryImpl::freeStack( linadr_t stack, size_t order )
{
   PHYS.releaseRegion( stack, order );
   kernelpages.releaseMulti( getPDTIndex(stack)-1, (1UL<<order)+1 );  // !after! 
}

// virtual
linadr_t
IVirtualMemoryImpl::mapMMIO( physadr_t physicalStart, u32 byteCount )
{
    u32 pageCount = DIV_ROUND_UP( byteCount, sizeof(Page) );
      
   // no need to allocate physical pages, just linear frames
   u32 index = kernelpages.getMulti( pageCount );
   if (index != kernelpages.E_FULL) {
      // put it in high (KERNEL) memory, not accessible to usermode...
      linadr_t a = (linadr_t) ((index << 12) + 0xFF800000);
      PHYS.mapMMIO( a, physicalStart, pageCount );
      return a;
   }
   DPRINTK( "\nFailed to find linear frames for MMIO mapping!" );
   return 0;   // throw ??
}

//virtual 
void 
IVirtualMemoryImpl::unmapMMIO( linadr_t adr, u32 byteCount )
{
    u32 pageCount = DIV_ROUND_UP( byteCount, sizeof(Page) );
    PHYS.unmapMMIO( adr, pageCount );
    kernelpages.releaseMulti( getPDTIndex(adr), pageCount );
}

// virtual
linadr_t
IVirtualMemoryImpl::allocISADMA( u32 pageOrder, physadr_t& physAdr )
{
   // NOTE: getMulti could also throw some exception, saves checks!
   u32 index = kernelpages.getMulti( 1UL << pageOrder ); 
   if (index != kernelpages.E_FULL) {
      // put it in high (KERNEL) memory, not accessible to usermode...
      linadr_t a = (linadr_t) ((index << 12) + 0xFF800000);
      physAdr = PHYS.mapDMA( a, pageOrder );
      return a;
   }
   DPRINTK( "\nFailed to find linear frames for DMA mapping!" ); 
   physAdr = 0;   // just in case
   return 0;
}

// virtual 
void 
IVirtualMemoryImpl::freeISADMA( linadr_t mem, u32 order )
{
   PHYS.unmapDMA( mem, order );
   kernelpages.releaseMulti( getPDTIndex(mem), (1UL<<order) );  // !after!  
}

}}  // namespaces
