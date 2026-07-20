#include "IVirtualMemoryImpl.h"
#include "IScriptable.h"

#include "new.h"
#include "Boot/memlayout.h"

#include "PageDirectory.h"
#include "PBT.h"
#include "Page.h"
#include "IContext.h"

#include "OStream.h"
#include "Atomic/LockHelper.h"

#include "debug.h"
#define FATAL_ERROR(e) ASSERTION(e==0,E_EMERGENCY)

// for IPaging impl
#include "IInterruptHandling.h"
using namespace MyOS::InterruptHandling;
#include "MMComponent.h"
#define PHYS MMComponent::getInstance().impl1

// BIOS hdd info collected in bootphase2, then copied here
u8 bioshddinfo[ 8 * 0x42 ];   // in .rodata (see memlayout.h)

/**
 * Implementation for physical MM
 */
namespace MyOS { namespace MM {

inline bool crosses4MBBorder( linadr_t a, u32 pages ) {
    return (((u32)a)>>24) != (((u32)a+(pages<<12))>>24);
}

bool
IVirtualMemoryImpl::initPhysical( Context::IContext& context, NVPAIR params[] )   // INITSECTION
{
   // here? Why not..
   Processor::enableNiceFeatures();

   size_t kernelpages = ((u32) &_KERNEL_PAGES_) + extraPagesAllocated;
   DPRINTK( "\n#Kernelpages: %u module: %u", kernelpages, extraPagesAllocated );

   // Start allocating pages after the kernel
   physadr_t target = KERNEL_PHYSICAL + (kernelpages-1) * sizeof(Page);

   // Pick next linear address for bitmap, assume PDT is already mapped
   linadr_t bitmapAddress = (linadr_t) &_FREE_KERNEL_SPACE_START_;

   PageDirectory& pooldir = PageDirectory::getDir( bitmapAddress );
   DPRINTK( "\nAllocating %d pages for bitmap starting at %x phys=%x",
           ALLOCATOR::getPageCount(), bitmapAddress, target+sizeof(Page) );

   size_t lastPageIndex = getPDTIndex( bitmapAddress );
   for (u32 f = 0; f<ALLOCATOR::getPageCount(); ++f) {
      pooldir[ lastPageIndex+f ].map( target+=sizeof(Page), E_PAGE_KERNEL );
      ++kernelpages;
   }
   pages.init( bitmapAddress );  // now we can clear those pages

   // Make target point at first free page
   target += sizeof(Page);

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

   // GRUB uses slightly different structure with extra 'size' field
   if ((*(u32*)biosmemmap)==20) biosmemmap = (BIOSMEMMAP*) (((u32*)biosmemmap)+1);
   BIOSMEMMAP *prev = biosmemmap;
   while (biosmemmap->baselo != 0xFFFFFFFF) {   // phase2 puts -1 at end
      //DPRINTK( "BIOS reports memory base=%x len=%x type=%d\n",
      //        biosmemmap->baselo, biosmemmap->lenlo, biosmemmap->type );

      if (biosmemmap->type == 1) {  // available memory ?
         u32 base   = biosmemmap->baselo;
         u32 length = biosmemmap->lenlo;

         // Save lowest 512KB for DMA, can be skipped if needed
         if (base<_512KB) {
            length -= _512KB;
            base = _512KB;
            dmaMemory512Kb = 0x1000;  // NOTE: don't issue page 0!
         }

         // Split the area where the kernel is mapped
         else if (base <= PBT_PHYSICAL && (PBT_PHYSICAL < (base+length))) {
            prev->baselo = base;
            prev->lenlo  = PBT_PHYSICAL - base; // piece from base to start of kernel
            prev->type = 1;
            biosmemmap->baselo  = target;   // first free page
            biosmemmap->lenlo   = (base+length) - biosmemmap->baselo;

            biosmemmap = prev;  // support both GRUB and non-GRUB
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
         // DPRINTK( "---- base=%x ----\n", base );
      }
      prev = biosmemmap;
      ++biosmemmap;

      // GRUB uses slightly different structure with extra 'size' field
      if ((*(u32*)biosmemmap)==20) biosmemmap = (BIOSMEMMAP*) (((u32*)biosmemmap)+1);
   }
   // pages.initDone(); // mark max available memory, moved to freeInitPages

   // DPRINTK( "Physical memory is %x(%d) bytes\n", totalPhysicalMemory,totalPhysicalMemory  );
   // pages.showMRUUnits();
   // while(1);

   // Added: save BIOS HDD parameters collected in bootphase2
   //struct BIOSHDDINFO {
   //   u16 size;
   //} *info = (BIOSHDDINFO*) 0x7800;
   //DPRINTK( "\nHDD INFO[0].size = %u", info->size );
   memcpy( bioshddinfo, (void*) 0x7800, sizeof(bioshddinfo) );

   // Fix up first Mb tables
   PageDirectory &lowmem = PageDirectory::getDir( 0 );
   lowmem.asPage()->clear();

   // page 0 is for the BIOS (read only)
   lowmem[0].map( 0, 0 );     // not-present to catch null ptrs
   // lowmem[0].map( 0, E_PAGE_SHAREABLE | E_PAGE_READONLY );     // present to scan for _MP_

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

   // Unmap guard page for the initial thread, assuming its still on last page
   // u32 guardPage = (((u32) Processor::ESP()) & 0xFFFFF000) - 0x1000;
   // this->unmapMMIO( (linadr_t) guardPage, 0 );

   // Now done by IInterruptHandlingImpl
   // IInterruptHandling *ih = (IInterruptHandling*) &context.lookup(myos_name_t(IInterruptHandling::ID()) );
   // ih->setTrapHandler( E_PAGE_FAULT, (myos_TRAP_f)pagefaultAsm, E_ENABLE_INTS | E_SYNCHRONOUS );

   return true;
}

// Called after sysinit is done, cannot itself be in INITSECTION (!)
// unless a stack-copy of the code is used
void
IVirtualMemoryImpl::freeInitPages()
{
    // skip it for debugging... YEP, Something gets caught...
    // return;

    // For simplicity, free each page individually...

   // first 2 pages are already freed in DoDecompress...
   Page *l = (Page*) &_COMPRESSED_START_LINEAR_;  // linker variable
   Page *end = (Page*) &_START_OF_CODE_;
   PageDirectory& dir = PageDirectory::getDir( (linadr_t) l );
   for (; l < end; ++l )
   {
       l->clear();  // clear it!
       physadr_t page = dir[ getPDTIndex( (linadr_t) l) ].unmap();
       pages.free( page, 0 );
   }
   DPRINTK( "\nFreed %d INIT pages from %x to %x",
      ((u32) &_START_OF_CODE_ - (u32) &_COMPRESSED_START_LINEAR_) >> 12,
      (u32) &_COMPRESSED_START_LINEAR_, (u32) &_START_OF_CODE_
   );
   pages.initDone();
}

// virtual
physadr_t
IVirtualMemoryImpl::mapRegion( linadr_t startaddr, size_t order, pteflags_t atts )
    throw (MM::OutOfMemoryException)
{
    // Address should be on a page boundary, though it gets clipped anyway
    ASSERTIONv( (((u32) startaddr) & 0xFFF)==0, E_WARNING, startaddr );

   // ASSERTION( framecount > 1, E_ERROR );  // use 1 frame method for this
   // DPRINTK( "\nIPhysicalMemoryImpl mapRegion order=%x", order );
   physadr_t frame = pages.allocate( order );   // bit 0 is set
   // ASSERTION( (frame&0xfffff000) != 0x25f000, E_CRITICAL );
   // DPRINTK( "\nIPhysicalMemoryImpl mapRegion(%x o=%u) got=%x", startaddr, order, frame );
   CHECKPOINT('+');

   // DPRINTK( "\nPhysical::mapRegion: got page %x order=%d", frame, order );
   if (frame) {
      // this mapping can span multiple directories !
      for ( u32 i=(u32)startaddr, framecount=1UL<<order; framecount--; i += _4KB, frame += _4KB )
      {
         // Mapping should be null, if not we have a problem! Bug in allocationMap?
         register linadr_t a = (linadr_t) i;
         PageDirectory& dir = PBT::getDirectory( a ); // gets mapped if needed
         ASSERTIONv( !dir[ getPDTIndex(a) ].isMapped(), E_WARNING, a /*dir[ getPDTIndex(a) ].getMapping()*/ );
         dir[ getPDTIndex(a) ].map( frame, atts );
         // ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, frame );
      }
      return (frame - 1 - _4KB); // bit 0 is set, clear it!
   } else { // frame==0, not enough contigous frames for mapRegion => allocate individual frames
       for ( u32 i=(u32)startaddr, framecount=1UL<<order; framecount--; i += _4KB )
       {
          // Allocate a single frame per iteration
          frame = pages.allocate( 1 );
          if (!frame) {

              // Recover previously allocated frames! cannot use releaseRegion :(
              for ( ; i>=(u32)startaddr; i -= _4KB ) {
                  PageDirectory& dir = PBT::getDirectory( (linadr_t) i );
                  frame = dir[ getPDTIndex( (linadr_t) i ) ].unmap();
                  pages.free( frame, 1 );
              }
              throw OutOfMemoryException( __FILE__ );
          }

          // Mapping should be null, if not we have a problem! Bug in allocationMap?
          register linadr_t a = (linadr_t) i;
          PageDirectory& dir = PBT::getDirectory( a ); // gets mapped if needed
          ASSERTIONv( !dir[ getPDTIndex(a) ].isMapped(), E_ERROR, dir[ getPDTIndex(a) ].getMapping() );
          dir[ getPDTIndex(a) ].map( frame, atts );
          // ASSERTIONv( ((Page*)a)->isZero(), E_ERROR, frame );
       }


   }
}

// virtual
void
IVirtualMemoryImpl::releaseRegion( linadr_t start, size_t order )
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
IVirtualMemoryImpl::getFreeFrames() const
{
   return pages.getAvailableUnits();
}

// virtual
Core::IScriptable*
IVirtualMemoryImpl::getScriptable() const throw()
{
    static class Impl : public Core::IScriptable {

        const IVirtualMemoryImpl &intf;

    public:
        inline Impl( const IVirtualMemoryImpl &i )
            : IScriptable( i.getComponent(), VERSION(1,0), "IPhysicalMemory-Script" ), intf(i) {}

        virtual const buffer getIDL() const throw()
        {
            return buffer( "<xml>todo</xml>" );
        }

        virtual myos_result_t get( const char* const call,
                NVPAIR parameters[], IO::OStream& output ) const
        {
            DPRINTK( "\nIVirtualMemoryImpl::Script:%s", call );
            if (strncmp(call,"idl",3)==0) {
                return intf.listTables( output );   // should be getIDL
            }
            return E_MYOS_E_NOTIMPL;
        }
    } scriptable( *this );
    return &scriptable;
}

// virtual
myos_result_t
IVirtualMemoryImpl::listTables( IO::OStream &o ) const
{
   u32 start = o.getMarker();
   o.printf( "<?xml version='1.0' encoding='UTF-8'?>\n" );
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
physadr_t
IVirtualMemoryImpl::getPhysicalAddress( linadr_t ofAddr ) const
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
pteflags_t
IVirtualMemoryImpl::getAttributes( linadr_t ofAddr ) const
{
    return (u32) PageDirectory::getDir(ofAddr)[ getPDTIndex(ofAddr) ].getAttributes();
}

void
IVirtualMemoryImpl::mapMMIOPhysical( linadr_t linear, physadr_t physicalStart, u32 pageCount )
{
    ASSERTIONv( physicalStart > totalPhysicalMemory, E_ERROR, physicalStart );
    ASSERTION( !crosses4MBBorder(linear,pageCount), E_ERROR );
    ASSERTION( pageCount>0, E_ERROR );

    //DPRINTK( "\nphys::mapMMIO lin=%x phys=%x pages=%d",
    //           linear, physicalStart, pageCount );

   // Assume linear and physical regions are consecutive
   // Assume linear memory does not cross 4MB border
   PageDirectory& dir = PageDirectory::getDir(linear);
   int start = getPDTIndex(linear);
   int end = start + pageCount;
   for (int i=start; i < end; ++i) {
      dir[ i ].map( physicalStart, E_PAGE_MMIO );
      physicalStart += _4KB;
   }
}

void
IVirtualMemoryImpl::unmapMMIOPhysical( linadr_t linear, u32 pageCount )
{
    ASSERTION( !crosses4MBBorder(linear,pageCount), E_ERROR );

   // Assume linear and physical regions are consecutive
   // Assume linear memory does not cross 4MB border
   PageDirectory& dir = PageDirectory::getDir(linear);
   int end = getPDTIndex(linear) + pageCount;
   for (int i=getPDTIndex(linear); i < end; ++i) {
      dir[ i ].unmap();
   }
}



// virtual
physadr_t
IVirtualMemoryImpl::mapDMA( linadr_t startaddr, u32 order )
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
      ASSERTIONv( !dir[ getPDTIndex(a) ].isMapped(), E_WARNING, dir[ getPDTIndex(a) ].getMapping() );

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
IVirtualMemoryImpl::unmapDMA( linadr_t start, u32 order )
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
IVirtualMemoryImpl::setAttributes( linadr_t forAdr, size_t pages, pteflags_t flags ) const
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
IVirtualMemoryImpl::fillEntries( linadr_t startaddr, size_t pages, u32 bits, u32 increment )
{
    PageDirectory& dir = PBT::getDirectory( startaddr );  // maps if needed
    for ( u32 i = getPDTIndex(startaddr); pages; --pages, ++i, bits += increment )
    {
        // XX this assumes all PTEs lie within the same dir...
        dir[i].map( bits );
    }
}


// virtual
void
IVirtualMemoryImpl::copyMapping( linadr_t dst, linadr_t src, size_t pagecount, pteflags_t atts )
{
   // DPRINTK( "\ncopyMapping dst=%x src=%x", dst, src );

// PTE bits for caching should be (and remain) identical? other bits
// may be changed for the copy
#define ALLOWED_BITS ( E_PAGE_USER | E_PAGE_ACCESSED | E_PAGE_DIRTY \
                     | E_PAGE_AVAIL0 | E_PAGE_AVAIL1 | E_PAGE_AVAIL2 \
                     | E_PAGE_PRESENT | E_PAGE_READWRITE | E_PAGE_GLOBAL | E_PAGE_NOCACHE)

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

// virtual
void
IVirtualMemoryImpl::clearMapping( linadr_t dst, size_t pagecount )
{
   PageDirectory &destdir = PageDirectory::getDir( dst );   // assumes at most 4Mb

   PTE* pdes = &destdir[ getPDTIndex(dst) ];

   memset_aligned( pdes, 0, pagecount*sizeof(PTE) /* bytes */ );
   PTE::invalidateAll();
}

/***
 * Paging impl
 */
PagingDevice* IVirtualMemoryImpl::devices[32] = { 0, };

/* TODO implement IPaging
struct IPagingImpl : public IPaging

    IPagingImpl( ) : IPaging( c, VERSION(1,0) )

    virtual paging_device_id_t registerPagingDevice( PagingDevice &callback );
    virtual void unregisterPagingDevice( paging_device_id_t id );
    virtual linadr_t mapRegion( u32 firstIndex, size_t nPages, bool readOnly,
        paging_device_id_t device );
} */


ASM (
".globl PFASM        \n"
".align 16           \n"
"PFASM:              \n"
"pushal              \n" // Could optimize here: EAX, ECX + used_regs(pagefault)
"pushw %ds           \n" // Save current DS,ES ( can be user mode selectors)
"pushw %es           \n"
"movw %ss, %ax       \n" // Load DPL0 data selector
"movw %ax, %ds       \n"
"movw %ax, %es       \n"
"movl %cr2, %eax     \n" // linear address of PF, first param [EAX]
"movl 36(%esp), %edx \n" // error code, second param [EDX]
"call pagefault      \n"
"popal               \n"
"popw %es            \n"
"popw %ds            \n"
"addl $4, %esp       \n" // remove errorcode
"iretl               \n"
);

// static
void
IVirtualMemoryImpl::pagefault( linadr_t adr, u32 ec ) throw (PagingException)
{
   enum E_ERRORBITS {
      E_PAGEPROTECTION  = (1<<0),   //< As opposed to a non-present page
      E_WRITE           = (1<<1),
      E_USERMODE        = (1<<2),
      E_RSVDBIT         = (1<<3),   //< Caused by a reserved bit in page table
   };

   // Catch NULL pointer references (first page)
   if ((u32)adr < 0x1000) doThrow(adr,0);

   // TODO Check that the directory is already mapped! This codes fits better in physical
   PageDirectory& dir = PageDirectory::getDir( adr );
   if (!PBT::isMapped(dir)) {
      doThrow(adr,1);
   }

   u32 pte = dir[ getPDTIndex(adr) ].getValue();
   DPRINTK( "\nPagefault CR2=%x errorcode=%x pte=%x", adr, ec, pte );

   // Catch unmapped pages
   if (pte==0) {
       doThrow(adr,2);
   } else if (( ec & E_WRITE ) && (pte&2)) {
       doThrow(adr,3);  // write to read-only entity
   } else if ( (pte & 0xFE) == 0 ) {
       // Paging device #0 = demand-allocated pages
       switch ( pte >> 8 )
       {
       case 1:  // set by mmap()
           {
               physadr_t page = PHYS.mapRegion( adr, 0, E_PAGE_KERNEL | (pte&2) );
               if (page==0) doThrow(adr,4);
               return;
           }

       default:
           doThrow(adr,5);
       }
   } else {
       PagingDevice* dev = devices[ (pte>>2) & 0x1f ];
       if (dev) {
           physadr_t page = PHYS.mapRegion( adr, 0, E_PAGE_KERNEL | (pte&2) );
           if (page==0) doThrow(adr,4);
           dev->loadPage( pte>>8, adr );
       } else doThrow(adr,5);
   }
}

// I dont want this one inline
// static, NORETURN
void
IVirtualMemoryImpl::doThrow( void* adr, int cause ) throw (PagingException)
{
#ifdef DEBUG
   register u32 eip = ((u32*) Processor::ESP())[29];   // stackframe, see .asm: esp - 0x10, 4 * PUSH
   //for ( int p = 18; p<30; ++p ) {
   //  PRINTK( "\nstack[%d]=%x", p, ((u32*) Processor::ESP())[p] ); 
   //}
#else   // 26 ?
   register u32 eip = ((u32*) Processor::ESP())[27];   // minus stackframe (0x18)
#endif

   // debugging only
   switch (cause)
   {
   case 0: PRINTK( "\nNULL PTR EXCEPTION at EIP=%x attempting address=%x", eip, adr ); break;
   case 1: PRINTK( "\nPagefault - unmapped DIR  adr=%x EIP=%x", adr, eip ); break;
   case 2: PRINTK( "\nPagefault - unmapped PAGE adr=%x EIP=%x", adr, eip ); break;
   case 3: PRINTK( "\nWrite to read-only DPE adr=%x EIP=%x", adr, eip ); break;
   case 4: PRINTK( "\nUnable to allocate page on-demand" ); break;
   case 5: PRINTK( "\nNon-existing paging device" ); break;
   }
   throw PagingException( (PagingException::E_REASON) cause, (linadr_t) eip, adr, 0 /* todo: physical? */ );
}

}} // namespace
