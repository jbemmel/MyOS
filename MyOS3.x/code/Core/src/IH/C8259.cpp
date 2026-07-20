#include "C8259.h"
#include "CPU/Processor.h"

#include "debug.h"

namespace MyOS {
namespace InterruptHandling {

C8259::C8259()  // INITSECTION
    : MasterMask(0xff), SlaveMask(0xff) // all IRQs disabled
{	
	// Write ICW1 2x
	MASTER_IO = ( BIT_ON | ICW4_FOLLOWS );
	SLAVE_IO  = ( BIT_ON | ICW4_FOLLOWS );
	
	// Set master vector start (ICW2)
	MASTER_DATA = ( E_IRQ0 );

	// Set slave vector start (ICW2)
	SLAVE_DATA = ( E_IRQ8 );

	// Indicate that IRQ2 is cascaded (ICW3)
	MASTER_DATA = ( (1<<2) );

	// Send slave identification (ICW3) as IRQ2
	SLAVE_DATA = ( 2 );

	// Finish initializtaion (ICW4)	
	MASTER_DATA = ( X86MODE | AutoEOI );	   // AutoEOI now supported by Bochs

	// From the Intel 8259A documentation, page 15:
	// "The AEOI mode can only be used in a MASTER 8259A and not a SLAVE. 
	// 8259As with a copyright date after 1985 will operate in AEOI mode as a
	// master or a slave"

	SLAVE_DATA  = ( X86MODE /* | AutoEOI */ ); // AEOI does not work for slave!

	// Mask (disable) all interrupts
	MASTER_DATA = ( MasterMask );
	SLAVE_DATA  = ( SlaveMask );

	// Enable IRQ2 (cascaded)
	enableIRQ( E_IRQ2_cascade );
}

}}   // namespace
