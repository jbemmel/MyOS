/***************************************************************************
                          Timing.h  -  description
                             -------------------
    begin                : Tue Oct 16 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TIMETYPES_H
#define TIMETYPES_H

namespace MyOS {

/// Contains all timer related classes
namespace Timer {

/**
 * Time units
 */
enum ETimeUnit {
	common_time,	// not used (yet)
	cpucycles,     // rename to 'ticks' ?
	ns,
	us,
	timer_ticks,
	ms,
	seconds,
	minutes,
	hours
};		// add : APIC timer, sound card timer, ...


class ITimer;

/**
 * Interface to be implemented by clients of the Timer::ITimerFacility interface
 */
class ITimerTarget 
{
public:
    /**
     * Callback function to be implemented by users of ITimerFacility.
     * @param timer - timer that fired
     * 
     * @note Called in a IRQ context (IRQ0 or local APIC IRQ)
     * 
     * @throws : Nothing, callee must make sure no exceptions escape
     */
	virtual void onTimer( ITimer& timer ) throw() = 0;	
};

}}	// namespace

#endif
