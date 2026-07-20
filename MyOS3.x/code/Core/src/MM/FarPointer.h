#ifndef FARPOINTER_H
#define FARPOINTER_H

#include "types.h"
#include "memtypes.h"
#include "Selector.h"

namespace MyOS {
namespace MM {

/// For passing around values, compiler is not smart enough
typedef u64 farptr_t;

/// Represents a far pointer ( segment:offset )
struct FarPointer
{
    union {
        struct {
            u32 ofs;
            selector_t sel;    // Selector has constructor
        } PACKED;
        farptr_t data;
    } PACKED;
    
    inline FarPointer( selector_t s, u32 offset ) 												
        : sel(s), ofs(offset) {}

    inline FarPointer( farptr_t ptr = 0 )                                               
        : data(ptr) {}
    
    
    inline Selector selector() const   { return sel; }
    inline u32 offset() const  { return ofs; }

    inline operator farptr_t() const { return data; }
    
    /**
     * Performs a far jump to the segment:offset denoted by this pointer
     */
    inline void jmp() const {
       ASMVOLATILE( "ljmp *%0;" : : "m"(ofs) : "memory" ); 
    }
} PACKED;

}}  // namespace

#endif

