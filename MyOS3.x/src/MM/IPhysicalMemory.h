#ifndef IPhysicalMemory_H
#define IPhysicalMemory_H

#include "Core/IInterface.h"
#include "Exceptions/MyOSExceptions.h"

using MyOS::Core::IInterface;

#include "myosresult.h"
#include "memtypes.h"

namespace MyOS {
namespace MM {

/**
 * Interface for physical memory allocation.
 * 
 * Physical memory is allocated in pages (size:4KB on x86), the implementation
 * of this interface manages the various page tables. It allocates physical
 * pages and maps them into linear memory regions (as provided in parameters,
 * assignment of linear space is the responsibility of the caller).
 * 
 * Three types of physical memory are distinguished:
 * - regular memory
 * - DMA memory (below 1MB physical, implementation uses memory below 512KB physical)
 * - MMIO memory (device registers outside the physical memory banks)
 * 
 * @note implicitly operates on the memory mapping of the _current_ process
 * @todo enable changing mapping of other processes besides current
 */
INTERFACE( IPhysicalMemory, "e87d9b4e-9a74-40a0-93ef-4f1c1d47d2e8" )

    /// Constant that defines the size of a memory page, in bytes
    static const int PAGE_SIZE = 4096;

	/**
	 * Allocates physical frames and maps them to the specified linear range.
	 * 
	 * @param startaddr - linear address where region starts, MUST lie on a page boundary
	 * @param order     - log2 of number of pages needed
	 * @param atts      - pagetable attributes to set, see PTEFLAGS
	 * 
	 * @return Physical address of first page
	 * 
	 * @throw OutOfMemoryException - when insufficient physical frames are available
	 * 
	 * @note physical frames are allocated contiguously
	 */
	virtual physadr_t mapRegion(linadr_t startaddr, size_t order, pteflags_t atts) 
	    throw (MM::OutOfMemoryException) = 0;

	/**
	 * Unmaps the given linear region and releases physical frames allocated to it
	 *  
	 * @param startaddr - linear address where region starts, MUST lie on a page boundary
     * @param order     - log2 of number of pages allocated
     */
	virtual void releaseRegion(linadr_t startaddr, size_t order) = 0;

	/**
	 * Returns the number of available physical pages
	 * @return number of available pages
	 */
	virtual u32 getFreeFrames() const = 0;
	
	/**
	 * Determines the physical address mapping for the given linear address
	 * @param ofAddr - linear address to map
	 * @return physical address, <code>0</code> if not mapped
	 * 
	 * @note The page offset is copied from the input parameter, no PTE flag bits are returned
	 */ 
	virtual physadr_t getPhysicalAddress(linadr_t ofAddr) const = 0;

	/**
	 * Returns the page table attributes applicable for a given linear address
	 * @param ofAddr - linear address to map
	 * @return PTEFLAGS
	 */
	virtual pteflags_t getAttributes( linadr_t ofAddr ) const = 0;
	
	/**
	 * Sets the page table attributes applicable for a given region of linear memory
	 * @param startaddr - starting address to change the attributes for
	 * @param pages     - size of the region, in pages
	 * @param flags     - new page table attributes to set 
	 */
    virtual void setAttributes(linadr_t startaddr, size_t pages, pteflags_t flags) const = 0;
		
	/**
	 * Maps a given linear address onto a given physical address
	 * @param startaddr     - linear address to map to
	 * @param physicalStart - physical address, assumed to be outside physical memory range
	 * @param pageCount     - number of pages to map (typically small, i.e. 1)
	 * 
	 * @note consistency: pageCount is not log2 here
	 * @note region is assumed not to cross a 4MB border
	 */
	virtual void mapMMIO(linadr_t startaddr, physadr_t physicalStart, u32 pageCount) = 0;
	
	virtual void unmapMMIO(linadr_t linear, u32 pageCount) = 0;

	/**
	 * Allocates physical frames below 1MB physical and maps them to the specified linear range.
	 * @param startaddr - linear address where region starts, MUST lie on a page boundary
     * @param order     - log2 of number of pages needed, max. 7 supported
     * @return Physical address of first page
     * 
     * @note physical frames are allocated contiguously
     */
	virtual physadr_t mapDMA(linadr_t startaddr, u32 pageOrder) = 0;
	
	
	virtual void unmapDMA(linadr_t linear, u32 pageOrder) = 0;

	/**
	 * Copies the mapping of a given region of memory to another region
	 * (overwrites any existing mapping, assumes region is currently unmapped
	 * else physical frames are lost)
	 * 
	 * @param dest - region to map
	 * @param src  - region to copy
	 * @param pagecount - number of page mappings to copy, max. 1024
	 * @param atts - [optional] attributes to set, by default current attributes are copied
	 *
	 * @note 'createAlias' better ?
	 *  
	 * @note Implementation assumes the 'src' and 'dest' regions each lie within
	 *       a single pagetable directory (i.e. max 4Mb, and not crossing borders) 
	 */
	virtual void copyMapping(linadr_t dest, linadr_t src, size_t pagecount,
			pteflags_t atts = 0 ) = 0;

	virtual void clearMapping( linadr_t dest, size_t pagecount ) = 0;
	
	virtual myos_result_t listTables(IO::OStream& out) const = 0;

	virtual myos_result_t getStatistics(IO::OStream& out) const = 0;

};

}
} // namespaces
#endif
