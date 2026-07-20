#include "IInterruptHandlingImpl.h"
#include "IDT.h"
#include "MM/SegmentDescriptor.h"
#include "MM/GDT.h"
#include "CPU/Processor.h"
#include "IOAPIC.h"

#include "MM/IVirtualMemoryImpl.h"
#include "MM/PTE.h"// todo restructure

#include "debug.h"

namespace MyOS { namespace InterruptHandling {

using namespace MM;

//const uuid_t IInterruptHandling::ID = "26468cf9-fa5b-4de7-9914-eec11e667215";

// static, one for each IRQ
IInterruptHandlingImpl::IRQDATA IInterruptHandlingImpl::IRQhandlers[17];

int IInterruptHandlingImpl::IRQNestingLevel[ MAX_CPU ] = { 0, };   // ASMNAME( "IRQNESTING" );
u8 IInterruptHandlingImpl::assignedCPU[ 17 ] = { 0, };             // ASMNAME( "CPU_FOR_IRQ" );

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
    *
    * simply clone current CS
    */
   INTSegment = GDT::cloneEntry( GDT::E_CODE_DPL0, MM::E_DPL0 );
   DPRINTK( "\nINTSegment selector=%x", INTSegment );	// == 0x28

   // Dummy class to handle unexpected IRQ
   static class CUnexpectedIRQ : public IInterruptContext
   {
      u32 count;
   public:
      /* virtual */ void onInterrupt( u32 message, cpu_id_t cpu ) throw() {
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

	// E_PAGE_FAULT is overridden in IDemandPageingImpl

	// Catch some common exceptions for debugging
	//
	// TODO: To catch stack overflows too, I should really use a task gate
	// here which switches to a separate stack! (on DoubleFault)
	setTrapHandler( E_GPF_EXCEPTION, trapGPF, E_NONE );
	setTrapHandler( E_STACK_EXCEPTION, trapStackfault, E_NONE );
	setTrapHandler( E_SEGMENT_NOT_PRESENT, trapSegmentfault, E_NONE );

	// Use MM handlers
	setTrapHandler( E_PAGE_FAULT, IVirtualMemoryImpl::getPageFaultHandler(), E_ENABLE_INTS | E_SYNCHRONOUS );
	setTrapHandler( E_IPI_FLUSHTLB, MM::PTE::flush_TLB_IPI, E_ENABLE_INTS | E_SYNCHRONOUS );

	this->initSMP_ACPI();

	initIOAPIC( context );

	return true;
}

// Added: search for BIOS MP structure, see http://www.osdever.net/tutorials/mp.php
void
IInterruptHandlingImpl::initSMP()
{
#define SMP_MAGIC_IDENT (('_'<<24)|('P'<<16)|('M'<<8)|'_')

    struct intel_mp_floating
    {
        u32 mpf_signature;              /* "_MP_"                       */
        unsigned long mpf_physptr;      /* Configuration table address  */
        unsigned char mpf_length;       /* Our length (paragraphs)      */
        unsigned char mpf_specification;/* Specification version        */
        unsigned char mpf_checksum;     /* Checksum (makes sum 0)       */
        unsigned char mpf_feature1;     /* Standard or configuration ?  */
        unsigned char mpf_feature2;     /* Bit7 set for IMCR|PIC        */
        unsigned char mpf_feature3;     /* Unused (0)                   */
        unsigned char mpf_feature4;     /* Unused (0)                   */
        unsigned char mpf_feature5;     /* Unused (0)                   */
     } PACKED;

     /**
      * Linux does this: (mpparse.c)
      * 1) Scan the bottom 1K for a signature
      * 2) Scan the top 1K of base RAM
      * 3) Scan the 64K of bios                <= Bochs found here
      */
     struct area {
         u32 start;
         u32 end;

         inline intel_mp_floating* st() const { return (intel_mp_floating*) start; }
         inline intel_mp_floating* e() const { return (intel_mp_floating*) end; }

     } toScan[3] = { { 0x000, 0x400 }, { 639*0x400, 640*0x400 }, { 0xF0000, 0x100000 } };

     // Skip scanning 0K for now
     for ( u32 i=/*0*/1; i<3; ++i ) {
         for ( intel_mp_floating *mp = toScan[i].st(); mp < toScan[i].e(); ++mp ) {
             if ( mp->mpf_signature == SMP_MAGIC_IDENT ) {
                 DPRINTK( "\nFound MP table @ %x phys=%x spec=%X", mp,
                         mp->mpf_physptr, mp->mpf_specification );

                 return;
             }
         }
     }
     DPRINTK( "\n#4f#No MP table found" );
}

// Added: search for BIOS ACPI RSDP structure, see http://www.osdev.org/osfaq2/index.php/RSDP
void
IInterruptHandlingImpl::initSMP_ACPI()
{
    struct RSDPDescriptor
    {
        char signature[8];
        u8 checkSum;
        char oemID[6];
        u8 revision;    /* Must be (0) for ACPI 1.0 or (2) for ACPI 2.0+ */
        u32 rsdtAddress;

        // Performs a checksum
        inline bool isValid() const {
            u8 *b = (u8*) this;
            u32 sum = 0;
            for ( int i=0; i<sizeof(RSDPDescriptor); ++i ) {
                sum += b[i];
            }
            return (sum&0xff)==0;  // lowest byte of sum should be 0
        }

    } PACKED;

    struct RSDPDescriptor20
    {
        RSDPDescriptor v1;

        u32 length;
        u64 xsdtAddress;
        u8 extCheckSum;
        u8 reserved[3];
    } PACKED;

     /**
      * It should be either in the EBDA or in 0xF0000-0x100000
      *
      * Scanning only latter here
      */
     struct area {
         u32 start;
         u32 end;

         inline u32* st() const { return (u32*) start; }
         inline u32* e() const { return (u32*) end; }

     } toScan[1] = { { 0xF0000, 0x100000 } };

     for ( u32 i=0; i<1; ++i ) {
         // scan per 16 bytes
         for ( u32 *rsdp = toScan[i].st(); rsdp < toScan[i].e(); rsdp+=4 ) {
             if ( strncmp( (const char*) rsdp, "RSD PTR ", 8 ) == 0 ) {
                 RSDPDescriptor *d = (RSDPDescriptor*) rsdp;
                 DPRINTK( "\nFound RSDP table @ %x phys=%x spec=%X checksum=%b", d,
                         d->rsdtAddress, d->revision, d->isValid() );

                 return;
             }
         }
     }
     DPRINTK( "\n#4f#No ACPI RSDP table found" );
}

void
IInterruptHandlingImpl::deinit( IContext& context )
{
    removeTrapHandler( E_PAGE_FAULT );
    removeTrapHandler( E_SEGMENT_NOT_PRESENT );
    removeTrapHandler( E_STACK_EXCEPTION );
    removeTrapHandler( E_GPF_EXCEPTION );
    removeIRQHandler( E_IRQ7 );
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

	// handler must be aligned!
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

// static
void
IInterruptHandlingImpl::trapSegmentfault()
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
   PRINTK( "#4f#\nSegment fault caught! EIP is %x", eip );
   while (1);
}

// static
void
IInterruptHandlingImpl::trapPagefault()
{
/**
  Stack:
ESP-> [ n-dword filler for GCC stack alignment (!) ]
      [ fault code ]
      [ EIP ]
      [ EFlags ]
      [ CS ]

**/


#ifdef DEBUG
   register u32 eip = ((u32*) Processor::ESP())[2];   // minus stackframe (12 bytes)
#else
   register u32 eip = ((u32*) Processor::ESP())[1];   // minus stackframe
#endif

   // not only during DEBUG ! No DPRINTK
   PRINTK( "#4f#\nPagefault caught! EIP is %x CR2=%x", eip, Processor::CR2() );
   while (1) ASMVOLATILE("hlt");
}

/* EAX == irq
ASMVOLATILE (
"triggerIRQ:         \n"
"popl %ecx           \n" // get caller's return address
"pushf               \n" // pretend INT was executed, for IRET
// "pushw $0            \n" // keep stack aligned(!)
"pushl %cs           \n" // keep stack aligned(!) not 'pushw'
"pushl %ecx          \n" // put caller's return address back
"pushl %eax          \n" // INT # parameter
// "cli			     \n" // Match HW IRQ, prevent interruption (needed?)
"ljmp $0x28, $.IRQentry \n" // jump into the async handler routine below (.entry), using CS=0x28 (IRQ)
);*/

// Note: Each trampoline is 5 bytes, aligned to 8 using filler
#define TRAMPOLINE(n,p) \
   ".align 8; pushl %eax ; movb $" #n ", %ah ; jmp pic" #p ";\n"

__asm__ (
".globl IRQ_TRAMPOLINES \n"
".align 128             \n"     // Pentium has 32 bytes of L1 cache, P4 has 64 bytes
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
".align 16			\n"
"pic1: 				\n"
"movb $0x58, %al	\n"	// Send specific EOI to slave, 0x60 + (irq-8)
"addb %ah, %al		\n"
"outb %al, $0xA0  	\n"
"pic0: 				\n" // Using Auto-EOI for master, so nothing here

"movzx %ah, %eax 	\n"             // get irq number in EAX, clear rest
".globl .IRQentry    \n"
".IRQentry:                        \n"
//"incl IRQDATA( , %eax, 1<<3 )  	\n" // use %SS, as DS may not be valid?
//"jnz .done                      \n" // exit when handler already running
"pushal                         \n" // save registers
"pushw %ds                      \n" // Push DS,ES such that privilege changes work
"pushw %es                      \n"

// First, setup DS and ES since they may not be valid (e.g. if in VM86 mode)
// NOTE: Be careful with special stack segments tricks for this reason!
// "pushw %ss \n %popw %ds \n pushw %ss \n popw %es"  // alt: use selector!

"lea IRQDATA( , %eax, 1<<3), %ebx \n"    // use EBX, which should not be clobbered
"movzx CPU_FOR_IRQ( , %eax, 1), %esi \n" // get assigned CPU in ESI (no-clobber)

// TODO: Start accounting here, using RDTSC, then push EAX/EDX

"incl IRQNESTING( , %esi, 1<<2 )\n" // Increase IRQ nesting level for given CPU
// ".align 16             \n"
".again:               \n"
"movl  4(%ebx), %eax   \n"   // Load context in EAX (GCC calling convention)
"movl  (%eax), %ecx    \n"   // Load method pointer at [0] (!! ?? always OK?? !!)
                             // TODO load 'message' in ECX, 'cpu' in EDX
"call  *(%ecx)         \n"   // Call handler, EBX,ESI,EDI and EBP are callee-save per C++ ABI
//"decl (%ebx)           \n"   // Decrease counter
//"jns  .again           \n"   // if not <0, repeat loop

"decl IRQNESTING( , %esi, 1<<2 )\n" // Decrease IRQ nesting level for given CPU
"jnz .done2            \n"   // Nested execution, so skip accounting

// TODO: Stop accounting here, safe delta RDTSC for appropriate CPU

"sti                                \n" // Enable interrupts (e.g. disabled during network IRQs), pending IRQ may fire now
"decl REQ_RESCHEDULE( , %esi, 1<<2 )\n" // Check reschedule request flag (set to 1)
"jz .reschedule        \n"              // If 0, call reschedule(). This handles RACE condition of interrupt during accounting

".globl _new_thread_start \n"
"_new_thread_start:       \n"

".done2:               \n"
"popw %es              \n"
"popw %ds              \n"
"popal                 \n"   // restore registers
".done:                \n"
"pop  %eax             \n"   // restore original value in EAX
"iretl                 \n"   // and finish INT

".reschedule:          \n"
"pushl $.done2         \n"   // Fake return address, first code executed by new threads
"ljmp $0x8, $RESCHEDULE\n"   // Call scheduler, restore CS (also non-interrupt call sites)

);

}}  // namespaces
