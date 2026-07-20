/***************************************************************************
                          TSC.h  -  description
                             -------------------
    begin                : Mon Sep 17 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TSC_H
#define TSC_H

#include "types.h"

namespace MyOS {
namespace Timer {

/// Represents the 64bit TimeStampCounter register
class TSC
{
public:
	typedef u64 timestamp;

	inline static timestamp read()
	{
		/* register */ timestamp t;
		ASMVOLATILE( "rdtsc" : "=A"(t) );  // with clobbered, gcc fails
		return t;
	}
};

}}   // namespace

#endif

