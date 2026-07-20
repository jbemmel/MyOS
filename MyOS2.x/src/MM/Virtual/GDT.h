#ifndef GDT_H
#define GDT_H

#include "defs.h"
#include "Atomic/atomic32.h"
#include "Atomic/atomic64.h"
#include "../memtypes.h"   // EDPL
#include "Selector.h"
#include "SegmentDescriptor.h"

namespace MyOS {
namespace MM {

/// Represents the Global Descriptor Table containing segment descriptors visible to all processes
class GDT
{
	friend class IVirtualMemoryImpl;        // caller of initialize()
	static void initialize() INITSECTION;  // no constructor: gcc duplicates them

public:
	static inline SegmentDescriptor& getEntry( u32 i )
	{
		return GDTable[i];
	}

	static inline void clearEntry( Selector at )
	{
	  	GDTable[ at.index() ].clear();	// atomic
	}

	static selector_t addEntry( const SegmentDescriptor &desc, EDPL rpl );

private:
   enum { MAX_GDT_ENTRIES = 256 };

	static inline atomic32* count()
	{
		// use GDT[0] to count number of descriptors
		return (atomic32*) &GDTable[0];
	}

   // GCC 3.1 needs the ASMNAME here...
   static ATOMIC64< SegmentDescriptor > GDTable[ MAX_GDT_ENTRIES ] ASMNAME( "theGDT_mem" ) TABLESECTION;
};

}}  // namespace

#endif // GDT_H
