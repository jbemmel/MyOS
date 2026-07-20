#ifndef IVirtualMemoryImpl_H
#define IVirtualMemoryImpl_H

#include "IVirtualMemory.h"

// for MMComponent
#include "IInterruptHandling.h"

// impl
#include "Page.h"
#include "BitBuddyAllocator.hpp"
#include "IPaging.h"
#include "Atomic/AtomicBitArray.h"

namespace MyOS
{
namespace MM
{

using Context::IContext;
using InterruptHandling::IInterruptHandling;
// install pagefault

class IVirtualMemoryImpl: public IVirtualMemory
{
public:
    IVirtualMemoryImpl(MyOS::Core::IComponent& c) INITSECTION;

    bool init(IContext& context, NVPAIR params[]) INITSECTION;
    void deinit(IContext& context);

    // XML
    virtual myos_result_t getStatistics(IO::OStream& out) const;

    // custom
    virtual linadr_t allocate( size_t order, physadr_t* mapping = 0 ) throw (MM::OutOfMemoryException);
    virtual linadr_t allocateNoThrow(size_t order, physadr_t* mapping = 0);
    virtual void free(linadr_t vmem, size_t order);

    // one page versions, optimized for this common case
    virtual linadr_t allocPage(physadr_t& mapping)
            throw(MM::OutOfMemoryException);
    virtual linadr_t allocPageNoThrow(physadr_t& mapping);
    virtual void freePage(linadr_t vmem);

    virtual linadr_t mmap(size_t order, bool executable)
            throw (MM::OutOfMemoryException);
    virtual void unmap( linadr_t start, size_t order );

    virtual void copyMapping( linadr_t dst, linadr_t src, size_t pagecount, pteflags_t atts );

    virtual linadr_t allocateStack(size_t order, bool userMode)
            throw(MM::OutOfMemoryException);
    virtual void freeStack(linadr_t stack, size_t order);

    // Mapping MMIO
    virtual linadr_t mapMMIO(physadr_t physicalStart, u32 byteCount);
    virtual void unmapMMIO(linadr_t adr, u32 byteCount);

    // Allocating DMA memory suitable for ISA DMA (needed for floppy / sound)
    virtual linadr_t allocISADMA(u32 pageOrder, physadr_t& physAdr);
    virtual void freeISADMA(linadr_t mem, u32 pageOrder);

    inline static InterruptHandling::myos_TRAP_f getPageFaultHandler() {
        return (InterruptHandling::myos_TRAP_f) pagefaultAsm;
    }

private:
    /// bitmap of available kernel code pages @ 0xFF800000 - 0xFFA00000
    AtomicBitArray<512, 0> kernel_Codepages;

    /// bitmap of available kernel data pages @ 0xFFA00000 - 0xFFC00000
    AtomicBitArray<512, 512> kernel_Datapages;

    /***
     * Physical memory management
     */
    bool initPhysical(Context::IContext& context, NVPAIR params[]) INITSECTION;

    physadr_t mapRegion(linadr_t startaddr, size_t order,
            pteflags_t atts) throw (MM::OutOfMemoryException);

    void releaseRegion(linadr_t startaddr, size_t order);

    u32 getFreeFrames() const;
    virtual physadr_t getPhysicalAddress(linadr_t ofAddr) const;
    pteflags_t getAttributes(linadr_t ofAddr) const;

    void mapMMIOPhysical(linadr_t linear, physadr_t physicalStart,
            u32 pageOrder);
    void unmapMMIOPhysical(linadr_t linear, u32 pageOrder);

    physadr_t mapDMA(linadr_t linear, u32 pageOrder);
    void unmapDMA(linadr_t linear, u32 pageOrder);

    void setAttributes(linadr_t forAdr, size_t pages, pteflags_t flags) const;

    void clearMapping(linadr_t dest, size_t pagecount);

    // XML
    virtual Core::IScriptable* getScriptable() const throw();
    myos_result_t listTables(IO::OStream& out) const;

    // Called after system init has finished
    friend class MMComponent;
    void freeInitPages();

    inline size_t getBitmapPages() const
    {
        return ALLOCATOR::getPageCount();
    }

    // demand paging support (called by mmap)
    void fillEntries(linadr_t startaddr, size_t pagecount, u32 bits, u32 increment);

private:

    // PBT, PageDirectory can call this method too
    friend class PBT;

    inline physadr_t allocateFrame()
    {
        //Locking::LockHelper lock( memlock );
        return pages.allocate(0);
    }
    inline void releaseFrame(physadr_t frame)
    {
        //Locking::LockHelper lock( memlock );
        pages.free(frame, 0);
    }

    u32 totalPhysicalMemory;

    // Max. physical mem supported 1Gb
    typedef BitBuddyAllocator<30, 12> ALLOCATOR;
    ALLOCATOR pages; // should be aligned to 32 bytes!

    // Separate memory for below 1Mb pages, for (floppy) DMA
    u32 dmaMemory512Kb;

    // IPaging impl
    static void pagefaultAsm() throw (PagingException) ASMNAME("PFASM");
    static void pagefault(linadr_t adr, u32 ec) throw (PagingException) ASMNAME("pagefault") REGPARM(2) ;

    static void doThrow( void* adr, int cause) throw (PagingException) NORETURN;

    static PagingDevice* devices[32]; //< Registered paging devices, #0 = reserved

};

}
} // namespaces
#endif
