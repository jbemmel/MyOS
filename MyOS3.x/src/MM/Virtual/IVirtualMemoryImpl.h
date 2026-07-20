#ifndef IVirtualMemoryImpl_H
#define IVirtualMemoryImpl_H

#include "MM/IVirtualMemory.h"

#include "Atomic/AtomicBitArray.h"
// for MMComponent
#include "MM/Physical/IPhysicalMemoryImpl.h"
#include "InterruptHandling/IInterruptHandling.h"

namespace MyOS
{
namespace MM
{

using Context::IContext;
using InterruptHandling::IInterruptHandling;
// install pagefault

class IVirtualMemoryImpl : public IVirtualMemory
{
public:
    IVirtualMemoryImpl(MyOS::Core::IComponent& c) INITSECTION;

    bool init(IContext& context, NVPAIR params[]) INITSECTION;
    void deinit(IContext& context);

    // XML
    virtual myos_result_t getStatistics(IO::OStream& out) const;

    // custom
    virtual linadr_t allocate(size_t order) throw(MM::OutOfMemoryException);
    virtual linadr_t allocateNoThrow(size_t order);
    virtual void free(linadr_t vmem, size_t order);

    // one page versions, optimized for this common case
    virtual linadr_t allocPage(physadr_t& mapping) throw(MM::OutOfMemoryException);
    virtual linadr_t allocPageNoThrow(physadr_t& mapping);
    virtual void freePage(linadr_t vmem);

    virtual linadr_t allocateStack(size_t order, bool userMode) throw(MM::OutOfMemoryException);
    virtual void freeStack(linadr_t stack, size_t order);

    // Mapping MMIO
    virtual linadr_t mapMMIO(physadr_t physicalStart, u32 byteCount);
    virtual void unmapMMIO(linadr_t adr, u32 byteCount);

    // Allocating DMA memory suitable for ISA DMA (needed for floppy / sound)
    virtual linadr_t allocISADMA(u32 pageOrder, physadr_t& physAdr);
    virtual void freeISADMA(linadr_t mem, u32 pageOrder);

private:
    /// bitmap of available kernel pages @ 0xFF800000 - 0xFFC00000
    AtomicBitArray< 1024 > kernelpages;
};

}
} // namespaces
#endif
