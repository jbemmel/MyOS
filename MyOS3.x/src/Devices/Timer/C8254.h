#ifndef C8254CHIP_H
#define C8254CHIP_H

#include "TimerFacility/ITimerSource.h"
#include "Devices/IOPort.h"

namespace MyOS { 

namespace InterruptHandling { class IInterruptContext; } 
  
namespace Timer {

// Controls the C8254 timer chip
class C8254 : public ITimerSource {
public:
   void init( InterruptHandling::IInterruptContext& irqhandler ) INITSECTION;   
   u32 afterProbe( u32 maxPeriodInTicks ) INITSECTION;

   void deinit();

   // TimerSource   
   virtual void fireAfterCPUCycles( u32 cpuTicks );
   virtual void doIdlePeriod();
   virtual void trigger() throw();

private:
   // Some variables to help translate us -> timer ticks   

   
   // Counting format (bit 0 of control word)
   enum { Binary = 0, BCD = 1 };

   // Timer count modes (bits 3..1 of control word)
   enum { Mode0 = 0, Mode1 = (1<<1), Mode2 = (1<<2), Mode3 = (1<<3), Mode4 = (1<<4), Mode5 = (1<<5) };

   // Timer read/write modus (bits 5..4)
   enum { Latch = 0, RW_Low = 0x10, RW_High = 0x20, RW_Both = RW_Low+RW_High };

   // select CNT bits in control word (7..6)
   enum { SelCountIndex = 6 };
 
   // Mode control registers for both PITS
   volatile IOPort8 < 0x43 > PIT0MCR;
   volatile IOPort8 < 0x4b > PIT1MCR;

   // CNT registers
   // enum { PIT0_CNT0 = 0x40, PIT0_CNT1 = 0x41, PIT0_CNT2 = 0x42,
   // PIT1_CNT0 = 0x48, PIT1_CNT1 = 0x49, PIT1_CNT2 = 0x4a };
   volatile IOPort8 < 0x40 > PIT0CNT0;

/***
 * Conversion routines & values
 */
   inline u32 ipsToTicks( u32 ips ) { 
      return FASTIDIV( ips, ticks2c8254 ); 
   }

   // Converts IPS to timer ticks, but no larger than maximum timer capacity
   inline u32 ipsToTicksWithMax( u32 ips ) const { 
      // If ips is only slightly larger than maxPeriodInIPS, prevent doing maxPeriodforTimer?
      return ips < maxPeriodInIPS 
         ? FASTIDIV( ips, ticks2c8254 ) 
         // : ( ips < 2*maxPeriodInIPS ? maxPeriodforTimer/2 : maxPeriodforTimer);
         : maxPeriodforTimer;
   }

   
   // safety limit: if too small a value is programmed, recursion in timer 
   // routines will occur !
   enum { SAFETY_LIMIT_LOW = 3, TIMER_TICKS_PER_SECOND = 1193180 /* Hz */ };

   // minPeriodInIPS is in base class
   u32 maxPeriodInIPS, ticks2c8254;
   u32 minPeriodforTimer, maxPeriodforTimer;      
};
   
}} // namespace

#endif
