#include "APICTimer.h"
#include "Processor/Processor.h"
#include "Timer/TimerComponent.h"
#include "TimeUtil.h"   // stores IPS

namespace MyOS { namespace Timer {

// see ftp://download.intel.com/design/pentium4/manuals/24547212.pdf p274
enum E_DCR_VALUES {
   E_DIV_1   = 0xB,  // 1011 binary, fastest
   E_DIV_2   = 0x0,  // 0000 binary (initial value at startup)
   E_DIV_4   = 0x1,  // 0001 binary
   E_DIV_8   = 0x2,  // 0010 binary
   E_DIV_16  = 0x3,  // 0011 binary
   E_DIV_32  = 0x8,  // 1000 binary
   E_DIV_64  = 0x9,  // 1001 binary
   E_DIV_128 = 0xA,  // 1010 binary, slowest
};

struct APICTimer::LOCALAPIC {
   const u32 reserved1[8];
   struct {
     u32 localId;
     const u32 reserved2[3];
   };     
   struct {     
     const u32 version;       // read-only
     const u32 reserved3[3];
   };
   const u32 reserved4[16];

   struct {
      u32 tpr;
      const u32 reserved45[3];     
   };      

   // not of interest right now: APR, PPR
   const u32 reserved5[8];
   
   struct {
      u32 eoi;    // write-only, offset 0x2C * 4
      const u32 reserved6[3];     
   };      
   const u32 reserved7[4];

   struct {
      u32 ldr;
      const u32 reserved8[3];
   };

   struct {
      u32 dfr;
      const u32 reserved85[3];
   };

   struct {
      u32 sivr;   // initialized to 0xFF at startup, which means timer OFF!
      const u32 reservedx[3];
   };
   
   const u32 inService[ 32 ];    // offset 0x100
   const u32 triggerMode[ 32 ];
   const u32 intRequest[ 32 ];
   struct {
      /*const*/ u32 errorStatus; // read-only according to Intel, but linux wr
                                 // Due to the Pentium erratum 3AP
      const u32 reserved9[3];
   };
   const u32 reserved10[28];
   struct {
      u32 icr_lo;				// 0xc0
      const u32 reserved11[3];
   };
   struct {
      u32 icr_hi;				// 0xc4
      const u32 reserved12[3];
   };

