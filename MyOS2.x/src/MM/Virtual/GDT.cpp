#include "GDT.h"
#include "mem.h"     // memcpy_aligned

// Cannot use yet, crashes (bootstrap)
// #include "debug.h"

namespace MyOS {
namespace MM {

// ASMNAME: used from Thread.cpp when MultiProcessing is enabled
ATOMIC64< SegmentDescriptor > GDT::GDTable[ MAX_GDT_ENTRIES ] ASMNAME( "GDT" );

// helper class to get/set the GDT address
class GDTAddress
{
	u16 __align__bogus;
	u16 limit;
	SegmentDescriptor* address;

public:
	inline GDTAddress();
	inline void load( SegmentDescriptor& new_address, u16 nEntries );
	inline SegmentDescriptor& operator[]( u32 i)	{ return address[i]; }
	inline SegmentDescriptor* getAddress() const	{ return address; }
} PACKED;

inline GDTAddress::GDTAddress()
{
	// get current address of GDT
	ASMVOLATILE( "sgdt	(%0)" : : "r"(&limit) : "memory" );
}

inline void GDTAddress::load( SegmentDescriptor& newGDT, u16 nEntries )
{
	limit = nEntries * sizeof(SegmentDescriptor) - 1;	// in bytes, minus 1
	address = &newGDT;
	ASMVOLATILE( "lgdt	(%0)" : : "r"(&limit) : "memory" );
}

void
GDT::initialize()		// INITSECTION
{
	// XX Could clear entries...Or is table memory cleared elsewhere ?		
	GDTAddress curGDT;

	* ((size_t*) count()) = 2;	// NULL desc=0, CODE=0x8, DATA=0x10

	memcpy_aligned( &GDTable[1], &curGDT[1], 2 * sizeof(SegmentDescriptor) );
	curGDT.load( getEntry(0), MAX_GDT_ENTRIES );

	// This code used to be in the bootloader (does not fit anymore)
	// and we do like our segregs clean (some BIOSes don't clear them)
	ASMVOLATILE( "movw %%ax, %%fs; movw %%ax, %%gs" :  :"a"(0) );

   // see http://www.sudleyplace.com/dpmione/expanddown.htm
   //
   // Apply a trick to catch null pointers using segment limit checks!
   // Patch the DS descriptor (#2 in bootsec.asm)
   // getEntry(2).setAttributes( E_EXPAND_DOWN | E_WRITABLE );
   // getEntry(2).setLimit( 1 );    // avoid page 0 to be touched
   //
   // See Intel manual: EXPAND_DOWN will allow addresses between
   // (limit+1) to 0xFFFFFFFF

   /* reload segment registers to activate...
   ASMVOLATILE( 
      "movw %ds, %ax \t\n"
      "movw %ax, %ds \t\n"
      "movw %ax, %es \t\n"
      "movw %ax, %fs \t\n"
      "movw %ax, %gs \t\n"
   ); */

   // could do the same for CS...

   // PRINTF( "\nGDT initialized" );
}

selector_t
GDT::addEntry( const SegmentDescriptor &desc, EDPL rpl )
{
	// DPRINTK( "\nGDT::addEntry curCount=%d", count() );
	register size_t index = ++*count();		            // first increments, then reads

	GDTable[ index ].assignAtomic( desc );		// atomic
	return Selector(rpl, Selector::E_GDT, index);
}

}}  // namespace
