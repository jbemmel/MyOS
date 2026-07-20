#include "IVirtualMemoryImpl.h"
#include "MMComponent.h"
#include "Boot/memlayout.h"
#include "GDT.h"
#include "Page.h" // isZero

#include "debug.h"

// Linker symbol for static constructors
extern u32 _CTOR_LIST__;

namespace MyOS { namespace MM {

#define OUTOFMEMORY OutOfMemoryException( __FILE__ )

//const uuid_t IVirtualMemory::ID = "2a147d6f-cab0-43b8-8b9f-4687312c101d";

IVirtualMemoryImpl::IVirtualMemoryImpl( MyOS::Core::IComponent& c )
: IVirtualMemory( c, VERSION(1,0) )
, totalPhysicalMemory(0)
{

}

bool
IVirtualMemoryImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // switch to own GDT
   GDT::initialize();

   // NOTE: This frees UNCOMPRESSED pages
   size_t firstCodePageIndex = getPDTIndex( (linadr_t) (u32) &_COMPRESSED_START_LINEAR_);
   size_t lastCodePageIndex  = firstCodePageIndex + (u32) &_CODE_PAGES_;
   for (u32 i=firstCodePageIndex;i<lastCodePageIndex; ++i ) {
       kernel_Codepages.markUsed( i );
   }

   size_t firstDataPageIndex = getPDTIndex( (linadr_t) (u32) &_START_OF_DATA_ );
   size_t lastDataPageIndex  = firstDataPageIndex
                             + ((u32) &_KERNEL_PAGES_ - (u32) &_CODE_PAGES_)
                             + getBitmapPages();
   for (u32 i=firstDataPageIndex;i<lastDataPageIndex; ++i ) {
       kernel_Datapages.markUsed( i );
   }

   size_t firstModulePageIndex = getPDTIndex( (linadr_t) (u32) &_START_OF_MODULE_ );
   size_t lastModulePageIndex  = firstModulePageIndex + extraPagesAllocated;
   for (u32 i=firstModulePageIndex;i<lastModulePageIndex; ++i ) {
       kernel_Datapages.markUsed( i );
   }


   DPRINTK( "\n code %u-%u data %u-%u module %u-%u", firstCodePageIndex, lastCodePageIndex,
           firstDataPageIndex, lastDataPageIndex, firstModulePageIndex, lastModulePageIndex );

   /* After setting up memory, we can run static constructors, listed by linker
   for( u32 *staticConstructors = (u32*) &_CTOR_LIST__; *staticConstructors; ++staticConstructors ) {
       void (*constructor)() = (void(*)()) *staticConstructors;
       constructor();
   }*/


   // show initial state for debug
   // MyOS::Devices::LPT::LPTStream printer;
   // this->listPools( printer );
   // this->listTables( printer );

   // Clean up some garbage...
   // memset_aligned( (void*) 0xFF800000, 0, (u32) &_COMPRESSED_START_LINEAR_ - 0xFF800000 );
   // DPRINTK( "\nClean size=%x", (u32) &_COMPRESSED_START_LINEAR_ - 0xFF800000 );

   return this->initPhysical( context, params );
}

void
IVirtualMemoryImpl::deinit( IContext& context )
{

}

// virtual
myos_result_t
IVirtualMemoryImpl::getStatistics( IO::OStream& out ) const {
    out.printf( "\nNumber of free kernel pages: data=%u code=%u",
            kernel_Datapages.getFree(), kernel_Codepages.getFree() );
    out.printf( "\nTotal physical pages: %d\nAvailable pages: %d",
          pages.getTotalUnits(), pages.getAvailableUnits() );
    return E_MYOS_SUCCESS;
}

// virtual
linadr_t
IVirtualMemoryImpl::allocateNoThrow( size_t order, physadr_t* mapping )
{
   ASSERTION( order <= 5, E_ERROR ); // implementation limit

   u32 startindex = kernel_Datapages.getMulti( 1UL << order );
   if (startindex != kernel_Datapages.E_FULL) {
      linadr_t a = (linadr_t) ((startindex << 12) + (u32) &_KERNEL_BASE_ );
      physadr_t phys;
      if (( phys = mapRegion( a, order, E_PAGE_KERNEL ))) {
         // ASSERTIONv( ((Page*)a)->isZero(), E_CRITICAL, a );

         // to see if it matters
         // memset_aligned( a, 0, npages*_4KB );
         // DPRINTK( "\nAllocated page(s): %x order=%u", a, order );
          // ASSERTIONv( ((u32)a<0xffa0c000) || ((u32)a>=0xffa14000), E_CRITICAL, a );
          if ( mapping) *mapping = phys;
          return a;
      } else {
          kernel_Datapages.releaseMulti( startindex, 1UL << order );
      }
   }
   return 0;
}

// virtual
linadr_t
IVirtualMemoryImpl::mmap( size_t order, bool executable ) throw (MM::OutOfMemoryException)
{
    ASSERTION( order <= 5, E_ERROR ); // implementation limit, 2^5 = 32 bits

    u32 startindex = executable ? kernel_Codepages.getMulti( 1UL << order )
                                : kernel_Datapages.getMulti( 1UL << order );
    if (startindex != kernel_Datapages.E_FULL) {
        linadr_t a = (linadr_t) ((startindex << 12) + (u32) &_KERNEL_BASE_);

        // Special value: paging device #0, R/W, upper 24-bits=0x000001 meaning demand-paged
        fillEntries( a, 1UL<<order, 0x00000100, 0 );
        return a;
    } else {
         throw OUTOFMEMORY;
    }
}

// virtual
void
IVirtualMemoryImpl::unmap( linadr_t start, size_t order )
{
    clearMapping( start, 1UL<<order );   // first! without freeing the frames
    if ( start >= &_START_OF_DATA_ ) {
        kernel_Datapages.releaseMulti( getPDTIndex(start), 1UL << order );  // !after!
    } else {
        kernel_Codepages.releaseMulti( getPDTIndex(start), 1UL << order );  // !after!
    }
}

// virtual
linadr_t
IVirtualMemoryImpl::allocate( size_t order, physadr_t* mapping )
   throw (MM::OutOfMemoryException)
{
   linadr_t a = allocateNoThrow( order, mapping );
   if (RARELY(a==0)) {
      DPRINTK( "\n#4f#Out of memory! Throwing exception..." );
      throw OUTOFMEMORY;
   } else return a;
}

// virtual
void
IVirtualMemoryImpl::free( linadr_t start, size_t order )
{
   DPRINTK( "\nIVirtualMemoryImpl::free start=%x order=%d", start, order );

   releaseRegion( start, order );
   unmap( start, order );
}

// virtual
linadr_t
IVirtualMemoryImpl::allocPageNoThrow( physadr_t& mapping )
{
   u32 startindex = kernel_Datapages.getIndex();  // only 1
   if (startindex != kernel_Datapages.E_FULL) {
      linadr_t a = (linadr_t) ((startindex << 12) + (u32) &_KERNEL_BASE_ );
      if ((mapping = mapRegion( a, 0, E_PAGE_KERNEL ))) {      //< @todo add parameter for caller to set PTE bits
         // ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, a );

         // to see if it matters... YES!
         // memset_aligned( a, 0, sizeof(Page) );
          // DPRINTK( "\nAllocated page: %x", a );
          // ASSERTIONv( ((u32)a<0xffa0c000) || ((u32)a>=0xffa14000), E_CRITICAL, a );
          // ASSERTION( (mapping&0xfffff000) != 0x25f000, E_CRITICAL );
          return a;
      } else {
         kernel_Datapages.releaseIndex( startindex );
      }
   }
   // DPRINTK( "\n#4f#Out of memory! Throwing exception (nothrow=%b)...", zone&E_ZONE_NOTHROW );
   return 0;
}

linadr_t
IVirtualMemoryImpl::allocPage( physadr_t& mapping ) throw (MM::OutOfMemoryException)
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
    DPRINTK( "\nIVirtualMemoryImpl::freePage vmem=%x", vmem );

