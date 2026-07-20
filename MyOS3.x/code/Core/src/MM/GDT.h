#ifndef GDT_H
#define GDT_H

#include "defs.h"
#include "Atomic/atomic32.h"
#include "Atomic/atomic64.h"
#include "memtypes.h"   // EDPL
#include "Selector.h"
#include "SegmentDescriptor.h"

namespace MyOS {
namespace MM {

/**
 * Represents the Global Descriptor Table containing segment descriptors visible to all processes.
 *
 * All operations are atomic (which is overkill, as the GDT is typically static: initialized upon startup,
 * then not touched anymore)
 *
 */
class GDT
{
	friend class IVirtualMemoryImpl;              // caller of initialize()
	static void initialize() INITSECTION;  // no constructor: gcc duplicates them

public:

    /// Static design for the GDT structure
    enum E_GDT_INDEX {

        E_CODE_DPL0 = 1,
        E_DATA_DPL0 = 2,
        E_CODE_DPL3 = 3,
        E_DATA_DPL3 = 4,

        E_IRQ_DESC  = 5, //< Code segment used during IRQ/INT, same as E_CODE_DPL0 but different index
        E_TSS_DESC  = 6,
    };

    static inline SegmentDescriptor& getEntry( u32 i ) {
       return *((SegmentDescriptor*) &GDTable[ i * sizeof(SegmentDescriptor) ]);
    }

	static inline SegmentDescriptor& getEntry( Selector at )
	{
	    // assert( at.table == GDT )
	    return getEntry( at.index() );
	}

	static inline void clearEntry( Selector at )
	{
	    getEntry(at).asAtomic64()->clear();	// atomic
	}

	static selector_t addEntry( const SegmentDescriptor &desc, EDPL rpl );

	static selector_t cloneEntry( E_GDT_INDEX index, EDPL rpl );

private:
   enum { MAX_GDT_ENTRIES = 256 };

	static inline atomic32* count()
	{
		// use GDT[0] to count number of descriptors
		return (atomic32*) &GDTable[0];
	}

   // GCC 3.1 needs the ASMNAME here...
   static u8 GDTable[ MAX_GDT_ENTRIES * sizeof(SegmentDescriptor) ] TABLESECTION;
};

}}  // namespace

#endif // GDT_H
