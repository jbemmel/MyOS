#include "../IDemandPagingImpl.h"
#include "InterruptHandling/ihtypes.h"		// E_PAGE_FAULT
#include "../Physical/PageDirectory.h"    	// move to this dir ?
#include "../Physical/PBT.h"    			// move to this dir ?

#include "MM/DemandPagingComponent.h"
#include "Processor/Processor.h"

#include "debug.h"

namespace MyOS { namespace MM {

using namespace InterruptHandling;
using Paging::DemandPagedEntity;

IDemandPagingImpl::IDemandPagingImpl( MyOS::Core::IComponent& c )
: IDemandPaging( c, VERSION(1,0) )
{

}

bool 
IDemandPagingImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	DemandPagingComponent::getInstance().iInterruptHandling->setTrapHandler( 
		E_PAGE_FAULT, (myos_TRAP_f)pagefaultAsm, E_ENABLE_INTS | E_SYNCHRONOUS
	);
	return true;
}

void 
IDemandPagingImpl::deinit( IContext& context )
{

}

// virtual 
linadr_t 
IDemandPagingImpl::map( Paging::DemandPagedEntity *first, size_t itemsize, size_t count, bool readonly )
{
   DPRINTK( "\nIDemandPaging::map first=%x itemsize=%d count=%d", 
      first, itemsize, count );
  
   // 1. In the context of current process (could be a param too...)
   //    allocate a virtual memory range 
   linadr_t adr = (linadr_t) (_1MB * 8);   // XX: TODO, implement this

  
   // XX Like this, it cannot cross a 4Mb boundary...
   PageDirectory& dir = PBT::getDirectory( adr );     // maps if needed
   u32 roFlag = readonly ? 0x2 : 0;    // use bit 1 for RO
   for (u32 e = getPDTIndex(adr), p = (u32) first; count > 0; --count, p += itemsize ) {
      dir[e++].map( p | roFlag );
   }
   DPRINTK( "-> Mapped to linear address %x", adr );
   return adr;
}

// virtual 
physadr_t 
IDemandPagingImpl::unmap( linadr_t page ) {
   PageDirectory& dir = PageDirectory::getDir( page );
   physadr_t r = dir[ getPDTIndex(page) ].unmap();     // assumes mapped!
   return r & E_PAGE_PRESENT ? r : 0;
}

ASM (
".globl PFASM        \n"
".align 16           \n"
"PFASM:              \n"
"pushal              \n" // Could optimize here: EAX, ECX + used_regs(pagefault)
"movl %cr2, %eax     \n" // linear address of PF, first param [EAX]
"movl 32(%esp), %edx \n" // error code, second param [EDX]
"call pagefault      \n"
"popal               \n"
"addl $4, %esp       \n" // remove ec
"iretl               \n"
);

// static
void
IDemandPagingImpl::pagefault( linadr_t adr, u32 ec ) throw (Exceptions::PagingException)
{
   enum E_ERRORBITS {
      E_PAGEPROTECTION  = (1<<0),   //< As opposed to a non-present page
      E_WRITE           = (1<<1),
      E_USERMODE        = (1<<2),
      E_RSVDBIT         = (1<<3),   //< Caused by a reserved bit in page table
   }; 

   // Catch NULL pointer references (first page)
   if ((u32)adr < 0x1000) doThrow(adr,0);
   
   // Check that the directory is already mapped!
   PageDirectory& dir = PageDirectory::getDir( adr );
   if (!PBT::isMapped(dir)) {
      doThrow(adr,1);
   }

   u32 pte = dir[ getPDTIndex(adr) ].getValue();

   // Catch unmapped pages
   if (pte==0) {
      doThrow(adr,2);
   }      

   DemandPagedEntity* dpe = (DemandPagedEntity*) (pte & ~2);
   DPRINTK( "\nPagefault CR2=%x errorcode=%x pte=%x", adr, ec, pte );

   // Check if it was mapped read-only, and a write was tried
   if (( ec & E_WRITE ) && (pte&2)) {
      doThrow(adr,3);         
   } else {
      // Note: This could return also some paging bits, eg read-only or MMIO
      physadr_t frame = dpe->fetchNow();
      // For smooth demand-paging, make sure everything is page-aligned!
      ASSERTIONv( (frame & 0xFFF)==0, E_ERROR, frame );
      DPRINTK( "\nData fetched @physical %x adr=%x pte=%x", frame, adr, pte );

      // TODO: atomic: map_if_eq(pte)
      // read-only if requested
      
      // Note: Could allow lowest 12 bits of 'frame' value | flags
      dir[ getPDTIndex(adr) ].map( frame /*& 0xFFFFF000*/, E_PAGE_KERNEL & ~(pte&2) );
   }      
}

// I dont want this one inline
// static, NORETURN 
void 
IDemandPagingImpl::doThrow( void* adr, int cause ) throw (Exceptions::PagingException)
{
#ifdef DEBUG 
   register u32 eip = ((u32*) Processor::ESP())[29];   // minus stackframe
#else	// 26 ?
   register u32 eip = ((u32*) Processor::ESP())[27];   // minus stackframe
#endif   
		
   // debugging only
   switch (cause)
   {
   case 0: PRINTK( "\nNULL PTR EXCEPTION at %x", eip ); break;    
   case 1: PRINTK( "\nPagefault - unmapped DIR  adr=%x EIP=%x", adr, eip ); break;
   case 2: PRINTK( "\nPagefault - unmapped PAGE adr=%x EIP=%x", adr, eip ); break;
   case 3: PRINTK( "\nWrite to read-only DPE adr=%x EIP=%x", adr, eip ); break;
   }
   throw Exceptions::PagingException(); 
}

}}  // namespaces