   /**
    * And finally, the interesting stuff...
    */
   struct {
      u32 timerLVT;			// 0xc8
      const u32 reserved13[3];     
   };
   struct {
      u32 thermalSensor;   	// 0xcc
      const u32 reserved14[3];     
   };
   struct {
      u32 perfMonitoring;	// 0xd0
      const u32 reserved15[3];     
   };
   struct {
      u32 lint0;			// 0xd4
      const u32 reserved16[3];     
   };
   struct {
      u32 lint1;			// 0xd8
      const u32 reserved17[3];     
   };
   struct {
      u32 lvtError;			// 0xdc
      const u32 reserved18[3];     
   };
   struct {
      u32 initialCount;
      const u32 reserved19[3];     
   };
   struct {
      const u32 currentCount;
      const u32 reserved20[3];     
   };
   const u32 reserved24[16];
   struct {
      u32 dcr;    // one of the E_DCR_VALUES
      const u32 reserved25[3];
   };
   const u32 reserved26[4];   
};

// static
volatile APICTimer::LOCALAPIC *APICTimer::apic;

// see linux: getMaxlvt
u8
APICTimer::getMaxLVT() const
{
   u32 v = apic->version;
   if (v & 0xF0) {            // if APIC is integrated, read it
      return (v>>16) & 0xFF;
   } else {
      return 2;              // 82489DXs do not report # of LVT entries
   }
}

void
APICTimer::clear()
{
   u8 maxlvt = getMaxLVT();
   if (maxlvt >= 3) apic->lvtError = 0xfe | (1<<16);  // linux values...
      
   // see linux' apic.c : APIC reset, set masks only first, then clear rest
   apic->timerLVT |= (1<<16);
   apic->lint0    |= (1<<16);
   apic->lint1    |= (1<<16);
   if (maxlvt >= 4) apic->perfMonitoring |= (1<<16);

   apic->timerLVT = (1<<16);
   apic->lint0    = (1<<16);
   apic->lint1    = (1<<16);
   if (maxlvt >= 3) {
      apic->lvtError = (1<<16);
      if (maxlvt >= 4) apic->perfMonitoring = (1<<16);
   }
   if (apic->version & 0xF0) {   // !82489DX
      if (maxlvt > 3) apic->errorStatus = 0;
      volatile u32 dummy UNUSED;
      dummy = apic->errorStatus;
   }   
}

// @return false if CPU does not support it
bool 
APICTimer::init( InterruptHandling::IInterruptContext& timerImpl ) // INITSECTION;
{
   return false;  // test to force usage of C8254, under Bochs 2.2 APIC seems not to work
   
   // The local APIC may need to be enabled using some MSR!
   /* 
    * Some BIOSes disable the local APIC in the APIC_BASE MSR. 
    * This can only be done in software for PPro and above. 
    * 
    * JvB: And in particular, not on AMD K6-II (which has no local APIC)
    */
	if (!Processor::enableLocalAPIC()) return false;

	IInterruptHandling* ih = TimerComponent::getInstance().iInterruptHandling;
	ih->setTrapHandler( E_APIC_TIMER_VECTOR, onTimer, E_NONE );

	// XX This violates separation of concern principles
	ih->setAPICHandler( timerImpl );

	IVirtualMemory* vm = TimerComponent::getInstance().iVirtualMemory;
   
	// Need to map memory before I can access it!
	apic = (volatile LOCALAPIC*) vm->mapMMIO( APIC_BASE, 0 );  // 1 page is enough
	DPRINTK( "\nLocal APIC mapped to vector %x mem is %x linear icr=0x%x", 
		E_APIC_TIMER_VECTOR, apic, OFFSET_OF(LOCALAPIC,initialCount) );

	/****
 	 * This comes roughly from Linux 'init_bsp_APIC' which is called just before
 	 * ISA IRQs are setup
 	 */
	u8 version = apic->version & 0xFF;
	clear();

	// 0xFF == spurious APIC vector, let's hope it does not get called...
	// Enables timer by setting the bit 8 in the SIVR (which starts as 0)
	apic->sivr = (apic->sivr & ~0xFF) | (1<<8) | (1<<9) | 0xFF;

	// setup virtual wire mode (82489DX needs (1<<15) for level triggered)
	apic->lint0 = 0x700;
	apic->lint1 = (version&0xF0) ? 0x400 : (0x400 | (1<<15) );

/**
 * 'init_bsp_APIC' ends here (Linux sets up ISA IRQs next...)
 * 
 * Then (later) follows 'smp_init' -> 'APIC_init_uniprocessor'
 */
 
#define	APIC_IMCR_P1	0x22	/* int mode conf register port 1 */
#define	APIC_IMCR_P2	0x23	/* int mode conf register port 2 */
#define	APIC_IMCR_SELECT 0x70	/* select imcr by writing into P1 */
#define	APIC_IMCR_PIC	0x0		/* selects PIC mode (8259-> BSP) */
#define	APIC_IMCR_APIC	0x1		/* selects APIC mode (8259->APIC) */
 
   // Linux: "connect_BSP_APIC" magic ??
   IOPort8<APIC_IMCR_P1> m0 = APIC_IMCR_SELECT;
   IOPort8<APIC_IMCR_P2> m1 = APIC_IMCR_APIC;

   // Assuming APIC ID is set correctly...
   
   // Intel recommends this way
   apic->dfr = 0xFFFFFFFFUL;
   apic->ldr = (apic->ldr & ~(0xFF<<24)) | (1<<(0+24));
   apic->tpr &= ~0xFF;     // accept-all

/**
 * NOTE: This duplicates some code above, perhaps not needed?
 */

   // now enable it (bit 9 (Focus processor) is set to 0)
   apic->sivr = (apic->sivr & ~0x2FF) | (1<<8) | 0xFF;
   // apic->lint0 assumed ok
   // apic->lint1 assumed ok
   
   // leave ESR disabled for now...
   
	// program LVT to interrupt on INTx, one-shot mode, clear mask
	apic->timerLVT = E_APIC_TIMER_VECTOR;
   
	// for bochs, in case 'reset' was pressed (else INT won't fire!)
	// This is actually a bochs BUG
	apic->eoi = 0;
   
	// Division reg is initialized to /2, linux uses /16, let's try /1 ??
	apic->dcr = E_DIV_1;   

	// set ITimerSource::minPeriodInIPS to some realistic value? 1 bus clocktick?
	this->minPeriodInIPS = 5000;  // ??   == CPU cycles too...
   
   // apic->initialCount = 1000;    // try it!
   return true;   
}

/**
 * This is called "slightly" before the C8254 timer is setup to do one
 * full cycle for measurement
 */
void 
APICTimer::startCountDown() // INITSECTION;
{
   apic->initialCount = 0xFFFFFFFF;    // set it at max   
}

/**
 * This is called "slightly" after the IRQ fired...
 * Correction factor ??
 * 
 * @return False if something wrong, e.g. 0 ticks counter
 *         This currently happens eg on Bochs without APIC support 
 *        (not on real CPU)
 */
bool
APICTimer::probeDone( u32 CPUticksInPeriod )     // INITSECTION
{
   u32 busticks = (u32) 0xFFFFFFFF - (apic->currentCount);
   DPRINTK( "\nI counted %d busticks (CPU=%d), so bus:cpu = 1 : %d", 
      busticks, CPUticksInPeriod, busticks / CPUticksInPeriod );
   return busticks!=0;      
}

void
APICTimer::deinit()
{
   if (apic) {
      apic->sivr = 0;   // disable APIC, works ?
      IVirtualMemory* vm = TimerComponent::getInstance().iVirtualMemory;
      vm->unmapMMIO( (MM::linadr_t) apic, 0 );
      apic = 0;     
   }   
}

static volatile int intcount ASMNAME("intcount") = 0;

/***
 * **NOT** in INITSECTION!
 * 
 * TODO: FIXME, APIC code currently untested!

 ***/
ASM (
".globl APIC_onTimer   			\n"
".align 16             			\n"
"APIC_onTimer:         			\n"
"pushl %eax            			\n"
"incl %ss:intcount				\n"	// Keep count
"movl %ss:apic, %eax			\n"
"movl $0, 4*44(%eax)			\n"	// send EOI to local APIC
"movb $16, %ah		   			\n" // load "IRQ" number
"jmpl *INT			   			\n" // Call regular INT processing code

);

// virtual 
void 
APICTimer::fireAfterCPUCycles( u32 cpuTicks )
{
	if (!cpuTicks) cpuTicks = 1;
	apic->initialCount = cpuTicks;   // == 1:1, make sure !=0
}

// virtual 
void 
APICTimer::doIdlePeriod()
{
   // DPRINTK( "\nAPIC::doIdlePeriod ips=%d", TimeUtil::getTicksPerSecond() );
   apic->initialCount = TimeUtil::getTicksPerSecond();   // 1 second
   // Or some other reasonable value ?
}

// virtual 
void 
APICTimer::trigger()
{
   ASSERTION( Processor::interruptsEnabled(), E_CRITICAL );
   NOT_IN_IRQ;
/*   
   // Could also simply set a flag in the IRQ routine, instead of relying
   // on the HW to change state ? tried it, no good
   
   // DPRINTK( "\nAPIC::trigger" );
   // triggered = false;
   // apic->initialCount = 1;    // cannot set it to 0, 1 may miss
   // while ( (apic->timerLVT & (1<<12)) == 0 );    // simply wait for INT!
   // while (apic->currentCount > 0 )
   // while ( apic->initialCount == 1)
   // while (!triggered); //  ASMVOLATILE( "hlt" ); ??
   Processor::interrupt<E_APIC_TIMER_VECTOR>(); // -> bad, writes EOI
   // Could check for that though -> works :)
   
   // I am not sure if this really protects us from parallel modification
   // of the supposedly protected timer structures: local APIC is probably
   // not blocked, so INT can still happen! Perhaps set timer really high
   // before doing SW int, or disable it entirely!
*/
   int count = intcount;
   apic->initialCount = 1;
   int waitlimit = 100000;
   while (count==intcount) {      // best waiting option?
      //ASMVOLATILE("hlt"); 
      if (--waitlimit == 0) {
         BUG( "\nAPIC failed to fire IRQ after 100000 loops" );
         return; 
      }
   }
}
   
}} // namespace
