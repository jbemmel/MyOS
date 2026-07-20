/***************************************************************************
                          TimeUtil.h  -  description
                             -------------------
    begin                : Sun Apr 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#include "types.h"
#include "timetypes.h"
#include "TSC.h"
// #include "Assertion.h"  // BUG

namespace MyOS {
namespace Timer {

/**
 * Utility class (static) containing time conversion routines
 *
 * @author Jeroen van Bemmel
 */
class TimeUtil
{
    friend class ITimerFacilityImpl;
    static void initIPS( u32 measuredIPS ) INITSECTION;
    
public:


	static u32 now( ETimeUnit unit ) ;

	inline static u32 getTicksPerSecond() { return ips; }

    /// Returns <ticks> converted to seconds, current time in seconds iff ticks==0
    static u32 getSeconds( u64 ticks = 0 );

    template <ETimeUnit unit>
    inline static u32 convertTicks( u64 ticks );

    template <ETimeUnit unit>
    inline static u64 time2ticks( u32 time );

	/// busy wait-loop for exactly 'unit'*time delay
	template <ETimeUnit unit>
	inline static void delay( u32 time );

private:
	static u32 ips;

	// array of divisors
	static u32 divisor[];
};

/*
template <ETimeUnit unit>
inline u32 TimeUtil::convertTicks( u32 ticks )	// looses bits...
{
	switch (unit)
	{
	case Timing::cpucycles:	return ticks;
	case Timing::ns:	      return FASTDIV( ticks, multiplier_ns );
	case Timing::us:	      return FASTDIV( ticks, multiplier_us );
	case Timing::ms:	      return FASTDIV( ticks, multiplier_ms );
	case Timing::seconds:	return FASTDIV( ticks, multiplier_ms ) /    1000; // return getSeconds( ticks );
	case Timing::minutes:	return FASTDIV( ticks, multiplier_ms ) /   60000;
	case Timing::hours:	   return FASTDIV( ticks, multiplier_ms ) / 3600000;
	default:	return 1;
	}
}
*/

template <ETimeUnit unit>
inline u32 TimeUtil::convertTicks( u64 ticks )
{
   u32 result;
   ASMVOLATILE(
      "div %2  \n"
      : "=a"(result)
      : "A"(ticks), "c"( divisor[unit] )
   );
   return result;
}


template <ETimeUnit unit>
inline u64 TimeUtil::time2ticks( u32 time )
{
   return time * divisor[unit];
}

template <ETimeUnit unit>
inline void TimeUtil::delay( u32 time )
{
	TSC::timestamp end = TSC::read() + time2ticks<unit>( time );
	while ( TSC::read() < end );
}


}}   // namespace

#endif
