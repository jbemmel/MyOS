#include "IDT.h"
#include "CallGate.h"
#include "Devices/Processor/Processor.h"
#include "ihtypes.h"

namespace MyOS {
namespace InterruptHandling {

using namespace MM;

ATOMIC64<CallGate> IDT::IDTable[ MAX_IDT_ENTRIES ];

/// helper class to get/set the IDT address
class IDTAddress
{
	u16 __align__bogus;
	u16 limit;
	CallGate* address;

public:
    inline IDTAddress( CallGate* start, u16 nEntries ) {
	    limit = nEntries * sizeof(CallGate) - 1;	// in bytes, minus 1
	    address = start;
    }

   inline void load() {
      ASMVOLATILE( "lidt (%0)" : : "r"(&limit) : "memory" );
   }
} PACKED;


void
IDT::initialize()
{
   /// Point all interrupts at unmapped memory (i must be >8 bit)
   /// assumes linear address 0X00000000 is unmapped...
   CallGate unmapped( Processor::CS(), 0X00000000, E_DPL0, 0, true );
   for (u32 i=0; i<MAX_IDT_ENTRIES; ++i)
   {
      setVector( i, unmapped );
   }

   // Load the address of this IDT
   IDTAddress adr( start(), MAX_IDT_ENTRIES );
   adr.load();
}

myos_TRAP_f
IDT::setVector( index_t i, const CallGate& entry )
{
   // PRINTF( "\nIDT::setVector %X", i );

   /// Creates a gate in the current CS, override RPL in INTSegment ?
   // CallGate gate( INTSegment, (u32) handler, dpl, 0, runIntEnabled );

   myos_TRAP_f old = (myos_TRAP_f) ((CallGate&)IDTable[i]).offset();
   IDTable[i].assignAtomic( entry );
   return old;
}

}}  // namespace
