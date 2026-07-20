#include "../IPhysicalMemoryImpl.h"
#include "../new.h"
#include "Init/memlayout.h"

#include "PageDirectory.h"
#include "PBT.h"
#include "Page.h"

#include "IO/OStream.h"
#include "Locking/LockHelper.h"

#include "debug.h"
#define FATAL_ERROR(e) ASSERTION(e==0,E_EMERGENCY)

// BIOS hdd info collected in bootphase2, then copied here
u8 bioshddinfo[ 8 * 0x42 ];   // in .rodata (see memlayout.h)

/**
 * Implementation for physical MM
 */
namespace MyOS { namespace MM { 

// INITSECTION
IPhysicalMemoryImpl::IPhysicalMemoryImpl( MyOS::Core::IComponent& c )
: IPhysicalMemory( c, VERSION(1,0) )
, totalPhysicalMemory(0)
{
   // need to finish initialize pages later!  
   // spin_lock_init( &memlock );
} 

bool
IPhysicalMemoryImpl::init( IContext& context, NVPAIR params[] )   // INITSECTION
{
   size_t firstPageIndex = getPDTIndex(&_COMPRESSED_START_LINEAR_);
   size_t lastPageIndex = getPDTIndex( (void*) ((u32) &_FREE_KERNEL_SPACE_START_ - 1) );
   size_t kernelpages = lastPageIndex - firstPageIndex - 1;	
	
   // Start allocating pages after the kernel
   physadr_t target = KERNEL_PHYSICAL + (kernelpages-1) * sizeof(Page);

   // Create a PDT that maps the allocator bitmap pages (at end of phys mem)
   // and map it
   PBT::getCurrentPBT()[ 
      getPBTIndex( (linadr_t) ALLOCATOR::E_MAPPING ) 
   ].map( target+=sizeof(PBT), E_PAGE_KERNEL );
   ++kernelpages; 

   PageDirectory& pooldir = PageDirectory::getDir( (linadr_t) ALLOCATOR::E_MAPPING );
   // DPRINTK( "\nAllocating %d pages for bitmap", ALLOCATOR::getPageCount() );
   for (u32 f = 0; f<ALLOCATOR::getPageCount(); ++f) {
      pooldir[f].map( target+=sizeof(Page), E_PAGE_KERNEL );
      ++kernelpages;
   }

   pages.init();  // now we can clear those pages
      
   // Parse the BIOS int 15h 0xE820 result, stored by phase2 at 0x7C00
   // See http://www.uruk.org/orig-grub/mem64mb.html
   
   // See also linux' e820.c:
   // Type==1  -> Available
   // Type==2  -> Reserved
   // Type==3  -> ACPI, can be reused after tables have been copied
   //             (I don't need them yet, so I could overwrite them :)
   //             However, Paola's laptop reports only 0xFFC0 of it...
   //             The rest of that page is type 4, so not much is gained
   // Type==4  -> ACPI non-volatile memory
   struct BIOSMEMMAP {
      u32 baselo, basehi;
      u32 lenlo, lenhi;
      u32 type;      // 1=available, 2=reserved, other = undefined (reserved) 
   } *biosmemmap = (BIOSMEMMAP*) 0x7C00;

   while (biosmemmap->baselo != 0xFFFFFFFF) {   // phase2 puts -1 at end
      //DPRINTK( "BIOS reports memory base=%x len=%x type=%d\n",
      //   biosmemmap->baselo, biosmemmap->lenlo, biosmemmap->type );
         
      if (biosmemmap->type == 1) {  // available memory ?
         u32 base   = biosmemmap->baselo;
         u32 length = biosmemmap->lenlo;

         // Save lowest 512KB for DMA, can be skipped if needed
         if (base<_512KB) {
            length -= _512KB;
            base = _512KB;
            dmaMemory512Kb = 0;  // NOTE: save page 0 later!
         }
            
         // Split the area where the kernel is mapped
         else if (base <= KERNEL_PHYSICAL && KERNEL_PHYSICAL <= (base+length)) {
            biosmemmap[-1].baselo = base;
            biosmemmap[-1].lenlo  = PBT_PHYSICAL - base;
            biosmemmap[0].baselo  = PBT_PHYSICAL + (4+kernelpages) * sizeof(Page);
            biosmemmap[0].lenlo   = length - (4+kernelpages) * sizeof(Page);
            --biosmemmap;
            // DPRINTK( "->patched" );
            continue;
         }

         totalPhysicalMemory += length;
                   
         // only support whole pages...
         while (length>=sizeof(Page)) {
          
            // Find order of this region, possibly multiple parts
            u32 o = 0;
            u32 s = 0x1000;
            for (u32 hlen=length/2; s<=hlen; s<<=1, ++o) {
               // Order cannot be bigger than base alignment
               if (base & s) break;
            }
            // DPRINTK( "Adding region base=%x order=%d\n", base, o );

            pages.free( base | 1, o ); // set present bit!
            length -= s;
            base += s;
         }            
      }         
      ++biosmemmap;
   }
   // pages.initDone(); // mark max available memory, moved to freeInitPages
      
   //DPRINTK( "Physical memory is %x(%d) bytes\n", totalPhysicalMemory,totalPhysicalMemory  );
   // pages.showMRUUnits();

   // Added: save BIOS HDD parameters collected in bootphase2
   //struct BIOSHDDINFO {
   //   u16 size;
   //} *info = (BIOSHDDINFO*) 0x7800;
   //DPRINTK( "\nHDD INFO[0].size = %u", info->size );
   memcpy_aligned( bioshddinfo, (void*) 0x7800, sizeof(bioshddinfo) );
              
   // Fix up first Mb tables
   PageDirectory &lowmem = PageDirectory::getDir( 0 );
   lowmem.asPage()->clear();

   // page 0 is for the BIOS (read only)
   lowmem[0].map( 0, 0 );     // not-present to catch null ptrs

   // map EBDA
   lowmem[0x9F].map( 0x9F000, E_PAGE_SHAREABLE | E_PAGE_READONLY );

   // also map video ROMS & BIOS32
   for ( u32 video = 0xa0; video < 0x100; ++video )
   {
      // non-cacheable, user_accessible for V86 mode BIOS calls (parts could be read-only...)
      lowmem[video].map( video << 12, E_PAGE_MMIO | E_PAGE_USER );
   }

   // Set the PDT entry for 0-4Mb linear memory to allow user-mode access, since 
   // VM86 always runs at CPL=3 (and needs to call BIOS functions there)
   PBT::getCurrentPBT()[0].setAttributes( E_PAGE_KERNEL | E_PAGE_USER );
   
   // here? Why not..
   Processor::enableGlobalPages();
   
   return true;
}

// virtual 
physadr_t 
IPhysicalMemoryImpl::mapRegion( linadr_t startaddr, size_t order, pteflags_t atts )
{
   // ASSERTION( framecount > 1, E_ERROR );  // use 1 frame method for this

   physadr_t frame = pages.allocate( order );   // bit 0 is set
   // DPRINTK( "\nIPhysicalMemoryImpl mapRegion got=%x", frame );
   CHECKPOINT('+');
   
   // DPRINTK( "\nPhysical::mapRegion: got page %x order=%d", frame, order );
   if (frame) {
      // this mapping can span multiple directories !
      for ( u32 i=(u32)startaddr, framecount=1UL<<order; framecount--; i += _4KB, frame += _4KB )
      {
         // Mapping should be null, if not we have a problem! Bug in allocationMap?
         register linadr_t a = (linadr_t) i;
         PageDirectory& dir = PBT::getDirectory( a ); // gets mapped if needed
         ASSERTIONv( !dir[ getPDTIndex(a) ].isMapped(), E_ERROR, dir[ getPDTIndex(a) ].getMapping() );
         dir[ getPDTIndex(a) ].map( frame, atts );
         // ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, frame );
      }
      return (frame - 1 - _4KB); // bit 0 is set, clear it!
   } else { // frame==0
      FATAL_ERROR( "Not enough frames for range!" );
      return 0;
   }
}

// virtual 
void 
IPhysicalMemoryImpl::releaseRegion( linadr_t start, size_t order )
{
   // DPRINTK( "\nPhysical::releaseRegion: linstart %x order=%d", start, order );
   
   // clear memory when freeing, starting with the first page
   memset_aligned( start, 0, _4KB );
      
   // Keep address of first frame released, assumes continuous block
   physadr_t first = PageDirectory::getDir( start )[ getPDTIndex(start) ].unmap();
   for (u32 i=(u32)start + _4KB, framecount=(1UL<<order); --framecount; i+=_4KB )
   {
      memset_aligned( (linadr_t) i, 0, _4KB );
      PageDirectory::getDir( (linadr_t) i )[ getPDTIndex((linadr_t) i) ].unmap();
   }

   {
      // Locking::LockHelper locker( memlock );
      pages.free( first, order );
   }   
   CHECKPOINT('-');   
   // DPRINTK( "\nPhysical::releaseRegion: freed first %x order=%d", first, order );
}

// virtual
u32    // counts the number of available physical frames
IPhysicalMemoryImpl::getFreeFrames() const
{
   return pages.getAvailableUnits();
}


// virtual
myos_result_t
IPhysicalMemoryImpl::listTables( IO::OStream &o ) const
{
   u32 start = o.getMarker();
   o.printf( "\n<MEMORY physical=\"%d\" freeframes=\"%d\">", 
      totalPhysicalMemory, getFreeFrames() );
   u32 count = 0;
   for (u32 i=0; i<1024; ++i) {
      PageDirectory &dir = PageDirectory::getDir( (linadr_t) (i*_4MB) );
      if (PBT::isMapped(dir)) count += dir.listTable(o);
   }
   o.printf( "\n<TOTAL pages=\"%d\"/>\n</MEMORY>", count );
   return (myos_result_t) (o.getMarker() - start);
}

// virtual 
myos_result_t
IPhysicalMemoryImpl::getStatistics( IO::OStream& out ) const {
   out.printf( "\nTotal pages: %d\nAvailable pages: %d",
      pages.getTotalUnits(), pages.getAvailableUnits() );
   return E_MYOS_SUCCESS;      
}


// virtual 
physadr_t 
IPhysicalMemoryImpl::getPhysicalAddress( linadr_t ofAddr ) const
{
   // NOTE: THis is slightly different from MyOS2: The lower 12 bits are
   // cleared. Alternatively, any offset bits of 'ofAddr' could be added,
   // or paging bits set for that linear address
   //
   // YES! That is actually needed for DMA to work!
   //
   return (u32) PageDirectory::getDir(ofAddr)[ getPDTIndex(ofAddr) ].getMapping() | getOffset(ofAddr);  
}

// virtual
void
IPhysicalMemoryImpl::mapMMIO( linadr_t linear, physadr_t physicalStart, u32 pageCount )
{
   // Assume linear and physical regions are consecutive
   // Assume linear memory does not cross 4MB border
   PageDirectory& dir = PageDirectory::getDir(linear);
   int end = getPDTIndex(linear) + pageCount;
   for (int i=getPDTIndex(linear); i < end; ++i) {
      dir[ i ].map( physicalStart, E_PAGE_MMIO );
      physicalStart += _4KB;
   }
}

// virtual
void
IPhysicalMemoryImpl::unmapMMIO( linadr_t linear, u32 pageCount )
{
   // Assume linear and physical regions are consecutive
   // Assume linear memory does not cross 4MB border
   PageDirectory& dir = PageDirectory::getDir(linear);
   int end = getPDTIndex(linear) + pageCount;
   for (int i=getPDTIndex(linear); i < end; ++i) {
      dir[ i ].unmap();
   }
}

// Called after sysinit is done, cannot itself be in INITSECTION (!)
// unless a stack-copy of the code is used 
void
IPhysicalMemoryImpl::freeInitPages()
{
   // skip it for debugging... YEP, Something gets caught...
   // return;
   
   // For simplicity, free each page individually...
   
   // first 2 pages are already freed in DoDecompress...
   PageDirectory& dir = PageDirectory::getDir( (linadr_t) &_COMPRESSED_START_LINEAR_ );
   for (linadr_t l = (linadr_t) &_COMPRESSED_START_LINEAR_; 
            l < (linadr_t) &_START_OF_CODE_; (u32&) l += _4KB )
   {            
      // clear it!
      memset_aligned( l, 0, _4KB );
      pages.free( dir[ getPDTIndex(l) ].unmap(), 0 );
   }
   DPRINTK( "\nFreed %d INIT pages from %x to %x", 
      ((u32) &_START_OF_CODE_ - (u32) &_COMPRESSED_START_LINEAR_) >> 12,
      (u32) &_COMPRESSED_START_LINEAR_, (u32) &_START_OF_CODE_
   );
   pages.initDone();
}

// virtual 
physadr_t 
IPhysicalMemoryImpl::mapDMA( linadr_t startaddr, u32 order )
{
   ASSERTION( order <= 7, E_ERROR );   // impl. defined max (1<<7) * 4Kb   
   
   // first 512KB is reserved in init(), for now simple add
   physadr_t frame = dmaMemory512Kb, start = frame;
   dmaMemory512Kb += (1UL<<(order+12));  // for next request, could be atomic

   // this mapping can span multiple directories !
   for ( u32 i=(u32)startaddr; frame < dmaMemory512Kb; i += _4KB, frame += _4KB )
   {
      // Mapping should be null, if not we have a problem! Bug in allocationMap?
      register linadr_t a = (linadr_t) i;
      PageDirectory& dir = PBT::getDirectory( a ); // gets mapped if needed
      ASSERTIONv( !dir[ getPDTIndex(a) ].isMapped(), E_ERROR, dir[ getPDTIndex(a) ].getMapping() );

/**
 * Apparently, the x86 is cache-coherent for DMA, and I don't need to 
 * disable the cache!
 */
      dir[ getPDTIndex(a) ].map( frame, E_PAGE_KERNEL );
      // DPRINTK( "\nmap(%x)", frame );

      // ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, frame );
   }
   
   return start;
}

// virtual 
void 
IPhysicalMemoryImpl::unmapDMA( linadr_t start, u32 order )
{
   for (u32 i=(u32)start, framecount=(1UL<<order); --framecount; i+=_4KB )
   {
      // memset_aligned( (linadr_t) i, 0, _4KB ); Dont bother...
      PageDirectory::getDir( (linadr_t) i )[ getPDTIndex((linadr_t) i) ].unmap();
   }
   dmaMemory512Kb -= (1UL<<order);  // assuming reverse order of releases...
}

// virtual
void
IPhysicalMemoryImpl::setAttributes( linadr_t forAdr, size_t pages, pteflags_t flags ) const
{
   // DPRINTK( "\nsetAttributes adr=%x", forAdr );
   
   // Depending on state transitions, this might be needed (write back & flush)
   // if (flags & (E_PAGE_NOCACHE|E_PAGE_WRITETHROUGH)) {
   // PTE::flushcache();
   // }
   PageDirectory& dir = PBT::getDirectory( forAdr );  // maps if needed
   for ( u32 i = getPDTIndex(forAdr); pages; --pages, ++i )
   {
      // XX this assumes all PTEs lie within the same dir...         
      dir[i].setAttributes( flags );
   }
}



// virtual
void
IPhysicalMemoryImpl::copyMapping( linadr_t dst, linadr_t src, size_t pagecount, pteflags_t atts )
{
   DPRINTK( "\ncopyMapping dst=%x src=%x", dst, src );   
   
// PTE bits for caching should be (and remain) identical, but other bits 
// may be changed for the copy
#define ALLOWED_BITS ( E_PAGE_USER | E_PAGE_ACCESSED | E_PAGE_DIRTY \
                     | E_PAGE_AVAIL0 | E_PAGE_AVAIL1 | E_PAGE_AVAIL2 \
                     | E_PAGE_PRESENT | E_PAGE_READWRITE | E_PAGE_GLOBAL)      
 
   ASSERTION( (atts & ~ALLOWED_BITS) == 0, E_ERROR );
 
   /// the directories should already be mapped (at least src...)
   PageDirectory &srcdir  = PageDirectory::getDir( src );
   PageDirectory &destdir = PageDirectory::getDir( dst );

   PTE* psrc = &srcdir[ getPDTIndex(src) ];
   PTE* pdes = &destdir[ getPDTIndex(dst) ];

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
  
}} // namespace
