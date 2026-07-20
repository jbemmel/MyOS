#include "LocalAPIC.h"
#include "IHComponent.h"
#include "CPU/Processor.h"
#include "IH/ihtypes.h"
#include "TimerFacility/TimeUtil.h"

#include "IVirtualMemory.h"
#include "MT/Thread.h"          // sizeof(Thread)
#include "MP/Process.h"         // @todo move this elsewhere

#include "debug.h"

namespace MyOS {
namespace InterruptHandling {

using MM::IVirtualMemory;
using namespace Timer;

// see ftp://download.intel.com/design/pentium4/manuals/25366821.pdf chapter 8
enum E_LVT_BITS
{

    /// Delivery modes, also for ICR register
    E_FIXED             = (0x0 << 8), //< 000b
    E_LOWEST_PRIORITY   = (0x1 << 8),
    E_SMI               = (0x2 << 8), //< 010b
    E_NMI               = (0x4 << 8), //< 100b
    E_INIT              = (0x5 << 8), //< 101b, vector must be set to 0
    E_STARTUP           = (0x6 << 8), //< 0b110
    E_EXTINT            = (0x7 << 8), //< 111b, compatibility mode

    E_EDGE_TRIGGERED = 0x0,

    E_INT_ASSERT        = (1<<14),
    E_LEVEL_TRIGGERED   = (1<<15),

