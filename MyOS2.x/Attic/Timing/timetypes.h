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

/// Contains all timing related classes
namespace Timing {

enum ETimeUnit {
	common_time,	// not used (yet)
	cpucycles,     // rename to 'ticks' ?
	ns,
	us,
	timer_tick,
	ms,
	seconds,
	minutes,
	hours
};		// add : APIC timer, sound card timer, ...

}}	// namespace

#endif
