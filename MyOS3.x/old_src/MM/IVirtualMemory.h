#ifndef IVirtualMemory_H
#define IVirtualMemory_H
#include "Core/IInterface.h"

using MyOS::Core::IInterface;

#include "memtypes.h"

namespace MyOS {
namespace MM {

/**
 * Interface for allocating virtual memory in the kernel range (0xFF800000-0xFFC00000)
 */
INTERFACE( IVirtualMemory, "2a147d6f-cab0-43b8-8b9f-4687312c101d" )

	/**
	 * Allocates a region of virtual memory, and physical memory to back it up
	 */
	virtual linadr_t allocate(size_t order) throw(MM::OutOfMemoryException) = 0;
	virtual linadr_t allocateNoThrow(size_t order) = 0;

	virtual void free(linadr_t vmem, size_t order) = 0;

	/** one page versions, optimized for this common case.
	 Note that 'mapping' currently does not include PTE bits */
	virtual linadr_t allocPage(physadr_t& mapping) throw(MM::OutOfMemoryException) = 0;
	virtual linadr_t allocPageNoThrow(physadr_t& mapping) = 0;
	virtual void freePage(linadr_t vmem) = 0;

	/**
	 * Specialized functions
	 */

	/**
	 * Allocates pages in kernel memory range for a thread stack.
	 * Like allocate(), but an additional unmapped 'guard page' is allocated as
	 * a first page, in order to catch stack overflows
	 *
	 * @param order    - log2 of number of pages to allocate (excluding guard page)
	 * @param userMode - if true, set stack pages to be read/write from usermode
	 *
	 * @return address of the mapped page after the guard page
	 */
	virtual linadr_t allocateStack( size_t order, bool userMode=false ) throw(MM::OutOfMemoryException) = 0;

	/**
	 * Frees a stack region allocated via #allocateStack()
	 */
	virtual void freeStack(linadr_t stack, size_t order) = 0;

	/**
	 * Maps a region of physical memory into virtual memory for memory-based IO
	 * @param physicalStart - first physical address to map
	 * @param byteCount     - number of bytes to map
	 *
	 * @return linear address of mapped virtual memory range
	 */
	virtual linadr_t mapMMIO(physadr_t physicalStart, u32 byteCount) = 0;

	/**
	 * Unmaps a region previously mapped via #mapMMIO
	 * @param adr       - linear address as returned by #mapMMIO
	 * @param byteCount - number of bytes mapped
	 */
	virtual void unmapMMIO(linadr_t adr, u32 byteCount) = 0;

	/**
	 * Allocates ISA-DMA compatible memory, i.e. below 1Mb and not crossing 64Kb alignment
	 */
	virtual linadr_t allocISADMA(u32 pageOrder, physadr_t &physAdr) = 0;
	virtual void freeISADMA(linadr_t mem, u32 pageOrder) = 0;

	virtual myos_result_t getStatistics(IO::OStream& out) const = 0;
};

}
} // namespaces
#endif