   releaseRegion( vmem, 0 );
   if (vmem >= &_START_OF_DATA_) {
       kernel_Datapages.releaseIndex( getPDTIndex(vmem) );  // !after!
   } else {
       kernel_Codepages.releaseIndex( getPDTIndex(vmem) );  // !after!
   }
}


// virtual
linadr_t
IVirtualMemoryImpl::allocateStack( size_t order, bool userMode ) throw (MM::OutOfMemoryException)
{
   ASSERTION( order <= 5, E_ERROR ); // limit

   u32 startindex = kernel_Datapages.getMulti( (1UL<<order)+1 );
   if (startindex != kernel_Datapages.E_FULL) {
      ++startindex;  // first page not allocated
      linadr_t a = (linadr_t) ((startindex << 12) + (u32) &_KERNEL_BASE_ );

      if (mapRegion( a, order, E_PAGE_KERNEL | (userMode?E_PAGE_USER:0) )) {
         // ASSERTION( 0 == PHYS.getPhysicalAddress( (linadr_t) (((startindex-1) << 12) + 0xFF800000) ), E_ERROR );
          return a;
      } else {
         kernel_Datapages.releaseMulti( startindex-1, (1UL<<order)+1 );
      }
   }

   DPRINTK( "\n#4f#Out of memory for stack! Throwing exception..." );
   throw OUTOFMEMORY;
}

// virtual
void
IVirtualMemoryImpl::freeStack( linadr_t stack, size_t order )
{
    // DPRINTK( "\nIVirtualMemoryImpl::freeStack stack=%x order=%u", stack, order );

    releaseRegion( stack, order );
    u32 startindex = getPDTIndex(stack)-1;
    kernel_Datapages.releaseMulti( startindex, (1UL<<order)+1 );  // !after!
}

// virtual
linadr_t
IVirtualMemoryImpl::mapMMIO( physadr_t physicalStart, u32 byteCount )
{
    u32 pageCount = DIV_ROUND_UP( byteCount, sizeof(Page) );

    // no need to allocate physical pages, just linear frames
    u32 index = kernel_Datapages.getMulti( pageCount );
    if (index != kernel_Datapages.E_FULL) {
        // put it in high (KERNEL) memory, not accessible to usermode...
        linadr_t a = (linadr_t) ((index << 12) + (u32) &_KERNEL_BASE_);
        mapMMIOPhysical( a, physicalStart, pageCount );
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
    unmapMMIOPhysical( adr, pageCount );
    kernel_Datapages.releaseMulti( getPDTIndex(adr), pageCount );
}

// virtual
linadr_t
IVirtualMemoryImpl::allocISADMA( u32 pageOrder, physadr_t& physAdr )
{
   // NOTE: getMulti could also throw some exception, saves checks!
   u32 index = kernel_Datapages.getMulti( 1UL << pageOrder );
   if (index != kernel_Datapages.E_FULL) {
      // put it in high (KERNEL) memory, not accessible to usermode...
      linadr_t a = (linadr_t) ((index << 12) + (u32) &_KERNEL_BASE_);
      physAdr = mapDMA( a, pageOrder );
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
    // DPRINTK( "\nfreeISADMA(%x,%d)", mem, order );

    unmapDMA( mem, order );
    kernel_Datapages.releaseMulti( getPDTIndex(mem), (1UL<<order) );  // !after!
}

}}  // namespaces
