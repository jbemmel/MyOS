#include "IInterruptHandlingImpl.h"
#include "IDT.h"
#include "MM/Virtual/SegmentDescriptor.h"
#include "MM/Virtual/GDT.h"
#include "Processor/Processor.h"

#include "debug.h"

namespace MyOS { namespace InterruptHandling {

using namespace MM;

// static, one for each IRQ
IInterruptHandlingImpl::IRQDATA IInterruptHandlingImpl::IRQhandlers[17];

// Flags used in IRQ handling routine
int IInterruptHandlingImpl::nestCount	= 0;

int IInterruptHandlingImpl::softIRQ    	= 0;

IInterruptHandlingImpl::IInterruptHandlingImpl( MyOS::Core::IComponent& c )
: IInterruptHandling( c, VERSION(1,0) )
{

}

bool 
IInterruptHandlingImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	IDT::initialize();

	for (u32 c=0; c<sizeof(IRQhandlers)/sizeof(IRQDATA); ++c) {
		IRQhandlers[c].count = -1;   // mark free
	}

   /**
    * Create a special segment descriptor in the GDT for interrupt routines
    *
    * In this way, code can determine whether it is executing in interrupt context 
    * (CS==INTSegment) or normal context (CS==flat_code)
    */
   SegmentDescriptor intseg( E_EXECUTABLE | E_EXECUTEREAD, 0, 0xFFBFF, E_DPL0 );
   INTSegment = GDT::addEntry( intseg, MM::E_DPL0 );
   // DPRINTK( "\nINTSegment selector=%x", INTSegment );

   // Dummy class to handle unexpected IRQ
   static class CUnexpectedIRQ : public IInterruptContext
   {
      u32 count;
   public:
      /* virtual */ void onInterrupt() {
       // REMARK( "Unexpected IRQ" );
        // NORET;
        // CHECKPOINT( '7' );
         ++count;
      }
   } unhandledIRQHandler;

   // IRQ7 is called for spurious interrupts, in-service register = 0
   setIRQHandler(E_IRQ7, unhandledIRQHandler, E_NONE );

   /// theC8259.DisableIRQ( C8259::IRQ7 ); // does not matter !
 /*
      In http://www.intel.com/design/intarch/applnots/27282301.pdf it is mentioned
    that IRQ7 is also used for "spurious interrupt detection". The 8259A chip sets
    it when an interrupt input level has disappeared before the INTA cyclus has
    completed (i.e. the signal is too short). Same goes for IRQ15 (i.e. too short
    input on the second PIC). Question: Should an EOI be sent for spurious 
    interrupts ? Since the IS bit is not set

    My PCNet32 driver generates IRQ7 interrupts at the same time as the chip's timer interrupt,
    but only if I use MMIO. Go figure. (Correction: at work also with PIO)

      My guess is it is a PCI timing issue (linux says "limiting PCI-PCI transfers" for my motherboard)

      Apparently, these interrupts can be detected by reading the in-service register of the C8259

     More info also here: http://support.intel.com/support/controllers/peripheral/7507.htm
    Perhaps certain timing restrictions are not met

     In any case, see e.g. PCNet32Ethernet.cpp : by using PIO in the right places (i.o. MMIO),
      the timing issue can be worked around.
   */

	// E_PAGE_FAULT is set in IDemandPageingImpl

