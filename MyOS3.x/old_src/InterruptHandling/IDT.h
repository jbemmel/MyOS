#ifndef IDT_H
#define IDT_H

#include "defs.h"
#include "types.h"
#include "Atomic/atomic64.h"
#include "CallGate.h"
#include "MM/memtypes.h"
#include "ihtypes.h"    // myos_TRAP_f

namespace MyOS {

namespace InterruptHandling {

using MM::EDPL;

/// Global Interrupt Descriptor Table, contains <= 256 callgates to interrupt routines
class IDT
{
	friend class IInterruptHandlingImpl;

	/// Initializes the IDT (code in INITSECTION)
	static void initialize() INITSECTION;

public:
    /// Index into IDT, 0..255
    typedef u8 index_t;

    /**
     *  Installs an interrupt handler for INT i
     *
     *  @param i                -   #interrupt (0..255)
     *  @param handler          -   linear address of handler
     *  @param dpl              -   Privilege level allowed to access the gate (0..3) (only for sw ints)
     *  @param runIntEnabled    -   if true, INTs will be enabled upon entry
     *
     *  @return address of previous handler

     *
     *  @todo IDT currently always uses current CS
     */
   // static myos_TRAP_f setVector( index_t i, myos_TRAP_f handler, EDPL dpl, bool runIntEnabled );
   static myos_TRAP_f setVector( index_t i, const CallGate& entry );

   /// Removes the interrupthandler for a particular INT
   static inline void clearVector( index_t i )
	{
	   // PRINTF( "\nIDT::clearVector %X", i );

		/// @todo IDTable[ INT ].ClearAtomic();
		IDTable[ i ].clear();
	}

private:
   enum { MAX_IDT_ENTRIES = 256 };

   static CallGate* start() { return (CallGate*) &IDTable[0]; }

   static ATOMIC64<CallGate> IDTable[ MAX_IDT_ENTRIES ] TABLESECTION;
};

}}   // namespace

#endif //IDT_H
