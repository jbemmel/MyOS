#ifndef MEMTYPES_H
#define MEMTYPES_H	

#include "types.h"

namespace MyOS {
namespace MM {

   // Need to be able to use '&' for masking, and '+'
   typedef void* linadr_t;
   
   typedef u32 physadr_t;

   /// Represents the 12 attribute bits available for PTEs
   enum PTEFLAGS /* From "Alles over PC hardware, p311 and Intel docs */
   {
        E_PAGE_READONLY     = 0,
        E_PAGE_PRESENT      = (1<<0), 
        E_PAGE_READWRITE    = (1<<1), 
        E_PAGE_USER         = (1<<2), 
        E_PAGE_WRITETHROUGH = (1<<3),
        E_PAGE_NOCACHE      = (1<<4), 
        E_PAGE_ACCESSED     = (1<<5), 
        E_PAGE_DIRTY        = (1<<6), 
        E_PAGE_4MB          = (1<<7),
        E_PAGE_GLOBAL       = (1<<8),
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
   inline u32 getOffset( const void* a )	CONSTFUNC;
   inline u32 getPDTIndex( const void* a )	CONSTFUNC;
   inline u32 getPBTIndex( const void* a )	CONSTFUNC;

   // Decomposition of a linear address into bits used for paging
   inline u32 getOffset( const void* a )    { return ((u32)a&0x00000FFF); }
   inline u32 getPDTIndex( const void* a )  { return ((u32)a&0x003FF000)>>12; }
   inline u32 getPBTIndex( const void* a )  { return ((u32)a>>22); }

}}	// namespace

#endif
