#line 1 "FramePoolManager.cpp"

#include "FramePoolManager.h"
#include "PageDirectory.h"
#include "PBT.h"
#include "Page.h"

#include "IO/OStream.h"

#include "debug.h"
#define TRACE_MEM(T)
#define CHECKPOINT(P)
#define FATAL_ERROR(e) ASSERTION(e==0,E_EMERGENCY)

// Start of free memory, linker constant
extern void* _FREE_KERNEL_SPACE_START_;

namespace MyOS {
namespace MemoryManagement {

FramePoolManager::FramePoolManager()  // also initializes kernelpages.free (!)
{
}

// in-place new, needed below
inline void* operator new( size_t, void* m ) { return m; }

void
FramePoolManager::initMemory()   // INITSECTION
{
   // determine end-of-mapped-memory marker (round up to end of page)
   u32 endOfUsedMemory = (u32) &_FREE_KERNEL_SPACE_START_ - 1;
   size_t lastPageIndex = (endOfUsedMemory - 0xFF800000) >> 12;

   // mark all used kernel pages (linear frames, physical done in FramePool)
   for (u32 i=0; i<=lastPageIndex; ++i ) {
      kernelpages.markUsed( i );
   }

   // find out how much memory there is (bytes)
   physadr_t maxPhysMemory = totalPhysicalMemory = testMemory();

   // Create a PDT that maps the physical memory pools (at end of phys mem)
   // and map it
   PBT::getCurrentPBT()[ 
      getPBTIndex( (linadr_t) FramePool::pools() ) 
   ].map( maxPhysMemory-=sizeof(PBT), E_PAGE_KERNEL ); 

   // Calculate how many pools there are, and construct them in 
   // contiguous memory starting at FramePool::E_FRAMEPOOLS linear
   // (using physical frames counting down from top of physical memory)
   size_t npools = (totalPhysicalMemory >> 22);   // 1 pool per 4 Mb
   size_t nframes = (npools * sizeof(FramePool)) / sizeof(Page);

   // Allocate & map physical frames for the pools
   PageDirectory& pooldir = PageDirectory::getDir( FramePool::pools() );
   for (u32 f = 0; f<nframes; ++f) {
      pooldir[f].map( maxPhysMemory-=sizeof(Page), E_PAGE_KERNEL );
   }

   // GDT has been remapped, so we can free almost all <1Mb frames   
   FramePool* p0 = FramePool::map1stMb();
   
   // Can do this, but need to clear the pages! And see kallocate1MBPages
   // poolring.insert( p0->ringnode );    // TODO: Something special for <1Mb frames
   
   // create the 4Mb pools for all physical memory
   // DPRINTK( "\nCreating %d pools...", npools );
   for ( u32 p=1; p<npools; ++p ) {
      FramePool *pool = new ( &FramePool::pool(p) ) FramePool();
      poolring.insert( pool->ringnode );
   }

   // mark the pages used for the pool PDT and the pools created above
   FramePool& poolpool = FramePool::getPoolForFrame( totalPhysicalMemory );
   for ( u32 p=1023-npools-1; p<1024; ++p ) {
      poolpool.allocationmap.markUsed( p );   // at back!
   }      
}

physadr_t
FramePoolManager::testMemory()  // INITSECTION, called by initMemory
{
   // The pte used to temporarily map pages for testing
   PTE &pte = PageDirectory::getDir(0)[1];

   // find the amount of physical memory present, start counting at 4Mb, 
   // test by 4Mbs
   for ( physadr_t adr = _4MB; adr; adr += _4MB )
	{
		// map as page 1 (non-cached)
		volatile u8 * const byte0_0   = (u8 *) 0x0;
		volatile u32 * const dword0_0 = (u32 *) 0x0;
		volatile u32 * const dword1_0 = (u32 *) 0x1000;
		pte.map( adr, E_PAGE_MMIO );	// map uncached, works ?

		// try reading it, writing it (this is what init_memory does)
		u32 value = !((*dword1_0) | (*byte0_0));
		*dword1_0 = value;
		volatile const u32 avoid_ghost UNUSED = *dword0_0;
		if (*dword1_0 != value) {
			pte.unmap();	// clean up
			return adr;
		}
	}
	return 0;	// something very wrong here...HLT is better
}

physadr_t
FramePoolManager::allocateFrame()
{
	FramePool *pool = &poolring.first()->content();
	for (u32 i=0; i<poolring.size(); ++i)
	{
		// could direct the next thread to the next pool (not strictly needed)
		physadr_t adr = pool->allocateFrame();		// get a frame

      if (adr != 0) {                  // pool could be empty
         ASSERTIONv( (adr & 0xFFF) == 0, E_ERROR, adr );
         return adr;
      } else {
         // NOTE: This is probably bad for caching, better to return 'hot' pages         
         pool = &poolring.rotate()->content();     // shift entire ring
		}
	}

	FATAL_ERROR( "Could not get a pool!" );
	return 0;
}

physadr_t
FramePoolManager::allocateFrameRange( u8 count /* max 32, for 32*4K = 128K memory */ )
{
	ASSERTION( count <= 32, E_ERROR );

	FramePool *pool = &poolring.first()->content();
	for (u32 i=0; i<poolring.size(); ++i)
	{
		// could direct the next thread to the next pool (not strictly needed)
		physadr_t adrStart = pool->allocateFrameRange(count);	    // get a frame range

		if (adrStart != 0)              // pool could be empty !
			return adrStart;
		else {
			pool = &poolring.rotate()->content();
		}
	}
	FATAL_ERROR( "Could not get a pool for range!" /* ,count */ );
	return 0;
}

// kernel linear memory, this method should be moved!
Page*
FramePoolManager::kallocatePage( pteflags_t bits /* = E_PAGE_KERNEL */ )
   throw (OutOfMemoryException)
{
	// CHECKPOINT( '+' );

	// strategy: first linear address, then physical (physical likely easier to get ?)
	u32 index = kernelpages.getIndex();
	if (index != kernelpages.E_FULL) {
		physadr_t frame = allocateFrame();
		if (frame) {
			register linadr_t a = (linadr_t) ((index << 12) + 0xFF800000);
			PBT::getDirectory( a )[ getPDTIndex(a) ].map( frame, bits );

			// TRACE_MEM(m);

         // extra check that page is all zeroes
         ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, frame );

			return (Page*) a;
		} else {
			kernelpages.releaseIndex( index );
		}
	}

