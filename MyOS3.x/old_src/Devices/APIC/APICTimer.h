#ifndef APICTIMER_H
#define APICTIMER_H

#include "TimerFacility/ITimerSource.h"

namespace MyOS { 

namespace MultiThreading { class Thread; }
namespace InterruptHandling { class IInterruptContext; }   

namespace Timer {

// Controls the local APIC timer, if detected on the CPU
class APICTimer : public ITimerSource {
public:
   inline APICTimer() { apic=0; }   // Just in case

   // @return false if CPU does not support it
   bool init( InterruptHandling::IInterruptContext& irqhandler ) INITSECTION;
   void startCountDown() INITSECTION;
   bool probeDone( u32 CPUticksInPeriod ) INITSECTION;

   void deinit();
   
   // TimerSource
   virtual void fireAfterCPUCycles( u32 cpuTicks );
   virtual void doIdlePeriod();
   virtual void trigger() throw();
   
private:
   // Some I/O registers, memory mapped      
   enum {
      // APIC_BASE           = 0xFEE00000,  // Memory mapped base, programmable
       APIC_BASE           = 0xFEC00000,    // Bochs uses this one, @todo read
   };

   // some utility functions, adopted from linux
   inline u8 getMaxLVT() const;
   void clear();

   // TRAP handler, written in asm
   static void onTimer() ASMNAME( "APIC_onTimer" );

   // TRAP handler, C++ part
   static void handleTimer() ASMNAME( "handleTimer" );

   // Static, addressed from static handler function
   struct LOCALAPIC;
   static volatile LOCALAPIC* apic ASMNAME( "apic" );   
};
   
}} // namespace

#endif
