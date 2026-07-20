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

// To be used with interface methods	
class timer_t {
	void* opaque;

public:
	inline timer_t(void* o=0) : opaque(o) {}
		
	inline bool operator ==( const timer_t& t ) const {
		return this->opaque == t.opaque;
	}
	
	inline void* p() { return opaque; }
	
	inline timer_t& operator =( void* p ) {
		opaque = p;
		return *this;	
	}
};

// Callback function for timer? No, interface is nicer
// typedef void (*timerfunc_t)( timer_t& timer, void* userdata );

class ITimerTarget 
{
public:
	virtual void onTimer( /* timer_t */ ) = 0;	
};

}}	// namespace

#endif