	FATAL_ERROR( "Out of kernel memory!" );
	throw OUTOFMEMORY;
}

Page*
FramePoolManager::kallocateRange( size_t bytes, pteflags_t bits /* = E_PAGE_KERNEL */ )
{
	ASSERTIONv( bytes <= 128 * _1MB, E_ERROR, bytes );

	// more efficient, more chance of success
	if (bytes<=_4KB) return kallocatePage(bits);

	// strategy: first linear address, then physical (physical likely easier to get ?)
	u8 range = ((bytes + _4KB - 1) / _4KB);	// number of pages, round up

   // PRINTF( "\nkallocRange %u bits=%x", range, bits );

	u32 startindex = kernelpages.getMulti( range );	// check: range <= 32 !
   // PRINTF( "\nkallocRange startindex=%u bytes=%u", startindex, bytes );
	if (startindex != kernelpages.E_FULL) {
		physadr_t frame = allocateFrameRange(range);
		if (frame) {
         // this mapping can span multiple directories !
			for ( u32 i=((startindex << 12) + 0xFF800000); range--; i += _4KB, frame += _4KB )
			{
            // Mapping should be null, if not we have a problem! Bug in allocationMap?
            register linadr_t a = (linadr_t) i;
            ASSERTIONv( !PBT::getDirectory( a )[ getPDTIndex(a) ].isMapped(), 
               E_ERROR, i );
				PBT::getDirectory(a)[ getPDTIndex(a) ].map( frame, bits );
			}
			TRACE_MEM( ((startindex << 12) + 0xFF800000) );
			return (Page*) ((startindex << 12) + 0xFF800000);
		} else {	// frame==0
			kernelpages.releaseMulti( startindex, range );
			FATAL_ERROR( "Not enough frames for range!" );
		}
	} else FATAL_ERROR( "Not enough kernel memory for range!" );
	return 0;
}

// similar to kallocateRange, but without allocating physical frames
Page*
FramePoolManager::mapMMIO( physadr_t start, size_t bytes, pteflags_t flags )
{
	u32 startindex = kernelpages.getMulti( (bytes+_4KB-1) >> 12 );

	if (startindex != kernelpages.E_FULL) {
		for (u32 i = ((startindex << 12) + 0xFF800000), end = start+bytes;
         	start<end;
					i += _4KB, start += _4KB )
		{
			PBT::getDirectory( (linadr_t) i )[ getPDTIndex( (linadr_t) i) ].map( start, flags );
		}
		TRACE_MEM( (startindex << 12) + 0xFF800000 );
		return (Page*) ((startindex << 12) + 0xFF800000);
	} else {
		FATAL_ERROR( "Not enough kernel linear memory space to map MMIO" );
		return 0;
	}
}

// reverse the effect of kallocatePage
void
FramePoolManager::kfreePage( Page* page )
{
	ASSERTIONv( !(((u32) page) & 0xfff), E_ERROR, page );	// fails, compiler err
	TRACE_MEM( page );
	CHECKPOINT( '-' );

   // clear pages when freed, could have a cleaning thread for this
	memset_aligned( page, 0, _4KB );	
	physadr_t freed = PageDirectory::getDir( page )[ getPDTIndex(page) ].unmap();

   // PRINTF( "\nkfreePage lin=%x phys=%x", page, freed );

   FramePool::getPoolForFrame( freed ).releaseFrame( freed );
   kernelpages.releaseIndex( getPDTIndex(page) );
}

void
FramePoolManager::kfreeRange( Page* start, size_t bytes )
{
	ASSERTIONv( bytes <= 128 * _1MB, E_ERROR, bytes );
   ASSERTION( start->getMapping(), E_ERROR ); // catch 2x free
   TRACE_MEM( start );

	// more efficient, counterpart of kallocateRange
   if (bytes<=_4KB) {
      kfreePage( start );
      return;
   }
   ASSERTIONv( !(((u32) start) & 0xfff), E_ERROR, start );

	// clear memory when freeing, starting with the first page
	memset_aligned( start, 0, _4KB );

	// Keep address of first frame released
	physadr_t first = PageDirectory::getDir( start )[ getPDTIndex(start) ].unmap();
	for (u32 i=(u32)start + _4KB; i < ((u32)start + bytes); i+=_4KB )
	{
		memset_aligned( (linadr_t) i, 0, _4KB );
		PageDirectory::getDir( (linadr_t) i )[ getPDTIndex((linadr_t) i) ].unmap();
	}

	u8 range = ((bytes + _4KB - 1) >> 12);

	// NOTE: By definition, there cannot be multiple pools !
	FramePool::getPoolForFrame( first ).releaseFrameRange( first, range );

	// important to do this last!
	kernelpages.releaseMulti( getPDTIndex(start), range );
}

// kfreeRange without releasing the frames to a pool, could be merged ?
void
FramePoolManager::unmapMMIO( Page* start, size_t bytes )
{
	TRACE_MEM( start );

	for (u32 i=(u32)start; i < ((u32)start + bytes); i+=_4KB )
	{
		register linadr_t a = (linadr_t) i;
		PageDirectory::getDir( a )[ getPDTIndex(a) ].unmap();
	}
	u8 range = ((bytes + _4KB - 1) >> 12);

	// important to do this last!
	kernelpages.releaseMulti( getPDTIndex(start), range );
}

u32    // counts the number of available physical frames
FramePoolManager::getFreePages() const
{
	register u32 total = 0;
	for (u32 i=0; i<poolring.size(); ++i)	// ## NOTE ## This excludes 0-4Mb pool !!
	{
		total += FramePool::pool(i).availableFrames();
	}
	return total;
}

/**
 * Especially for pages below 1Mb, not crossing 64Kb (16 pages) segment border
 * 
 * NOTE: Sometimes the memory is not cleared yet! problem ??
 */
Page*
FramePoolManager::kallocate1MBPages( size_t bytes )
{
	TRACE_MEM( bytes );
	size_t pages = (bytes+_4KB-1) >> 12;
   // XX not guaranteed that this is below 1Mb, only because that's where
   // allocs will start. Eventually this will fail! 
	physadr_t start = FramePool::pool(0).allocateFrameRange(pages,16);
	ASSERTION( start && start < _1MB, E_ERROR );
	return start ? mapMMIO( start, bytes ) : 0;
}

void
FramePoolManager::kfree1MBPages( Page* start, size_t bytes )
{
	TRACE_MEM( start );
	ASSERTION( start && bytes, E_ERROR );

	if (start && bytes>0) {
      size_t roundedBytes = (bytes+_4KB-1) & 0xFFFFF000;
      memset_aligned( start, 0, roundedBytes );
		physadr_t physstart = start->getMapping();
		unmapMMIO( start, bytes );
		FramePool::pool(0).releaseFrameRange(physstart,roundedBytes>>12);
	}
}

myos_result_t
FramePoolManager::listTables( IO::OStream &o ) const
{
   u32 start = o.getMarker();
   o.printf( "\n<MEMORY physical=\"%d\" freeframes=\"%d\">", 
      totalPhysicalMemory, getFreePages() );
   u32 count = 0;
	for (u32 i=0; i<1024; ++i)
	{
		PageDirectory &dir = PageDirectory::getDir( (linadr_t) (i*_4MB) );
		if (PBT::isMapped(dir)) count += dir.listTable(o);
	}
	o.printf( "\n<TOTAL pages=\"%d\"/>\n</MEMORY>", count );
	return (myos_result_t) (o.getMarker() - start);
}

linadr_t
FramePoolManager::createStack( size_t pages )
{
   DPRINTK( "\nFramePoolManager::createStack this=%x size=%d", this, pages * sizeof(Page) );
   Page* stack = this->kallocateRange( (pages+1) * sizeof(Page) );
   DPRINTK( "\nCreated stack pages at %x", stack );
   if (stack) {
      PageDirectory &dir = PageDirectory::getDir( stack );
      this->releaseFrame( dir[ getPDTIndex(stack) ].unmap() );
      return ++stack;   // next page
   } else {
       // XX else throw exception! 
       BUG( "createStack failed, TODO throw exception" );
       return 0;
   }       
}

void
FramePoolManager::freeStack( linadr_t stack, size_t pages )
{
   ASSERTION( ((u32)stack > 0xFF800000) && ((u32)stack & 0xFFF)==0, E_ERROR );
   this->kfreeRange( (Page*) stack, pages * sizeof(Page) ); 
   kernelpages.releaseIndex( getPDTIndex( (void*) stack ) - 1 );
}

}}
