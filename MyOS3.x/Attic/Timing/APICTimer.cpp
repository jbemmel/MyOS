#include "APICTimer.h"
#include "Devices/IOPort.h"

#include "debug.h"


namespace MyOS { namespace Timing {

// see ftp://download.intel.com/design/pentium4/manuals/24547212.pdf p274
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

   // not of interest right now: TPR, APR, PPR
   const u32 reserved5[12];
   struct {
      u32 eoi;    // write-only
      const u32 reserved6[3];     
   };      
   const u32 reserved7[4];
   const u32 reserved8[8];   // not of interest: LDT, DFR
   struct {
      u32 sivr;   // initialized to 0xFF at startup, which means timer OFF!
      const u32 reservedx[3];
   };
   
   const u32 inService[ 32 ];
   const u32 triggerMode[ 32 ];
   const u32 intRequest[ 32 ];
   struct {
      /*const*/ u32 errorStatus; // read-only according to Intel, but linux wr
                                 // Due to the Pentium erratum 3AP
      const u32 reserved9[3];
   };
   const u32 reserved10[28];
   struct {
      u32 icr_lo;
      const u32 reserved11[3];
   };
   struct {
      u32 icr_hi;
      const u32 reserved12[3];
   };

   /**
    * And finally, the interesting stuff...
    */
   struct {
      u32 timerLVT;
      const u32 reserved13[3];     
   };
   struct {
      u32 thermalSensor;   // P4
      const u32 reserved14[3];     
   };
   struct {
      u32 perfMonitoring;
      const u32 reserved15[3];     
   };
   struct {
      u32 lint0;
      const u32 reserved16[3];     
   };
   struct {
      u32 lint1;
      const u32 reserved17[3];     
   };
   struct {
      u32 lvtError;
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
      const u32 dcr;    // one of the E_DCR_VALUES
      const u32 reserved25[3];
   };
   const u32 reserved26[4];   
};

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
      
// NOTE: Interrupts are still off when running this code...
bool
APICTimer::init( IVirtualMemory& pm, IInterruptHandling& ih ) // INITSECTION
{
   // claim INT handler
   // u32 vector = ih.setAPICHandler( *this );

   ih.setTrapHandler( E_APIC_TIMER_VECTOR, onTimer, E_ENABLE_INTS );

   // The local APIC may need to be enabled using some MSR!
   /* 
    * Some BIOSes disable the local APIC in the APIC_BASE MSR. 
    * This can only be done in software for PPro and above. 
    * 
    * JvB: And in particular, not on AMD K6-II (which has no local APIC)
    */
   if (!Processor::enableLocalAPIC()) return false;
   
   // Need to map memory before I can access it!
   apic = (volatile LOCALAPIC*) pm.mapMMIO( APIC_BASE, 0 );  // 1 page is enough
   DPRINTK( "\nLocal APIC mapped to vector %x mem is %x linear icr=%x", E_APIC_TIMER_VECTOR, apic, OFFSET_OF(LOCALAPIC,initialCount) );

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

   // Linux: "connect_BSP_APIC" magic ??
   IOPort8<0x22> m0 = 0x70;
   IOPort8<0x23> m1 = 0x01;

   // program LVT to interrupt on INTx, one-shot mode, clear mask
   apic->timerLVT = E_APIC_TIMER_VECTOR;
   
   // Enable it by setting the bit in the SIVR (which starts as 0)
   apic->sivr |= (1<<8);
   apic->initialCount = 1000;    // try it!
   return true;   
}

// static: dont use trampolines + locking, but rely on EOI!
MultiThreading::Thread* 
APICTimer::handleTimer()
{
   DPRINTK( "\nAPICTimer::handleTimer" );
   
   // Finally ACK it, this serves as the locking mechanism 
   apic->eoi = 0;   
   return 0;  
}


__asm__ (
".globl APIC_onTimer   \n"
".align 16             \n"
"APIC_onTimer:         \n"
"pushal                \n"
"call handleTimer      \n"
"test %eax, %eax       \n"
"jz done               \n"    // frequent ??
"lcall $0x8,$finishINT \n"    // restore CS (needed?) 
"done:                 \n"
"popal                 \n"
"iretl                 \n"
);
  
}} // namespace
