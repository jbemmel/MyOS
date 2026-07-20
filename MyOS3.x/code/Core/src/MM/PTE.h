#ifndef PTE_H
#define PTE_H

#include "memtypes.h"
#include "Atomic/atomic32.h"

namespace MyOS {

namespace IO {
    class OStream;
}

namespace MM
{
   class Page;

   /// Represents a single pagetable entry
   class PTE
   {
   public:
       static void flush_TLB_IPI() ASMNAME( "PTE_flush_TLB_IPI" );

       /// constructor for use in array: clears
       inline PTE() throw() : pte(0) {}

       /// To construct on stack, then assign atomically
       inline PTE( physadr_t a, pteflags_t flags ) throw() : pte( (a|flags) ) {}

       inline PTE& operator =( const PTE& _pte ) {
           *((u32 *)this) = *((u32 *) &_pte);
           return *this;
       }

      /**
       * Maps this PTE onto a particular physical page, with given memory attribute settings
       *
       * @param to      :  Physical pageframe (20 bits) to map to
       * @param flags   :  Memory attributes (12 bits) to set @see PTEFLAGS
       */
      inline void map( physadr_t to, pteflags_t flags );
      inline int  mapifnull( physadr_t to, pteflags_t flags );

      // Map to demand-paged pointer, bit[0] must be 0 (not present)
      inline void map( u32 entity );

      // @return all bits of this PTE
      inline u32 getValue() const { return pte; }

      /**
       * Unmaps this PTE (atomically) and returns the old value
       * (including paging bits)
       */
      inline physadr_t unmap();

      inline pteflags_t getAttributes() const { return ((PTEDATA&) pte).bits; }
      inline physadr_t getMapping() const     { return (((u32) pte) & E_PAGE_BITMASK); }
      inline bool isMapped() const            { return getAttributes() & E_PAGE_PRESENT; }

      inline linadr_t getLinearAddress() const;

      /**
       * Returns the page mapped by this PTE, 0 if none
       */
      inline Page* getPage() const;

      inline void setAttributes( pteflags_t flags );

      // @todo: Perhaps move this one somewhere else (CPU class)
      // inline static void flushcache();
      static void invalidateAll();

      s32 list( IO::OStream &o, u32 rangecount ) const;

      inline void initialize( physadr_t to, pteflags_t flags );
   private:
       void invalidate();

       // map without flushing, for FramePoolManager in INIT and PBT::PBT()
       friend class FramePoolManager;
       friend class PBT;

       /// Bit structure of PTE
       struct PTEDATA {
           pteflags_t bits : 12;   // OR-ed flags
           physadr_t frame : 20;

           inline PTEDATA( physadr_t a, pteflags_t m ) : bits(m), frame(a) {}
           inline PTEDATA( u32 v = 0 )                 { *((u32*)this)=v; }
           inline void setAttributes( pteflags_t m )   { bits = m; }
       };

       ATOMIC32<PTEDATA> pte;
   };

   /**
    *  Invalidates the mapping of this page
    *
    *  This implementation assumes the PTEs are contained in arrays of 1024
    *  ptes (pagetables) starting at 0xFFC00000 -> (<<10) gives physical page frame
    */
#ifdef UNI_CPU
   inline void PTE::invalidate()
   {
       physadr_t physadr = ((u32) this) << 10;
       ASMVOLATILE( "invlpg (%0)" :  : "r"(physadr) );
   }
#endif

   inline void PTE::initialize( physadr_t to, pteflags_t flags )
   {
       // could check bits are zero
       *((u32 *) this) = to | flags;		// could force present (but not with unmap() !)
   }

   inline void PTE::map( physadr_t to, pteflags_t flags )
   {
       // if this->pte != 0 it means the PTE is already mapped
       // ...but this occurs at initialization
       // ASSERTIONv( pte == 0, E_ERROR, this );

       initialize( to, flags );
       invalidate();	   // Optimization: only need to invalidate if the PTE was valid
   }

   inline void PTE::map( u32 e )
   {
      // assuming it was 0, so no invalidation done here
      *((u32*) &pte) = e;
   }

   /// Maps this PTE atomically, but only if it's not already mapped
   inline int PTE::mapifnull( physadr_t to, pteflags_t flags )
   {
      // Usually pte!=0, so prevent expensive CmpXchg op
      if RARELY((u32)pte==0) {
         return pte.CmpXchg( 0, to | flags );
      } else return false;
      // Only need to invalidate if the PTE was valid!
   }

   /// @note Constant must match PageDirectory, but include creates circular ref
   inline Page* PTE::getPage() const
   {
       // 0xFFC00000 == &PageDirectory[0]
       return isMapped() ? (Page*) (((u32) this - 0xFFC00000) * _1KB) : 0;
   }

   inline void PTE::setAttributes( pteflags_t flags )
   {
       ((PTEDATA&) pte).setAttributes( flags );
       invalidate();
   }

   /// Unmaps this PTE and returns the physical frame that becomes available
   inline physadr_t PTE::unmap()
   {
       physadr_t old = pte.Replace(0);
       invalidate(); // assuming old!=0
       return old;
   }

   /* static, flushes the CPU cache (should be in CPU class...)
   inline void PTE::flushcache()
   {
       // unfortunately, not per page ! Make the page non-cacheable if needed
       ASMVOLATILE( "wbinvd" );
   }
   */

   /// invalidate all PTE mappings at once
#ifdef UNI_CPU
   inline void PTE::invalidateAll()
   {
       int r;
       ASMVOLATILE(
           "movl %%cr3, %0 \t\n"
           "movl %0, %%cr3"
           : "=r"(r) : : "memory"
       );
   }
#endif

}}   // namespace

#endif
