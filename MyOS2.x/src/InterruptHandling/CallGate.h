#ifndef CALLGATE_H
#define CALLGATE_H

#include "defs.h"
#include "MM/Virtual/Selector.h"

namespace MyOS {
namespace InterruptHandling {

using namespace MM;

/// Represents a callgate descriptor used in the GDT/IDT
struct CallGate
{
    union {
        struct {
            u32 lo, hi;
        } lohi;

        struct {
            u16 offset15_0;

            // struct {	// cannot use struct or Selector: rounded up -> too big
            EDPL rpl    : 2;				// Requested Privilege level
            bool inLDT  : 1;
            u16 index   : 13;

            //} sel;

            u8 dword_count  : 5;		// Number of dwords copied to stack
            u32 _reserved   : 8;		// Value = 01100000 = 0x60, includes callgate type
            EDPL dpl        : 2;
            bool P          : 1;		// 1 = present
            u16 offset31_16;
        } bits;
    };

    /*  Compiler not very good at optimizing this ??
    CallGate( Selector sel, u32 offset, EDPL _dpl, u8 dwordstack, bool runIntsEna )
        :   offset15_0(offset&0xffff),
        rpl(sel.rpl()), inLDT(sel.inLDT()), index(sel.index()),
        dword_count(dwordstack), _reserved( 0x8E | runIntsEna ), dpl(_dpl), P(true),
        offset31_16(offset>>16)
    {
    }
    */

    /**
     *  Constructor
     *  @param  sel         -   Selector for segment in which code resides
     *  @param  offset      -   Offset of code within segment
     *  @param  dpl         -   Privilege level (0..3) for the code
     *  @param  runIntsEna  -   If true, code is called with ints enabled
     */
    CallGate( selector_t sel, u32 ofs, EDPL _dpl, u8 dwordstack, bool runIntsEna )
    {
        lohi.lo = ((u32) sel << 16) | (ofs&0xffff);
        lohi.hi = (ofs&0xffff0000) | 0x8E00 | (_dpl<<13) | ( ((u32)runIntsEna)<<8) | dwordstack;
    }

    // member functions
    inline u32 offset() const 	{ return ((bits.offset31_16 << 16) | bits.offset15_0); }
    inline selector_t CS() const { return Selector(bits.rpl,bits.inLDT,bits.index); }
} PACKED;

}}  // namespace

#endif

