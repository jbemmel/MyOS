#ifndef IPagingRegistry_H
#define IPagingRegistry_H

#include "IInterface.h"

using MyOS::Core::IInterface;

#include "memtypes.h"

namespace MyOS {
namespace MM {

/**
 * Callback interface for paging devices
 */
class PagingDevice {
public:
   /**
    * Callback to load the given page into memory
    * 
    * @param index  - 24-bit index of demand loaded entity being requested
    * @param targetAddress - address of the page to load (allocated, present) 
    */
   virtual void loadPage( u32 index, linadr_t targetAddress ) 
       throw (PagingException) = 0;
};

/**
 * Interface for supporting demand paging devices
 */
INTERFACE( IPaging, "de7134f4-70fc-4a14-aadb-6ade3764ca3d" )
	
	/**
	 * Registers a paging device instance
	 * 
	 * MyOS supports up to 31 paging devices (swap files or partitions, floppy disks, network
	 * device), each supporting up to 2^24 4K pages for a total of 1Tb demand-loaded memory
	 * 
	 * @param callback - callback interface, called when a page is to be loaded
	 * @return paging_device_id_t - ID for the device, to use in #mapRegion
     */
	virtual paging_device_id_t registerPagingDevice( PagingDevice &callback ) = 0;

    /**
     * Unregisters a paging device previously registered via #registerPagingDevice
     * 
     * @param id - value returned by #registerPagingDevice
     */
    virtual void unregisterPagingDevice( paging_device_id_t id ) = 0;
    
    /**
     * Reserves a region of virtual memory mapped onto a specific paging device
     * 
     * @param firstIndex - first 24-bit index assigned, incremented for subsequent pages
     * @param nPages - number of pages to map sequentially
     * @param readOnly - flag to indicate that writes are always invalid
     * @param device - Paging device to map to, obtained through #registerPagingDevice
     */
    virtual linadr_t mapRegion( u32 firstIndex, size_t nPages, bool readOnly, 
            paging_device_id_t device ) = 0;
    
    
};

}} // namespaces
#endif
