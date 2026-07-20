/***************************************************************************
                          SegmentDescriptor.cpp  -  description
                             -------------------
    begin                : Sun Apr 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "SegmentDescriptor.h"

namespace MyOS {
namespace MM {

void
SegmentDescriptor::init( u32 dt, u32 _attr_or_type, u32 base, u32 limit, EDPL dpl )
{
   limit15_0 = limit;
	base15_0 = base;
	base23_16 = base >> 16;
	attr_or_type = _attr_or_type;
	DT = dt;
	DPL = dpl;
	P = true;				// always present assumed
	limit19_16 = limit >> 16;
	AVL = 0;
	_RES = 0;
	DB = 1;		// always 32-bit assumed
	G = dt;		// system: byte granular, data segments: page granular
	base31_24 = base >> 24;
}

}} // namespace
