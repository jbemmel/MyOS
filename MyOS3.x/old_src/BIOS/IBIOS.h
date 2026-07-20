#ifndef IBIOS_H
#define IBIOS_H

#include "Core/IInterface.h"
#include "mem.h"
#include "MM/memtypes.h"

using MyOS::Core::IInterface;
using MyOS::MM::linadr_t;

namespace MyOS
{
namespace BIOS
{

/// In/out parameters to VM86 (BIOS) calls
struct VM86REGS
{
    u16 di, si, bp, sp; // sp will be ignored
    u16 bx, dx, cx, ax;

    u16 ds, es;

    // Constructor, clears all registers to 0
    inline VM86REGS() throw()
    {
        clear();
    }

    inline void clear() throw()
    {
        memset_aligned( this, 0, sizeof(VM86REGS));
    }

} PACKED;

INTERFACE( IBIOS, "2ebf3820-423e-47cc-b13c-07eff2a2a6a5" )

    virtual myos_result_t vm86int( u8 i, VM86REGS& regs ) = 0;

    /**
     * Maps a given region of memory to below 1Mb, to make it accessible in
     * VM86 mode
     *
     * @param mem   - memory to map
     * @param bytes - size of memory region  
     * @return mapped address (under 1Mb physical)
     */
    virtual linadr_t vm86map( linadr_t mem, size_t bytes ) = 0;

    /**
     * Reverses the effect of a #vm86map call
     * @param vm86mem - return value of #vm86map call
     * @param bytes   - size of memory region
     */
    virtual void vm86unmap( linadr_t vm86mem, size_t bytes ) = 0;
};

}
} // namespaces
#endif
