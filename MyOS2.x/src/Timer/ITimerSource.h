#ifndef ITIMERSOURCE_H
#define ITIMERSOURCE_H

#include "types.h"

namespace MyOS { namespace Timer {

/// Interface implemented by C8254 and localAPIC
/**
 * There are multiple choices for timer source in the PC:
 * 
 * <timer>           <resolution>
   ACPI power management (pm) timer       ~280 nano seconds
   TSC (Time Stamp Counter)               1/CPU clock
   PIT (Programmable Interrupt Timer)     ~838 nano seconds
   APIC                                   busclock
   some soundcards                           ?
   some networkcards                         ?
   
   I haven't addressed or seen the ACPI timer yet, and the TSC does not generate
   any INT (and is said to be subject to delays when power management is used)
   
   
 */   
class ITimerSource {

protected:
   u32 minPeriodInIPS;  // set by subclasses
  
public:
   inline u32 getMinPeriodInIPS() { return minPeriodInIPS; }

   /**
    * Set the timer to fire after 'ts' CPU cycles
    * (32 bit -> max time about 2^32/1*10^9 ~ 4000 seconds on a 1Ghz CPU...)
    */
   virtual void fireAfterCPUCycles( u32 cycles ) = 0;
   
   /**
    * Set the timer as long as possible (or reasonable)
    */
   virtual void doIdlePeriod() = 0;   
   
   /**
    * Trigger an IRQ to get into async context
    */
   virtual void trigger( ) = 0;
};

}} // namespace

#endif
