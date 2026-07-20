#ifndef SELECTOR_H
#define SELECTOR_H

#include "types.h"
#include "../memtypes.h"

namespace MyOS {
namespace MM {

/// For passing around selector values, compiler is not smart enough
typedef u32 selector_t;

/// Represents a segment selector
struct Selector
{
    union {
        struct {
            EDPL RPL 	: 2;		// Requested Privilege level
            bool inLDT 	: 1;
            u16 index 	: 13;
            
            // size rounded up to multiple of 4, also without this _unused_ part. Easier to work with 32 bits
            u16 _unused : 16;
        } PACKED s;
        selector_t data;
    } PACKED;
    
    enum ETABLE { E_GDT = 0, E_LDT = 0x4 };
    
    inline Selector( u32 _index, EDPL _rpl, ETABLE t = E_GDT ) 		
        : data(0) 	{ data |= ((_index<<3) | t | _rpl); }
    
    inline Selector( EDPL _rpl, bool in_ldt, u32 _index ) 
        : data(0) 	{ data |= ((_index<<3) | (in_ldt<<2) | _rpl); }
    
    inline Selector( selector_t sel = 0 ) 												
        : data(sel) 	{}
    
    inline u16 index() const  { return s.index; }
    inline bool inLDT() const { return s.inLDT; }
    inline EDPL rpl() const   { return s.RPL; }

    inline void setRPL( EDPL rpl ) {
        s.RPL = rpl;
    }
    
    inline operator selector_t() const { return data; }
} PACKED;

}}  // namespace

#endif

