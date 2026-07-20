#ifndef C8254_H
#define C8254_H

#include "defs.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "InterruptHandling/ihtypes.h"
#include "Devices/IOPort.h"

namespace MyOS {

using InterruptHandling::IInterruptHandling;
using InterruptHandling::IInterruptContext;

namespace Timing {

/// Container class for conversion between C8254 timings and processor timings
class C8254ConversionFactors
{
public:
   typedef u32 ips_t;
   typedef u32 timerticks_t;
    
   inline C8254ConversionFactors() {}

   /// Determines Instructions Per Second (IPS) in a timer period of <maxPeriod>
   ips_t initOnce( timerticks_t maxPeriod ) INITSECTION;

	inline ips_t getMaxPeriodInIPS() const    { return maxPeriodInIPS; }
	inline ips_t getMinPeriodInIPS() const    { return minPeriodInIPS; }

   // same in TimerTicks
	inline timerticks_t getMaxPeriodforTimer() const 		    { return maxPeriodforTimer; }
	inline timerticks_t getMinPeriodforTimer() const		    { return minPeriodforTimer; }
	
	inline timerticks_t ipsToTicks( ips_t ips ) { 
      return FASTIDIV( ips, ticks2c8254); 
   }

   // Converts IPS to timer ticks, but no larger than maximum timer capacity
   inline timerticks_t ipsToTicksWithMax( ips_t ips ) { 
      return ips < maxPeriodInIPS 
         ? FASTIDIV( ips, ticks2c8254) 
         : maxPeriodforTimer; 
   }
         

private:
	// safety limit: if too small a value is programmed, recursion in timer routines will occur !
	// put here, because template is cumbersome

	enum { SAFETY_LIMIT_LOW = 1, TIMER_TICKS_PER_SECOND = 1193180 /* Hz */ };

	u32 minPeriodInIPS, maxPeriodInIPS, ticks2c8254;
	u32 minPeriodforTimer, maxPeriodforTimer;
};

// Could be placed inside template definition, i.e. template < enum {...} ID >, but difficult (impossible?) to declare
enum ETimerID { E_TMR0_CNT0 = 0, E_TMR0_CNT1, E_TMR0_CNT2, E_TMR1_CNT0, E_TMR1_CNT1, E_TMR1_CNT2 };

/// Template for declaring timer(chip) instances
template< ETimerID ID >
class C8254
{
public:
//	enum { IRQ0_RUNS_CLI = true };
   C8254() : _isenabled(false) {}

	inline myos_result_t init( IInterruptHandling& intsup, IInterruptContext& context )
   {  
      if (ID<E_TMR1_CNT0)
         PIT0MCR = ((ID << SelCountIndex) | RW_Both | Mode0 | Binary );
      else
         PIT1MCR = ( ((ID-E_TMR1_CNT0) << SelCountIndex) | RW_Both | Mode0 | Binary );
        
      if (ID==E_TMR0_CNT0)
      {
         // the timer routine is made a TRAP -> ints enabled when the routine executes
         return intsup.setIRQHandler( InterruptHandling::E_IRQ0, context, InterruptHandling::E_ENABLE_INTS );
      }
      return E_MYOS_SUCCESS;
   }
  
	
	inline void enable( IInterruptHandling& intsup )
	{
		if (_isenabled==false)
		{
			_isenabled = true;	// Moved here: When enabled, the interrupt occurs ??
			switch (ID)
			{
			case TMR0_CNT0: 
            DPRINTK( "\nEnabling IRQ0..." );
            intsup.enableIRQ( IInterruptHandling::E_IRQ0 ); break;
			case TMR0_CNT2: PIT1GATE = 1; break;
			default:	
            BUG( "Invalid timer ID" );				
			}
			// _isenabled = true;
		}
	}

	inline void disable( IInterruptHandling& intsup )
	{
		if (_isenabled)
		{
			// theoretically, this can be interrupted before _isenabled is set...
			switch (ID)
			{
			case TMR0_CNT0: intsup.disableIRQ( IInterruptHandling::E_IRQ0 ); break;
			case TMR0_CNT2: PIT1GATE = 0; break;
			default:	ASSERTs( false, "Invalid timer ID" );
			}
			_isenabled = false;	// move to front ??
		}
	}
	
	inline bool isEnabled() const       { return _isenabled;}

   // programmed in _timer_ticks_
	inline void setTimer( u32 v );

	inline void doMaxPeriod();

	/* PROTECTED_REGION functions
	typedef PROTECTED_REGION <C8254<ID> > PROTECTED;
	void ENTER() { Disable(); }
	void LEAVE() { Enable(); }
    */

private:
	
	// Counting format (bit 0 of control word)
	enum { Binary = 0, BCD = 1 };

	// Timer count modes (bits 3..1 of control word)
	enum { Mode0 = 0, Mode1 = (1<<1), Mode2 = (1<<2), Mode3 = (1<<3), Mode4 = (1<<4), Mode5 = (1<<5) };

	// Timer read/write modus (bits 5..4)
	enum { Latch = 0, RW_Low = 0x10, RW_High = 0x20, RW_Both = RW_Low+RW_High };

	// select CNT bits in control word (7..6)
	enum { SelCountIndex = 6 };
	
	// Mode control registers for both PITS
	IOPort8 < 0x43 > PIT0MCR;
	IOPort8 < 0x4b > PIT1MCR;

	// CNT registers
	// enum { PIT0_CNT0 = 0x40, PIT0_CNT1 = 0x41, PIT0_CNT2 = 0x42,
	// PIT1_CNT0 = 0x48, PIT1_CNT1 = 0x49, PIT1_CNT2 = 0x4a };
	IOPort8 < 0x40 > PIT0CNT0;

	// Gate register for PIT2, bit 0 enables(1)/disables(0)
	IOPort8 < 0x61 > PIT1GATE;

	// variables
	volatile bool _isenabled;
};

template < ETimerID ID >
inline void C8254<ID>::setTimer( u32 value )
{
	// if (value!=0) ASSERT_RANGE( value, C8254ConversionFactors::SAFETY_LIMIT_LOW, 0xFFFF );
	switch (ID)
	{
		case E_TMR0_CNT0 : 
            PIT0CNT0.out2( value );	
            break;
		default:	
            BUG( "Timer not implemented");
	}
}

/// This function will start the timer for the longest period possible
/// @note This must be independent of any conversion factors!
template < ETimerID ID >
inline void C8254<ID>::doMaxPeriod()
{
	setTimer( 0 );
}

}}  // namespace

#endif //C8254_H
