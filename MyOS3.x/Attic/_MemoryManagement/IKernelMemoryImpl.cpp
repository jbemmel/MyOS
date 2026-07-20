#include "IKernelMemoryImpl.h"

#include "MemoryManagement/KMComponent.h"
#include "PBT.h"
#include "Page.h"
#include "GDT.h"

#include "debug.h"

// test
// #include "IO/DisplayComponent.h"
#include "Devices/Printer/LPTStream.h"

// Linker constants
extern "C" u8 _START_OF_CODE_;
extern "C" u8 _START_OF_DATA_;

namespace MyOS { namespace MemoryManagement {

IKernelMemoryImpl::IKernelMemoryImpl( MyOS::Core::IComponent& c )
: IKernelMemory( c, VERSION(1,0) )
, generalAllocator(E_PAGE_KERNEL), dmaAllocator( E_PAGE_MMIO )
{

}

bool 
IKernelMemoryImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	/// Write-protect kernel code & read-only data
	this->setAttributes( &_START_OF_CODE_, &_START_OF_DATA_ - &_START_OF_CODE_, E_PAGE_SHAREABLE );

	GDT::initialize();
	theFramePoolManager.initMemory();

   /* test
   theFramePoolManager.listTables( 
      IO::DisplayComponent::getInstance().cout()
   );

   Devices::LPT::LPTStream printer;
   theFramePoolManager.listTables( printer );
   */   
   Devices::LPT::LPTStream printer;
   this->getStatistics( printer );
	return true;
}

void 
IKernelMemoryImpl::deinit( IContext& context )
{

}

linadr_t
IKernelMemoryImpl::allocate( 
   Core::IComponent &requestor, size_t bytes, kmflags_t flags /*=E_KM_NONE*/ )
   throw (OutOfMemoryException)
{
	// maintain stats
	bytesAllocated += bytes;
	++nAllocations;

   linadr_t result = 0;

    // could track the requestor...
    // PRINTF( "\nRequest for %u bytes by %s", bytes, requestor.getIDLAttribute("name") );

    /// For normal small requests, use the blockallocator(s)
    if ( bytes <= BlockAllocator::maxBlockSize() ) {
        if (flags == E_KM_NONE)
            result = generalAllocator.allocate( bytes );
        else if (flags == E_KM_DMA)
            result = dmaAllocator.allocate( bytes );
    }

    // nothing yet ?
    if (result==0) {
       // else (for now), just allocate by entire pages
       if (flags==E_KM_BELOW_1MB) {
         // PRINTF( "\nAllocating %u bytes below 1Mb...", bytes );
         result = theFramePoolManager.kallocate1MBPages( bytes );
       } else {
           u32 atts = E_PAGE_KERNEL;
           if (flags&E_KM_READ_ONLY) atts &= ~E_PAGE_READWRITE;
           if (flags&E_KM_DMA) atts |= (E_PAGE_NOCACHE|E_PAGE_WRITETHROUGH);
           result = theFramePoolManager.kallocateRange( bytes, atts );
       }
   }
   // PRINTF( "\nAllocated %u bytes flags %x returning %x", bytes, flags, result );
   return result;
}

void
IKernelMemoryImpl::free( linadr_t address, size_t bytes, kmflags_t flags  )
{
//   ASSERTION( address, Debug::AS_ERROR );

   // maintain stats
   bytesFreed += bytes;
   ++nFrees;

   // PRINTF( "\nFreeing %u bytes at %x", bytes, address );

   if ( bytes <= BlockAllocator::maxBlockSize())
   {
      if (flags == E_KM_NONE) {
         generalAllocator.deallocate( (Page*) address, bytes );
         return;
      } else if (flags == E_KM_DMA) {
         dmaAllocator.deallocate( address, bytes );
         return;
      }
   }

   // else...address & 0xFFF should be 0...
   if (flags==E_KM_BELOW_1MB) {
      theFramePoolManager.kfree1MBPages( (Page*) address, bytes );
   } else {
      theFramePoolManager.kfreeRange( (Page*) address, bytes );
   }
}

linadr_t
IKernelMemoryImpl::mapMMIO( physadr_t physicalStart, size_t bytes, linadr_t hint /*=0*/ )
{
   if (hint) {
      // assume caller knows what it's doing...
      // PRINTF( "\nMapping physical %x to linear %x for %u bytes...",
      //   physicalStart, hint, bytes );
      for (u32 i = (u32) hint, end = i + bytes; i<end; i += _4KB )
      {
         register linadr_t a = (linadr_t) i;
         PBT::getDirectory( a )[ getPDTIndex(a) ].map( physicalStart, E_PAGE_MMIO );
         physicalStart += _4KB;
      }
      return hint;
   } else return theFramePoolManager.mapMMIO( physicalStart, bytes, E_PAGE_MMIO );
}

void
IKernelMemoryImpl::releaseMMIO( linadr_t address, size_t bytes )
{
    theFramePoolManager.unmapMMIO( (Page*) address, bytes );
}

physadr_t
IKernelMemoryImpl::getMapping( linadr_t forAdr ) const
{
    // could be completely inlined by caller, but...
    return ((Page*) forAdr)->getMapping();
}

// virtual
pteflags_t
IKernelMemoryImpl::getAttributes( const linadr_t forAdr ) const
{
   return ((Page*) forAdr)->getAttributes();
}

void
IKernelMemoryImpl::mapRegion( linadr_t dest, linadr_t src, size_t pagecount, pteflags_t atts )
{
// PTE bits for caching should be (and remain) identical, but other bits 
// may be changed for the copy
#define ALLOWED_BITS ( E_PAGE_USER    | E_PAGE_ACCESSED  | E_PAGE_DIRTY \
                     | E_PAGE_AVAIL0  | E_PAGE_AVAIL1    | E_PAGE_AVAIL2 \
                     | E_PAGE_PRESENT | E_PAGE_READWRITE | E_PAGE_GLOBAL)      
 
//   ASSERTION( (atts & ~ALLOWED_BITS) == 0, Debug::AS_ERROR );
 
