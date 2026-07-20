#include "TimerComponent.h"
#include "C8254.h"
#include "IInterruptHandling.h"

#include "debug.h"

namespace MyOS { namespace Timer {

using InterruptHandling::E_IRQ0;

void
C8254::init( InterruptHandling::IInterruptContext& irqhandler ) // INITSECTION;
{
   PIT0MCR = (RW_Both | Mode0 | Binary );

   IInterruptHandling* ih = TimerComponent::getInstance().iInterruptHandling;
   
   // Interrupts are still disabled when this code runs...
   // 17/1/05: changed, disabled IRQs
   ih->setIRQHandler( E_IRQ0, irqhandler, InterruptHandling::E_NONE );		
   ih->enableIRQ( E_IRQ0 );   
}

void
C8254::deinit()
{
   IInterruptHandling* ih = TimerComponent::getInstance().iInterruptHandling;
   
   ih->disableIRQ( E_IRQ0 );
   ih->removeIRQHandler( E_IRQ0 );
}

// virtual 
void 
C8254::fireAfterCPUCycles( u64 cpuTicks )
{
   //DPRINTK( "\nfireAfterCpuTicks=%u:%u min=%d", 
   //        (u32)(cpuTicks>>32), (u32) cpuTicks, minPeriodInIPS );
   PIT0CNT0.out2( ipsToTicksWithMax(cpuTicks) );
}

// virtual 
void 
C8254::doIdlePeriod()
{
   PIT0CNT0.out2( 0 );  
}

/* virtual
void
C8254::trigger() throw()
{
   // NOT_IN_IRQ;   // Too strict, better not in IRQ0 though
      
   // DPRINTK( "\nC8254::trigger" );
   TimerComponent::getInstance().iInterruptHandling->triggerIRQ( E_IRQ0 );
} 
*/  

u32   // IPS value after a period of 65536 / 1193180 second
C8254::afterProbe( u32 maxPeriodInTicks )
{
   // == 1/1193180 second, +1 to prevent division by zero
   u32 minPeriodInTicks = (maxPeriodInTicks >> 16) + 1;  
    
// conversion factor:   f(ticks) => timer_ticks = (ticks / ips) * TIMER_TICKS_PER_SECOND
//       == (ticks / ((max_period * TIMER_TICKS_PER_SECOND) >> 16)) * TIMER_TICKS_PER_SECOND
//       == ticks * (( TIMER_TICKS_PER_SECOND << 16) / (max_period * TIMER_TICKS_PER_SECOND))
//       == (ticks << 16) / max_period
//       == ticks / min_period
//       == FASTIDIV(ticks,min_period) => HIGH(ticks * (0xFFFFFFFF/min_period)+1))
   ticks2c8254 = RECIPROCE_U( minPeriodInTicks );

   // the factor might require some tweaking, since it may be too high due to overhead included in max_period_in_ticks...
   while ( ipsToTicks( maxPeriodInTicks ) > 0xFFFF )
   {
      --ticks2c8254;
  }

   // Next, increase minPeriodInTicks as much as needed
   while ( ipsToTicks( minPeriodInTicks ) < SAFETY_LIMIT_LOW )
   {
      ++minPeriodInTicks;
   }

#if 0
   Screen::POSITION p(1,0);   
   p <<  
    "\nminPeriodInTicks: " << minPeriodInTicks <<
    "\nmaxPeriodInTicks: " << maxPeriodInTicks <<
    "\nticks2c8254: " << ticks2c8254 <<
    "\nconvertTicks(maxPeriod) (should be 65535 or less) : " << convertTicks( maxPeriodInTicks ) <<
    "\nconvertTicks(minPeriod) (should be 1 or higher) : " << convertTicks( minPeriodInTicks );

    // while(1);         
#endif

   ASSERTIONv( minPeriodInTicks < maxPeriodInTicks, E_CRITICAL, minPeriodInTicks );


   // This is *required*, GCC generated the wrong code otherwise !! (for some reason, stack assignment of variables helps) 
   this->maxPeriodInIPS = maxPeriodInTicks;
   this->minPeriodInIPS = minPeriodInTicks;  

   // calculate these once, for efficiency
   this->maxPeriodforTimer = ipsToTicks(maxPeriodInTicks);
   this->minPeriodforTimer = ipsToTicks(minPeriodInTicks);

   // return IPS (correction: if for some reason, timers are set to fire too late)
   return (u32) (((u64) maxPeriodInTicks * TIMER_TICKS_PER_SECOND) >> 16);
}

   
}} // namespace
