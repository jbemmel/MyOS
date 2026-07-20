#ifndef APICTIMER_H
#define APICTIMER_H

#include "TimerFacility/ITimerSource.h"
#include "memtypes.h"

namespace MyOS {

namespace MultiThreading { class Thread; }
namespace InterruptHandling { class IInterruptContext; }

namespace Timer {

// Controls the local APIC timer, if detected on the CPU
class APICTimer : public ITimerSource {
public:
   inline APICTimer() : cpu2busDivisor_timesN(0) { }   // Just in case

   // @return false if CPU does not support it
   bool init( InterruptHandling::IInterruptContext& irqhandler ) INITSECTION;
   void startCountDown() INITSECTION;
   bool probeDone( u32 CPUticksInPeriod ) INITSECTION;

   // void deinit();

   // TimerSource
   virtual void fireAfterCPUCycles( u64 cpuTicks );
   virtual void doIdlePeriod();
   // virtual void trigger() throw();

private:
   // TRAP handler, written in asm
   static void onTimer() ASMNAME( "APIC_onTimer" );

   u32 cpu2busDivisor_timesN;
};

}} // namespace

#endif
