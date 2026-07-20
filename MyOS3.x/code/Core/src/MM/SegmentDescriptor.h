/***************************************************************************
 SegmentDescriptor.h  -  description
 -------------------
 begin                : Sun Apr 28 2002
 copyright            : (C) 2002 by Jeroen van Bemmel
 email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef SEGMENTDESCRIPTOR_H
#define SEGMENTDESCRIPTOR_H

#include "memtypes.h"
#include "Atomic/atomic64.h"

namespace MyOS
{
namespace MM
{

/// Segmenttypes, TSS and gates are i386 kind (of course)
enum E_SEGTYPE /* DT = 0 */
{
    SEG_LDT = 2,
    SEG_TSS = 9,
    SEG_TSS_ACT = 11, ///< Active TSS
    CALL_GATE = 12,
    INT_GATE = 14,
    TRAP_GATE = 15
};

/// Segment attributes, for creating data segments
enum E_SEGATTRIBUTES /* DT = 1 */
{

    //	E_ACCESSED 	   = 1,  ///< Statistics bit

    // for data segments
    E_WRITABLE = 2,
    E_EXPAND_DOWN = 4,

    // for code segments
    E_EXECUTEREAD = 2, ///< Both executable and readable
    E_CONFORMING = 4,  ///< If set, segment gets CPL from caller
    E_EXECUTABLE = 8,
};

/// Segment descriptors, can be in GDT, IDT or LDT
class SegmentDescriptor
{
    u16 limit15_0;
    u16 base15_0;
    u8 base23_16;
    int attr_or_type : 4; // mask of E_SEGATTRIBUTES or E_SEGTYPE
    int DT : 1; // 0 = system segment, 1 = application segment
    EDPL DPL : 2;
    bool P : 1; // 1 = present
    int limit19_16 : 4;
    int AVL : 1;
    int _RES : 1;
    int DB : 1; // 0 = 16 bit address/data, 1 = 32 bit address/data
    int G : 1; // 0 = byte granularity, 1 = page granularity
    u8 base31_24;

    void init(u32 dt, u32 _attr_or_type, u32 base, u32 limit, EDPL dpl);

public:
    typedef u32 segatt_t; ///< Bitmask of E_SEGATTRIBUTES

    /// Creates a special segment descriptor (type,base,limit) at a certain dpl
    inline SegmentDescriptor(E_SEGTYPE t, linadr_t base, u32 limit_in_bytes,
            EDPL dpl)
    {
        init( 0, t, (u32) base, limit_in_bytes, dpl );
    }

    /// Creates a code or datasegment descriptor with (base,limit) at a certain dpl
    /**
     * @param att          - Attributes for this segment, see E_SEGATTRIBUTES
     * @param base         - Linear address of offset 0
     * @param limitInPages - Size in pages - 1, valid range 0..0xFFFFF
     * @param dpl          - Descriptor Privilege level (0..3)
     */
    inline SegmentDescriptor(segatt_t att, linadr_t base, u32 limitInPages,
            EDPL dpl)
    {
        init( 1, att, (u32) base, limitInPages, dpl );
    }

    inline u32 getBase() const
    {
        return ((base31_24 << 24) | (base23_16 << 16)| (base15_0));
    }
    inline u32 getLimit() const
    {
        return (limit19_16<<16) | (limit15_0);
    }

    inline segatt_t getAttributes() const { 
        return this->attr_or_type; 
    }
    
    inline void setAttributes(segatt_t atts)
    {
        this->attr_or_type = atts;
    }

    inline void setBase(u32 base)
    {
        base15_0 = base & 0xFFFF;
        base23_16 = (base>>16) & 0xFF;
        base31_24 = (base>>24) & 0xFF;
    }

    /**
     * Sets the limit for this segment, in units of bytes(system seg) or pages
     */
    inline void setLimit(u32 limit20bits)
    {
        limit15_0 = (limit20bits ) & 0xFFFF;
        limit19_16 = (limit20bits >> 16) & 0xF; // '&' for safety, not needed
    }

    inline atomic64* asAtomic64() {
        return (atomic64*) this;
    }

    inline const atomic64 * asAtomic64() const {
        return (atomic64*) this;
    }

    
}PACKED;

}
} // namespace

#endif
