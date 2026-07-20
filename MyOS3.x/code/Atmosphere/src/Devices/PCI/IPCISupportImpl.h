/***************************************************************************
                          PCISupportIMPL.h  -  description
                             -------------------
    begin                : Tue Aug 21 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#ifndef PCISUPPORTIMPL_H
#define PCISUPPORTIMPL_H

#include "IPCISupport.h"

#include "defs.h"
#include "types.h"
#include "DRINGLIST.h"
#include "IPCIDeviceImpl.h"

namespace MyOS {

namespace Context { class IDirectory; }   
using Context::IDirectory;
   
namespace Drivers {
    
class IDriverManagerImpl;    
    
namespace PCI {

class PCIDeviceHandle;
struct PCI_RESOURCE_INFO;
class CPCIDriver;

class IPCISupportImpl : public IPCISupport
{
private:
    //Query for devices. The driver will be given devices to handle
    virtual bool queryDevices( CPCIDriver &driver, PCIClass_t pciclass );

    /// Enable the PCI device, allocates resources, updates info
    virtual bool enableDevice( IPCIDevice &device, u32 flags );
    virtual bool enableIRQ( IPCIDevice &device, IInterruptContext& h ) const;

    /// Return a device handled by the driver
    virtual void releaseDevice( IPCIDevice &device );

    // virtual IUnhandledPCIDevice* getUnhandledDevice( u32 i ) const;

private:    // implementation
	friend class Drivers::IDriverManagerImpl;
	IPCISupportImpl( ) INITSECTION;

	bool initialize( IContext& context ) INITSECTION;
	void deinitialize();

	void scanPCI() INITSECTION;
	s32 probeDevice( PCIDeviceHandle id, PCI_RESOURCE_INFO& info ) INITSECTION;

	/// iterator needs access in const method -> mutable
    mutable DRINGLIST< IPCIDeviceImpl > unclaimed_devices, claimed_devices;
};

}}}	// namespace

#endif
