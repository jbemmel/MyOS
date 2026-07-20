#include "C8254.h"
// #include "Assertion.h"

#define ASSERTIONv(A,S,v)

namespace MyOS {
namespace Timing {

C8254ConversionFactors::ips_t
C8254ConversionFactors::initOnce( timerticks_t max_period_in_ticks )
{
	u32 maxPeriodInTicks = max_period_in_ticks;    // == 65536 / 1193180 second
	
    // == 1/1193180 second, +1 to prevent division by zero
   u32 minPeriodInTicks = (max_period_in_ticks >> 16)+1;	
		
	// conversion factor:	f(ticks) => timer_ticks = (ticks / ips) * TIMER_TICKS_PER_SECOND
	//			== (ticks / ((max_period * TIMER_TICKS_PER_SECOND) >> 16)) * TIMER_TICKS_PER_SECOND
	//			== ticks * (( TIMER_TICKS_PER_SECOND << 16) / (max_period * TIMER_TICKS_PER_SECOND))
	//			== (ticks << 16) / max_period
	//			== ticks / min_period
	//			== FASTIDIV(ticks,min_period) => HIGH(ticks * (0xFFFFFFFF/min_period)+1))
	ticks2c8254 = RECIPROCE( minPeriodInTicks );

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

	ASSERTIONv( minPeriodInTicks < maxPeriodInTicks, Debug::AS_FATAL, minPeriodInTicks );


	// This is *required*, GCC generated the wrong code otherwise !! (for some reason, stack assignment of variables helps)	
	this->maxPeriodInIPS = maxPeriodInTicks;
	this->minPeriodInIPS = minPeriodInTicks;	

    // calculate these once, for efficiency
	this->maxPeriodforTimer = ipsToTicks(maxPeriodInTicks);
	this->minPeriodforTimer = ipsToTicks(minPeriodInTicks);

	// return IPS (correction: if for some reason, timers are set to fire too late)
	return (u32) (((u64) max_period_in_ticks * TIMER_TICKS_PER_SECOND) >> 16);
}

}} // namespace