   /// these map the directory if not already done (src should already be mapped...)
   PageDirectory &srcdir = PageDirectory::getDir( src );
   PageDirectory &destdir = PageDirectory::getDir( dest );

   PTE* psrc = &srcdir[ getPDTIndex(src)   ];
   PTE* pdes = &destdir[ getPDTIndex(dest) ];

   // if no atts are provided, there is a very efficient way...
   if (!atts) {
      memcpy_aligned( pdes, psrc, pagecount*sizeof(PTE) /* bytes */ );
   } else {
      atts &= ALLOWED_BITS;
      
      for( size_t i=0; i<pagecount; ++i) {
         pdes[i].initialize( 
            psrc[i].getMapping(), 
            (psrc[i].getAttributes() & ~ALLOWED_BITS) | atts
         );
      }
#undef ALLOWED_BITS      
   }      

   // XX should maintain state on these aliases, if the src pages are freed their physical frames
   // are put in the free map, and if they get reallocated the dest region suddenly changes...
   //

   // it is possible that the current code page is now remapped! so...
   // RUNONSTACK(24);
 PTE::invalidateAll();
}

void
IKernelMemoryImpl::unmapRegion( linadr_t dest, size_t pagecount )
{
   /// these map the directory if not already done (dest should already be mapped...)
   PageDirectory &destdir = PageDirectory::getDir( dest );

   // assuming this code is not being unmapped itself...
   memset_aligned( &destdir[ getPDTIndex(dest) ], 0, pagecount*sizeof(PTE) );   // in bytes

   PTE::invalidateAll();
}

void
IKernelMemoryImpl::setAttributes( linadr_t forAdr, size_t bytes, pteflags_t flags ) const
{
   // round bytes up to next page boundary
   bytes = (bytes + _4KB - 1) & 0xFFFFF000;

 // Depending on state transitions, this might be needed (write back & flush)
   if (flags & (E_PAGE_NOCACHE|E_PAGE_WRITETHROUGH)) {
      PTE::flushcache();
   }

   for ( Page* p = (Page*) forAdr; bytes>0; bytes-=_4KB, ++p)
   {
      p->setAttributes( flags );
   }
}

// virtual
linadr_t
IKernelMemoryImpl::getBIOSPage()
{
   if (biosPage0==0) {
      biosPage0 = theFramePoolManager.mapMMIO( 0, _4KB, E_PAGE_READONLY | E_PAGE_PRESENT );
   }
   return biosPage0;
}

// virtual 
myos_result_t
IKernelMemoryImpl::listTables( IO::OStream& ostream ) const {
   return theFramePoolManager.listTables(ostream);
}

// virtual 
myos_result_t
IKernelMemoryImpl::getStatistics( IO::OStream& ostream ) const {
   ostream.printf(
     "\n<STATS allocated=\"%d\" freed=\"%d\" inuse=\"%d\""
      " allocCount=\"%d\" freeCount=\"%d\" usage_above_core=\"%u\"/>",
      bytesAllocated, bytesFreed,bytesAllocated-bytesFreed,
      nAllocations, nFrees, (bytesAllocated-bytesFreed)-recordedUsage );
   return E_MYOS_SUCCESS;
}

// virtual 
linadr_t 
IKernelMemoryImpl::createStack( size_t pages /*=1*/ )
{
   return theFramePoolManager.createStack(pages); 
}

// virtual 
void 
IKernelMemoryImpl::freeStack( linadr_t stack, size_t pages )
{
   theFramePoolManager.freeStack(stack,pages);
}

}}  // namespaces
