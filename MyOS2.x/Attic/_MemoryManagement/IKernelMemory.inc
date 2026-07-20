//
//  The KernelMemory interface is used by drivers and other low-level components
//  These are the native extensions to the interface, not declarable in XML (yet)
//

#ifdef INCLUDE_PHASE
#include "memtypes.h"
#include "Exceptions/MyOSExceptions.h"

using MyOS::Exceptions::OutOfMemoryException;

#else

/// Bit flags used for allocate()
// Perhaps these should match more with MemoryManagement::PTEFLAGS
enum KMFLAGS {
   E_KM_NONE       = 0x0,  ///< No flags
   E_KM_READ_ONLY  = 0x1,  ///< Allocate the memory READ-ONLY, writing causes a pagefault exception
   E_KM_NO_CACHE   = 0x2,  ///< Allocate uncached memory
   E_KM_LOCKED     = 0x4,  ///< Allocate unpaged memory (excluded from page swapping)
   E_KM_CONTIGUOUS = 0x8,  ///< Allocate physically contiguous pages

   E_KM_DMA        = E_KM_NO_CACHE | E_KM_LOCKED | E_KM_CONTIGUOUS,    ///< Pages suitable for DMA

   /// Allocate DMA memory below 1Mb physical and not crossing a 64Kb boundary
   //  for old X86 style DMA controllers
   E_KM_BELOW_1MB  = 0x10 | E_KM_DMA,

   // ...more?  alignment ?
};

/// set of flagbits related to Kernel Memory management, @see KMFLAGS bit constants
typedef u32 kmflags_t;

/**
 * Allocates a block of linear memory
 * @param requestor  :  Handle to the component requesting the memory
 * @param bytes      :  Requested amount of bytes (is rounded up to multiple of 4)
 * @param flags      :  Optional KMFLAGS bits (default E_KM_NONE), @see KMFLAGS
 *
 * @return linear address of allocated memory, <code> null </code> if failed
 *
 * @todo Memory is now always physically colocated, no need for that!
 */
virtual linadr_t allocate( MyOS::Core::IComponent &requestor, size_t bytes, kmflags_t flags=E_KM_NONE ) 
	throw( OutOfMemoryException ) = 0;

/**
 * Free a block of linear memory previously allocated with allocate
 * @param address   :   The linear start address of the memory to free
 * @param bytes     :   The size of the block to free
 * @param flags     :   Optional KMFLAGS matching the flags at allocate() (!)
 *
 * @note It is possible to only partly free a block of memory previously
 *       allocated via allocate
 */
virtual void free( linadr_t address, size_t bytes, kmflags_t flags=E_KM_NONE ) = 0;

/**
 * Enables memory-mapped access to a region of device IO memory
 * @param physicalStart : Physical startaddress of device memory
 * @param bytes         : Size of the region in bytes
 * @return linear address for accessing the device, <code>null</code> if failed
 */
virtual linadr_t mapMMIO( physadr_t physicalStart, size_t bytes, linadr_t hint=0 ) = 0;

/**
 * Ends memory-mapped access to a region of device IO memory
 * @param linstart      : Linear starting address returned by mapMMIO
 * @param bytes         : Size of the region in bytes
 */
virtual void releaseMMIO( linadr_t linstart, size_t bytes ) = 0;

/**
 * Determines the physical page for a given linear address
 * @param forAdr    : Linear address of interest
 * @return The physical address to which this address is mapped, 0 if not mapped
 */
virtual physadr_t getMapping( const linadr_t forAdr ) const = 0;

/**
 * Returns the Page Table Entry(PTE) bits (12 bits) set for a particular address
 * @param forAdr    : Linear address of interest
 * @return The PTE bits for the page table entry of the given linear address
 */
virtual pteflags_t getAttributes( const linadr_t forAdr ) const = 0;

/**
 * Changes the memory attribute flags for a region of linear memory
 * @param address   : linear start of the memory region to change
 * @param bytes     : Size of the region to change
 * @param newatts   : Attributes to set for the region see PTEFLAGS (_not_ KMFLAGS!)
 *
 * @todo See how well this works, put some restrictions
 */
virtual void setAttributes( linadr_t address, size_t bytes, pteflags_t newatts ) const = 0;

/// Remap a whole region by copying PTE entries src -> dest
/**
 * Afterwards the dest region is an alias for the src region
 * @param dest            : Linear address of destination region
 * @param src             : Linear address of source region
 * @param pagecount       : Number of page mappings to copy
 * @param atts            : Attributes to set for copy, if ==0 original
 *                          settings for the region will be copied
 * 
 * Note: PTE attributes that may be set: 
 *   E_PAGE_USER | E_PAGE_ACCESSED | E_PAGE_DIRTY | E_PAGE_AVAIL0..2
 *   E_PAGE_PRESENT | E_PAGE_READWRITE | E_PAGE_GLOBAL
 */
virtual void mapRegion( linadr_t dest, linadr_t src, size_t pagecount, pteflags_t atts ) = 0;

/// Counterpart of @see mapRegion
virtual void unmapRegion( linadr_t dest, size_t pagecount ) = 0;

/**
 * Maps physical page 0 (BIOS) read-only and returns a linear address to read it
 */
virtual linadr_t getBIOSPage() = 0;

/**
 * This interface definitely needs redesign...
 * This routine is used by Thread to create a stack with a guard page at the top
 *
 * @param pages : Size of the stack (in pages), max. 128Mb
 * @return Linear address *after* the unmapped guard page
 */
virtual linadr_t createStack( size_t pages = 1 ) = 0;

/** Counterpart of createStack */
virtual void freeStack( linadr_t stack, size_t pages ) = 0;

#endif
