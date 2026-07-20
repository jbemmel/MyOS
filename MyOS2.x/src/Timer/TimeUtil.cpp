/***************************************************************************
                          TimeUtil.cpp  -  description
                             -------------------
    begin                : Sun Apr 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#include "TimeUtil.h"
#include "TSC.h"
#include "debug.h"

namespace MyOS {
namespace Timer {

// static
u32 TimeUtil::ips;

// divisors indexed by unit
u32 TimeUtil::divisor[hours+1];

/*    requires __udivdi3
u32
TimeUtil::getSeconds( u64 ticks )
{
   u64 q = ticks ? ticks : TSC::read();
   return q / ips;	// must be separate routine, calls C code
}
*/

/**
 * This function could called _TWO_ times:
 * - Once with a bogus (but reasonable) value
 * - Thereafter, once with the real measured value of IPS
 * 
 *.. but is only called once now
 */
void
TimeUtil::initIPS( u32 measured_ips ) // INITSECTION
{
	// could enable write_back here
	ips = measured_ips;

   // to prevent div by zero
   for (u32 i=0; i<sizeof(divisor)/sizeof(u32); ++i) divisor[i]=1;

   divisor[cpucycles] = 1;
   divisor[ms] = FASTDIV<1000>(ips);
   divisor[us] = FASTDIV<1000>(divisor[ms]);
   divisor[ns] = FASTDIV<1000>(divisor[us]);
   divisor[seconds] = ips;
   divisor[minutes] = ips * 60;
   divisor[hours] = ips * 3600;

/*
   DPRINTK( "\nips=%u ipms=%u"
      "\nConvertTicks<ms>(ips) (should be 1000) : %u"
      "\nConvertTime<ms>(1) (should be ipms)    : %u", ips, divisor[ms]
	,  convertTicks<Timing::ms>( ips )
	,  convertTime<Timing::ms>( 1 ) );
*/   
	// while (1);
}

// static (was inline before, but bochs freezed on that...
u32 TimeUtil::now( ETimeUnit unit )
{
   // this calls udivdi :(
   // return TSC::read() / divisor[unit];
   u32 result;
   ASMVOLATILE(
      "rdtsc   \n"
      "divl %1 \n"   // EAX quotient, EDX remainder
      : "=a"(result)
      : "c" (divisor[unit])
   );
   return result;
}


}}  // namespace