    E_MASKED = (1 << 16),
};

enum E_ICR {
    E_DM_PHYSICAL   = 0,
    E_DM_LOGICAL    = (1<<11),
    E_ICR_SENDING   = (1<<12),
};

enum E_VARIOUS
{
    E_APIC_SPIV_FOCUS_DISABLED  = (1 << 9),
    E_APIC_SPIV_APIC_ENABLED    = (1 << 8),

};



// static
volatile LocalAPIC::LOCALAPIC *LocalAPIC::apic;

// see linux: getMaxlvt
u8 LocalAPIC::getMaxLVT()
{
    u32 v = apic->version;
    if (v & 0xF0)
    { // if APIC is integrated, read it
        return (v >> 16) & 0xFF; //< #LVT minus 1 (!)
    }
    else
    {
        return 2; // 82489DXs do not report # of LVT entries
    }
}

void LocalAPIC::clear()
{
    u8 maxlvt = getMaxLVT();

    /*
     * Masking an LVT entry on a P6 can trigger a local APIC error
     * if the vector is zero. Mask LVTERR first to prevent this.
     */
    if (maxlvt >= 3)
        apic->lvtError = InterruptHandling::E_APIC_ERROR | E_MASKED; // linux values...

    // see linux' apic.c : APIC reset, set masks only first, then clear rest
    apic->timerLVT = apic->timerLVT | E_MASKED;
    apic->lint0    = apic->lint0 | E_MASKED;
    apic->lint1    = apic->lint1 | E_MASKED;
    if (maxlvt >= 3)
    {
        apic->lvtError = apic->lvtError | E_MASKED;
        if (maxlvt >= 4) {
            apic->perfMonitoring = apic->perfMonitoring | E_MASKED;

            if (maxlvt >= 5) {
                apic->thermalSensor = apic->thermalSensor | E_MASKED;
            }
        }
    }

    apic->timerLVT = E_MASKED;
    apic->lint0 = E_MASKED;
    apic->lint1 = E_MASKED;
    if (maxlvt >= 3)
    {
        apic->lvtError = E_MASKED;
        if (maxlvt >= 4) {
            apic->perfMonitoring = E_MASKED;
            if (maxlvt >= 5) {
                apic->thermalSensor = E_MASKED;
            }
        }
    }
    if (apic->version & 0xF0)
    { // !82489DX
        if (maxlvt > 3)
        {
            apic->errorStatus = 0;  // Pentium errata
            // apic->errorStatus = 0; // back2back writes clear
        }
        volatile u32 dummy UNUSED;
        dummy = apic->errorStatus;
    }
}

// @return false if CPU does not support it
bool
LocalAPIC::initOnce( IInterruptHandling &ih ) // INITSECTION;
{
    /**
     * Note: on Linux it happens as follows:
     *
     * 1. local APIC is initialized even before the 8259A chip, by init_bsp_APIC()
     * 2. Then interrupt vectors are set
     * ...
     */

    // return false;  // test to force usage of C8254

    // The local APIC may need to be enabled using some MSR!
    /*
     * Some BIOSes disable the local APIC in the APIC_BASE MSR.
     * This can only be done in software for PPro and above.
     *
     * JvB: And in particular, not on AMD K6-II (which has no local APIC)
     */
    if (!Processor::enableLocalAPIC())
        return false;

    using InterruptHandling::E_APIC_ERROR;
    ih.setTrapHandler(E_APIC_ERROR, onError, InterruptHandling::E_ENABLE_INTS);
    using InterruptHandling::E_APIC_SPURIOUS;
    ih.setTrapHandler(E_APIC_SPURIOUS, onSpurious,
            InterruptHandling::E_ENABLE_INTS);

    IVirtualMemory* vm = IHComponent::getInstance().iVirtualMemory;

    // Need to map memory before I can access it! Do this once for all CPUs, each CPU
    // will reference its own local APIC
    apic = (volatile LOCALAPIC*) vm->mapMMIO(APIC_BASE, sizeof(LOCALAPIC)); // 1 page is enough
    DPRINTK("\nLocal APIC mapped to mem %x linear icr=0x%x",
            apic, OFFSET_OF(LOCALAPIC,initialCount) );

    init(); // init BSP APIC
}

bool
LocalAPIC::init() // INITSECTION;
{
    /***
     * This comes roughly from Linux 'init_bsp_APIC' which is called just before
     * ISA IRQs are setup
     */
    u32 ver = apic->version;
    DPRINTK("\nAPIC ID=%X version (hex): %X maxLVT: %d", getID(),
            ver & 0xff, (ver >> 16) & 0xff);
    clear();

    /*for (int i=0; i<32; ++i) {
        u32 v;
        if (v = apic->inService[i]) {
            DPRINTK( "\n#4f#Warning: IRQ still in service(%d): %x", i, v );
        }
    }*/

    // 0xFF == spurious APIC vector, let's hope it does not get called...
    // Enables timer by setting the bit 8 in the SIVR (which starts as 0)
    // @todo for P4 FOCUS bit is reserved, clear it
    apic->sivr = (apic->sivr & ~E_APIC_SPURIOUS) | E_APIC_SPIV_APIC_ENABLED
            | E_APIC_SPIV_FOCUS_DISABLED | E_APIC_SPURIOUS;

    // setup virtual wire mode (82489DX needs level triggered)
    apic->lint0 = E_EXTINT;
    apic->lint1 = (ver & 0xF0) ? E_NMI : (E_NMI | E_LEVEL_TRIGGERED);

    /**
     * 'init_bsp_APIC' ends here (Linux sets up ISA IRQs next...)
     *
     * Then (later) follows 'smp_init' -> 'APIC_init_uniprocessor'
     */

    // @todo Detect this feature in MP tables ('IMCRP bit')
#define	APIC_IMCR_P1	0x22	/* int mode conf register port 1 */
#define	APIC_IMCR_P2	0x23	/* int mode conf register port 2 */
#define	APIC_IMCR_SELECT 0x70	/* select imcr by writing into P1 */
#define	APIC_IMCR_PIC	0x0		/* selects PIC mode (8259-> BSP) */
#define	APIC_IMCR_APIC	0x1		/* selects APIC mode (8259->APIC) */

    // Linux: "connect_BSP_APIC" magic ??
    // PIC mode, enable APIC mode in the IMCR, i.e.
    // connect BSP's local APIC to INT and NMI lines.
    // @see http://developer.intel.com/design/pentium/datashts/24201606.pdf

    // Don't do this for now? Also means interrupts need to get ACKed in local APIC, code not there
    // But Bochs doesn't seem to emulate it very well
    if ( apic->localId == 0 ) {
        IOPort8<APIC_IMCR_P1> m0 = APIC_IMCR_SELECT;
        IOPort8<APIC_IMCR_P2> m1 = APIC_IMCR_APIC; // route IRQs via local APICs
    }

    // Assuming APIC ID is set correctly...

    // Intel recommends this way
    apic->dfr = 0xFFFFFFFFUL;
    apic->ldr = (apic->ldr & ~(0xFF << 24)) | (1 << (0 + 24));
    apic->tpr = apic->tpr & ~0xFF; // accept-all

    /**
     * NOTE: This duplicates some code above, perhaps not needed?
     */

    // now enable it (bit 9 (Focus processor) is set to 0)
    apic->sivr = (apic->sivr & ~0x2FF) | E_APIC_SPIV_APIC_ENABLED
            | E_APIC_SPURIOUS;

    // Only BSP (CPU#0) handles these for now
    if ( apic->localId == 0 ) {
        apic->lint0 = E_EXTINT;
        apic->lint1 = (ver & 0xF0) ? E_NMI : (E_NMI | E_LEVEL_TRIGGERED);
    } else {
        apic->lint0 = E_EXTINT | E_MASKED;
        apic->lint1 = ((ver & 0xF0) ? E_NMI : (E_NMI | E_LEVEL_TRIGGERED)) | E_MASKED;
    }

    apic->lvtError = E_APIC_ERROR;
    apic->getErrorStatus(); // must clear after setting vector

    // for bochs, in case 'reset' was pressed (else INT won't fire!)
    // This is actually a bochs BUG, @todo remove
    // apic->eoi = 0;

    // apic->initialCount = 1000;    // try it!
    return true;
}

void
LocalAPIC::deinit()
{
    if (apic)
    {
        apic->sivr = 0; // disable APIC, works ?

        IVirtualMemory* vm = IHComponent::getInstance().iVirtualMemory;
        vm->unmapMMIO((MM::linadr_t) apic, sizeof(LOCALAPIC));
        apic = 0;
    }
}


void LocalAPIC::onSpurious()
{
    // Linux apic.c says that spurious interrupts should not be ACKed??
    DPRINTK("\n#4f#Local APIC spurious interrupt!");
}

void LocalAPIC::onError()
{
    DPRINTK("\n#4f#Local APIC error interrupt! Status=%x",
            apic->getErrorStatus());
}

bool
LocalAPIC::waitICRDone()
{
    int i = 1000;
    do {
        if ( (apic->icr_lo & E_ICR_SENDING) == 0) return true;
        TimeUtil::delay<us>( 100 );
    } while ( --i >= 0 );

    DPRINTK( "\nwaitICRDone FAILED" );
    return false;
}

void
LocalAPIC::sendIPI( cpu_id_t cpumask, E_INTCODES vector )
{
    // disable reschedule, since we're writing to 2 separate registers
    u32 tpr = apic->setTaskPriority( E_IPI_RESCHEDULE );
    if (waitICRDone()) {
        apic->icr_hi = (cpumask<<24);
        apic->icr_lo = E_FIXED | E_DM_LOGICAL | vector;
    }
    apic->restoreTaskPriority( tpr );
}

static void showAPICErrors( u32 err ) {
    if ( err&0x01 ) DPRINTK( "\nAPIC: Send checksum error" );
    if ( err&0x02 ) DPRINTK( "\nAPIC: Receive checksum error" );
    if ( err&0x04 ) DPRINTK( "\nAPIC: Send accept error" );
    if ( err&0x08 ) DPRINTK( "\nAPIC: Receive accept error" );

    if ( err&0x20 ) DPRINTK( "\nAPIC: Send illegal vector" );
    if ( err&0x40 ) DPRINTK( "\nAPIC: Received illegal vector" );
    if ( err&0x80 ) DPRINTK( "\nAPIC: Illegal register address" );
}

extern "C" void initCPU() NORETURN INITSECTION;

void initCPU()
{
    Processor::enableNiceFeatures();
    LocalAPIC::init();

    MultiProcessing::setCPUIsOnline();  // this changes TLB flushing to become SMP-aware

    // create idle thread, then create TSS (per CPU!)
    IVirtualMemory* vm = IHComponent::getInstance().iVirtualMemory;
    MM::linadr_t stack = vm->allocateStack( 0, false );
    u32 *initThread = (u32*) (((u32)stack) + 0x1000 - sizeof(MultiThreading::Thread));

    register void *initPage = (void*) (((u32) Processor::ESP()) & ~0xfff);
    ASMVOLATILE( "movl %0, %%esp" : : "r"( initThread - 4 ) );    // need some room!
    vm->freeISADMA( initPage, 0 );

    void *mem = &MultiThreading::Thread::currentThread();

    // Clear thread structure, just in case
    memset_aligned( mem, 0, sizeof(MultiThreading::Thread) );

    MultiThreading::Thread* idle = new ( mem ) MultiThreading::Thread();
    MultiProcessing::Process::initOnce();

    // TODO need to synchronize TSC, for timers?
    ASMVOLATILE( "wrmsr" : /* no outputs */ : "c" (0x10), "a" (10000), "d" (0) );

    DPRINTK( "\n#3f#CPU1 Alive! Stack=%x", initThread );
    MultiThreading::Thread::becomeIdle( 10 );

    // while (1) ASMVOLATILE( "hlt" );
}

/**
 * ASM trampoline code defined at end of this file
 */
extern void* init_cpu_realmode_asm() ASMNAME("INIT_CPU16");

void
LocalAPIC::startOtherCPU( u32 physicalAPIC_ID )
{
    IVirtualMemory* vm = IHComponent::getInstance().iVirtualMemory;

    MM::physadr_t eip_start;
    MM::linadr_t init = vm->allocISADMA( 0, eip_start );  // must be page aligned below 1Mb physical

    // Must identity-map that page, else upon PM switch OtherCPU dies?
    vm->copyMapping( (MM::linadr_t) eip_start, init, 1,
            MM::E_PAGE_PRESENT | MM::E_PAGE_READONLY | MM::E_PAGE_NOCACHE );

    DPRINTK( "\nStarting CPU1 at %x(%x)", init, eip_start );
    memcpy_aligned( init, (void*) init_cpu_realmode_asm, 0x40 );
    ASMVOLATILE(
       "sgdtl 0x40(%0)       \n"
       "sidtl 0x48(%0)       \n"
       "movl %%cr3, %%eax    \n"
       "movl %%eax, 0x50(%0) \n"
       "mov %0, 0x54(%0)     \n"    // stack ptr
       "addl $0x99c, 0x54(%0)\n"    // end of page
       "mov %%ds, %%ax       \n"
       "mov %%ax, 0x58(%0)   \n"
       "wbinvd               \n"  // Ensure not cached
       : /* no outputs */ : "r"( init ) : "%eax"
    );

    // Clear any APIC errors (need back-2-back write to clear it?)
    apic->getErrorStatus();

    DPRINTK( "\nSending INIT to APIC:%u", physicalAPIC_ID );
    apic->icr_hi = physicalAPIC_ID<<24;
    apic->icr_lo = E_LEVEL_TRIGGERED | E_INT_ASSERT | E_DM_PHYSICAL | E_INIT;
    TimeUtil::delay<ms>( 10 );  // Intel says 10ms delay
    //int loops = 2;
    //do {
        if (!waitICRDone()) return;

        if (u32 err = apic->getErrorStatus()) {
            DPRINTK( "\n#4f#APIC error: %x", err );
            showAPICErrors( err );
            return;
        }

        // Assume icr_hi still set correctly?
        apic->icr_hi = physicalAPIC_ID<<24;
        apic->icr_lo = E_STARTUP | (eip_start>>12); // EIP MUST be page aligned!

        //TimeUtil::delay<us>( 200 );     // Give AP time to accept IPI
    //} while (--loops>0);

    // WAIT(!) until CPU is initialized, else upon freeing INIT pages we kill it!
    while ( (MultiProcessing::runningCPUs & (1<<physicalAPIC_ID)) == 0 )
    {
        TimeUtil::delay<us>( 200 );
    }
    DPRINTK( "\nCPU up:%u", physicalAPIC_ID );
    vm->clearMapping( (MM::linadr_t) eip_start, 0 );

    while (1) ASMVOLATILE( "hlt" );
}

// static
void
LocalAPIC::initTimer( E_DCR_VALUE divisor )
{
    // program LVT to interrupt on INTx, one-shot mode, clear mask
    apic->timerLVT = E_APIC_TIMER_VECTOR;

    // Division reg is initialized to /2, linux uses /16, let's try /1 ??
    apic->dcr = divisor;
}

// Copied to page below 1Mb
__asm__ (
".section .text.init16     \n"
".globl INIT_CPU16         \n"
".globl INIT_CPU32         \n"
".code16                   \n"
"INIT_CPU16:               \n"
"wbinvd                            \n"  // needed for NUMA-Q, but crashes Bochs(!) due to shared caches
"cli                               \n"
"mov %cs:seg - INIT_CPU16, %ax     \n"      // no point to load all here, need to reload in PM
"mov %ax, %ds                      \n"
"xor %edx, %edx                    \n"
"mov %cs, %dx                      \n"
"shl $4, %dx                       \n"
"addw $stack - INIT_CPU16, %dx     \n"  // Calculate real/protected-mode address for LSS
"movl %cs:cur_cr3 - INIT_CPU16, %eax \n"
"lidtl %cs:idt - INIT_CPU16        \n"  // load IDT
"lgdtl %cs:gdt - INIT_CPU16        \n"  // load GDT
"mov %eax, %cr3                \n"  // setup paging
"mov $0x80010001, %eax         \n"  // setup paging
"mov %eax, %CR0                \n"  // enable protected mode, now in 32 bit mode
"ljmpl $0x8, $INIT_CPU32       \n"  // jump to C++ world

".align 8           \n"
"gdt: .long 0x0,0x0 \n"
"idt: .long 0x0,0x0 \n"
"cur_cr3: .long 0x0 \n"
"stack: .long 0x0   \n"
"seg:   .word 0x0   \n"

".section .text.init32\n"
".code32              \n"
"INIT_CPU32:          \n"
"lss %cs:(%edx), %esp \n"  // Need to reload segment registers, (%edx) is address of "stack:"
"mov %ss, %ax         \n"
"mov %ax, %ds         \n"
"mov %ax, %es         \n"
"xor %eax, %eax       \n"
"mov %ax, %fs         \n"
"mov %ax, %gs         \n"
"sti                  \n"
"jmp _initCPU         \n"  // jump to C++ world
);

}} // namespace
