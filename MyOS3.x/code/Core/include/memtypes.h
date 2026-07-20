#ifndef MEMTYPES_H
#define MEMTYPES_H

#include "types.h"

namespace MyOS {

/// Namespace for memory management related functionality
namespace MM {

   // Need to be able to use '&' for masking, and '+'
   typedef void* linadr_t;

   typedef u32 physadr_t;

   /**
    * Represents the 12 attribute bits available for PTEs
    */
   enum PTEFLAGS /* From "Alles over PC hardware, p311 and Intel docs */
   {
        E_PAGE_READONLY     = 0,
        E_PAGE_PRESENT      = (1<<0),   //< Marks the page as present
        E_PAGE_READWRITE    = (1<<1),   //< Marks the page as read/write
        E_PAGE_USER         = (1<<2),   //< If clear, page is not accessible (read/write) when CPL==3
        E_PAGE_WRITETHROUGH = (1<<3),   //< Enables write-through caching for this page
        E_PAGE_NOCACHE      = (1<<4),   //< Disables caching for this page
        E_PAGE_ACCESSED     = (1<<5),   //< Indicates the page has been read (since clearing this bit)
        E_PAGE_DIRTY        = (1<<6),   //< Indicates the page has been written to
        E_PAGE_4MB          = (1<<7),   //< Indicates this is a large page
        E_PAGE_GLOBAL       = (1<<8),   //< Indicates this page is global, i.e. not flushed upon TLB flush
        E_PAGE_AVAIL0       = (1<<9),
        E_PAGE_AVAIL1       = (1<<10),
        E_PAGE_AVAIL2       = (1<<11),

        // combinations of bits
        E_PAGE_SHAREABLE   = E_PAGE_GLOBAL    | E_PAGE_PRESENT,
        E_PAGE_KERNEL      = E_PAGE_SHAREABLE | E_PAGE_READWRITE,
        E_PAGE_MMIO        = E_PAGE_KERNEL
                           | E_PAGE_WRITETHROUGH
                           | E_PAGE_NOCACHE,
        E_PAGE_STATBITS    = E_PAGE_ACCESSED  | E_PAGE_DIRTY,

        E_PAGE_BITMASK      = 0xFFFFF000
    };

   // Actually only 12 bits, but for passing them around...
   typedef u32 pteflags_t;

   /// Descriptor privilege level
   enum EDPL { E_DPL0 = 0x0, E_DPL1 = 0x1, E_DPL2 = 0x2, E_DPL3 = 0x3 };

   // allow GCC to optimize where possible...
   inline u32 getOffset( linadr_t a )	CONSTFUNC;
   inline u32 getPDTIndex( linadr_t a ) CONSTFUNC;
   inline u32 getPBTIndex( linadr_t a ) CONSTFUNC;

   // Decomposition of a linear address into bits used for paging
   inline u32 getOffset( linadr_t a )    { return ((u32)a&0x00000FFF); }
   inline u32 getPDTIndex( linadr_t a )  { return ((u32)a&0x003FF000)>>12; }
   inline u32 getPBTIndex( linadr_t a )  { return ((u32)a>>22); }

   /**
    * Thrown when insufficient memory resources are available
    */
   class OutOfMemoryException {
      const char* location;
   public:
      inline OutOfMemoryException( const char* l ) throw() : location(l) {}
      inline const char* getLocation() const throw() { return location; }
   };

   /**
    * MyOS supports up to 32 paging devices (swap files or partitions, floppy disks, network
    * device), each supporting up to 2^24 4K pages for a total of 1Tb
    */
   typedef u8 paging_device_id_t;

   class PagingException
   {
   public:
       enum E_REASON
       {
           E_NULL_POINTER = 0,
           E_INVALID_READ = 1,
           E_INVALID_WRITE = 2,
       };

       inline PagingException(E_REASON r, linadr_t _eip, linadr_t lin,
               physadr_t phys) throw() :
           reason(r), linear(lin), physical(phys), eip(_eip)
       {
       }

   private:
       E_REASON reason;
       linadr_t linear;
       physadr_t physical;
       linadr_t eip;
   };

}}	// namespace

#endif