	// Catch some common exceptions for debugging
	//
	// TODO: To catch stack overflows too, I should really use a task gate
	// here which switches to a separate stack! (on DoubleFault)
	setTrapHandler( E_GPF_EXCEPTION, trapGPF, E_NONE );    
	setTrapHandler( E_STACK_EXCEPTION, trapStackfault, E_NONE );
	return true;
}

void 
IInterruptHandlingImpl::deinit( IContext& context )
{

}


// virtual 
myos_result_t
IInterruptHandlingImpl::enableIRQ( E_IRQ irq )  {
   //DPRINTK( "\nenableIRQ %x", irq );
   theC8259.enableIRQ(irq);
   return E_MYOS_SUCCESS;
}

// virtual
myos_result_t
IInterruptHandlingImpl::disableIRQ( E_IRQ irq )  {
   //DPRINTK( "\ndisableIRQ %x", irq );    
   theC8259.disableIRQ(irq);   
   return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t
IInterruptHandlingImpl::setIRQHandler( E_IRQ irq,IInterruptContext& context,u32 flags ) {
   // DPRINTK( "\nsetIRQHandler for irq=%x @context=%x", irq, &context ); 

	// In the new structure, interrupts should be disabled
	ASSERTIONv( (flags & E_ENABLE_INTS) == 0, E_ERROR, irq );

	// handler must be aligned !
	ASSERTIONv( (((int)&context) & 3) == 0, E_ERROR, &context );

   /// @todo: dpl value correct ? what happens if code is running at DPL3, and INT occurs?
   CallGate gate( INTSegment, (u32) &trampolines[irq - E_IRQ0], (EDPL) (flags&E_DPL3), 0, flags&E_ENABLE_INTS );
   IDT::setVector( irq, gate );

   // IInterruptContext* oldHandler = IRQhandlers[ (irq - E_IRQ0) ].handler;
   IRQhandlers[ (irq - E_IRQ0) ].handler = &context;
   theC8259.enableIRQ(irq);
   // return oldHandler; ??
   return E_MYOS_SUCCESS;
}

// virtual  
myos_result_t 
IInterruptHandlingImpl::setAPICHandler( IInterruptContext& context )
{
	IRQhandlers[ 16 ].handler = &context;
	return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t
IInterruptHandlingImpl::removeIRQHandler( E_IRQ irq )  {
   theC8259.disableIRQ(irq);
   IRQhandlers[ (irq - E_IRQ0) ].handler = 0;
   IDT::clearVector( irq );
   return E_MYOS_SUCCESS;
}

// virtual
myos_result_t
IInterruptHandlingImpl::setTrapHandler( E_INTCODES trap, myos_TRAP_f traphandler, u32 flags )
{
   // PRINTF( "\nINTManager::setTrapHandler int=%X h=%x flags=%X", INT, (u32) traphandler, flags );

   // Use normal CS for traps? NO, APIC timer needs async
   CallGate gate( 
      flags & E_SYNCHRONOUS ? Processor::CS() : INTSegment, 
      (u32) traphandler, (EDPL) (flags&E_DPL3), 0, flags&E_ENABLE_INTS );
   IDT::setVector( trap, gate );  // returns old handler
   return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t
IInterruptHandlingImpl::removeTrapHandler( E_INTCODES irq )  {
   IDT::clearVector( irq );
   return E_MYOS_SUCCESS;
}

// static
void
IInterruptHandlingImpl::trapGPF()
{
#ifdef DEBUG 
   register u32 eip = ((u32*) Processor::ESP())[4];   // minus stackframe
#else
   register u32 eip = ((u32*) Processor::ESP())[1];   // minus stackframe
#endif   
   
   // not only during DEBUG ! No DPRINTK
   PRINTK( "#4f#\nGPF caught! EIP is %x", eip );
   while (1);
}

// static
void
IInterruptHandlingImpl::trapStackfault()
{
   // hmm, perhaps make some room on the stack here, or switch to an
   // emergency one... 
   
   // Actually, this routine is currently not called, since non-present
   // pages do not cause #SS but #GPF 
 
#ifdef DEBUG 
   register u32 eip = ((u32*) Processor::ESP())[4];   // minus stackframe
#else
   register u32 eip = ((u32*) Processor::ESP())[1];   // minus stackframe
#endif   
   
   // not only during DEBUG ! No DPRINTK
   PRINTK( "#4f#\nStackfault caught! EIP is %x", eip );
   while (1);
}

// virtual 
bool 
IInterruptHandlingImpl::triggerIRQ( E_IRQ irq ) 
{
   // It is generally a bad idea to do this in an interrupt context
   // but not totally illegal, as long as it is not called recursively
   // within the *same* IRQ context...
   ASSERTION( !inIRQ(), E_WARNING );

   // XX it is possible that another process is activated while 
   // the irq is locked...
   if (!inIrqInternal(irq)) {
      // CHECKPOINT( 'I' );
      ASMVOLATILE(
         // set ID flag to signal this IRQ is software triggered ?
         // "pushf   \n    orl $0x10000, (%%esp)  \n popf \n"
         "movl $0xFEEDFEED, %%esi   \n"	// magic value to detect SW INT
         "call triggerIRQ"				// ASM routine, see below
         : "=a"(irq)
         : "0"(irq - E_IRQ0) : "edi", "esi" );    // edi, esi destroyed
      return true;
   }
	DPRINTK( "\nIRQ: %X count=%x nested=%d", irq, 
		IRQhandlers[ irq - E_IRQ0 ].count, nestCount );
	BUG( "triggerIRQ called from same ASYNC context!" );
	return false;    
}

// EAX == irq
__asm__ (
"triggerIRQ:         \n"
"popl %ecx           \n" // get caller's return address
"pushf               \n" // pretend INT was executed, for IRET
"pushw $0            \n" // keep stack aligned(!)
"pushw %cs           \n"   
"pushl %ecx          \n" // put caller's return address back
"pushl %eax          \n" // INT # parameter
"cli			     \n" // Match HW IRQ, prevent interruption (needed?)
"ljmp $24, $.entry   \n" // jump into the async handler routine below (.entry)
);

// Note: Each trampoline is 5 bytes, aligned to 8
#define TRAMPOLINE(n,p) \
   "pushl %eax ; movb $" #n ", %ah ; jmp pic" #p "; nop; nop; nop;"

__asm__ (
".globl IRQ_TRAMPOLINES \n"
".align 16              \n"
"IRQ_TRAMPOLINES:       \n"
TRAMPOLINE(0,0)
TRAMPOLINE(1,0)
TRAMPOLINE(2,0)
TRAMPOLINE(3,0)
TRAMPOLINE(4,0)
TRAMPOLINE(5,0)
TRAMPOLINE(6,0)
TRAMPOLINE(7,0)	// <- This one can be spurious interrupt too
TRAMPOLINE(8,1)
TRAMPOLINE(9,1)
TRAMPOLINE(10,1)
TRAMPOLINE(11,1)
TRAMPOLINE(12,1)
TRAMPOLINE(13,1)
TRAMPOLINE(14,1)
TRAMPOLINE(15,1) // <- This one can be spurious interrupt too
"pic1: 				\n"
"movb $0x58, %al	\n"	// Send specific EOI to slave, 0x60 + (irq-8)
"addb %ah, %al		\n"
"outb %al, $0xA0  	\n"
"pic0: 				\n" // Using Auto-EOI for master, so nothing here

".globl INT \n"
"INT:"
"movzx %ah, %eax 	\n"                // get irq number in EAX, clear rest
".entry:                           	\n"
"incl %ss:IRQDATA( , %eax, 1<<3 )  	\n" // use SS, as DS may not be valid...
"jnz .done                         	\n" // exit when handler already running
"incl %ss:nestCount					\n" // Count nested interrupts
"sti								\n" // re-enable interrupts, allow high prio
"pushal                            	\n" // save registers

// First, setup DS and ES since they may not be valid (e.g. if in VM86 mode)
// NOTE: Be careful with special stack segments tricks for this reason!
// "pushw %ss \n %popw %ds \n pushw %ss \n popw %es"  // alt: use selector!

"lea IRQDATA( , %eax, 1<<3), %ebx \n" // use EBX, which should not be clobbered
".align 4              \n"
".again:               \n"
"movl  $0, (%ebx)      \n"   // clear counter
"movl  4(%ebx), %eax   \n"   // Load context in EAX (GCC calling convention)
"movl  (%eax), %ecx    \n"   // Load method pointer at [0] (!! ?? always OK?? !!)
"call  *(%ecx)         \n"   // Call handler
"decl (%ebx)           \n"   // Decrease counter
"jns  .again           \n"   // if not <0, repeat loop
"decl %ss:nestCount    \n"   // decrease nested counter
"jz .schedule		   \n"	 // If last handler, see if scheduler needs a call
"popal                 \n"   // restore registers
".done:                \n"
"pop  %eax             \n"   // restore original value in EAX
"iretl                 \n"   // and finish INT

// In case at least 1 thread is woken, and the last handler is exiting
".schedule:          		  \n"
"cmpl $0, %ss:softIRQ		  \n"	// should be atomic ?
"jz .noCall					  \n"
"movl $0, %ss:softIRQ		  \n"	// clear flag
"lcall $0x8,$IRQ_schedule 	  \n" 	// Call scheduler, restore CS (could use ljmp)
"popl %eax					  \n" 	// remove segment from stack
".noCall:					  \n"
"popal      				  \n"
"popl %eax  				  \n"
"iretl      				  \n"
);

}}  // namespaces
