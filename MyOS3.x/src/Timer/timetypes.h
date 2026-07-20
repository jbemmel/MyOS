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



// Callback function for timer? No, interface is nicer
// typedef void (*timerfunc_t)( timer_t& timer, void* userdata );

class ITimer;

/**
 * Interface to be implemented by clients of the ITiming interface
 */
class ITimerTarget 
{
public:
	virtual void onTimer( ITimer& timer ) = 0;	
};

}}	// namespace

#endif
