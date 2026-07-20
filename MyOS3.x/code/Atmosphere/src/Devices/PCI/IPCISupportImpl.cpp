/*****************************************************************************
                          PCISupportIMPL.cpp  -  description
                             -------------------
    begin                : Tue Aug 21 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
***************************************************************************/
#include "IPCISupportImpl.h"
#include "Repository/IDirectory.h"
#include "IContext.h"
#include "CPCIDriver.h"
#include "pci_ids.h"
#include "../DriverManagerComponent.h"

#include "debug.h"

namespace MyOS {
namespace Drivers {
namespace PCI {

using namespace InterruptHandling;

IPCISupportImpl::IPCISupportImpl( )     // INITSECTION
{

}

bool
IPCISupportImpl::initialize( IContext& context )    // INITSECTION
{
//    IDirectory* dev = 0;
//    context.lookup( myos_name_t("/dev"), (IInterface*&) dev );
//	devices = dev->createSubdirectory( myos_name_t("pci") );
//    if (!devices) return false;
	scanPCI();
	return true;
}

void
IPCISupportImpl::deinitialize()
{
	DRINGLIST< IPCIDeviceImpl >::Iterator i( unclaimed_devices );

	while( IPCIDeviceImpl *d = i )
	{
		++i;
		delete d;
	}

    // hope devices->destroy() will kill the rest...
    // if (devices) { devices->destroy(); devices=0; }
}

void
IPCISupportImpl::scanPCI()
{
	PCI_RESOURCE_INFO info;		// should be per bridge...

	// LOG( DEVICE_SUPPORT, "Scanning PCI busses..." );

	for (u8 bus=0; bus<8; ++bus)
	{
		for (u32 slot = 0; slot < 256; slot += 8 )
		{
			PCIDeviceHandle id( bus, slot );
			if (probeDevice(id, info))
			{
				PCI_HEADER_TYPE hdr_type = PCICONFIG_FIXED.header_type.read( id );
				if (hdr_type.multi_function) {
					for ( u32 func=1; func<8; ++func)
					{
						PCIDeviceHandle id2( bus, slot+func );
						probeDevice( id2, info );
					}
				}
			}
		}
	}

	/* Testing under BOCHS => setPowerState fails (of course)
	PCIDeviceHandle id(0,0);
	PCIDevice *dev = new PCIDevice( PCI_VENDOR_ID_REALTEK, id );
	dev->info.id.device = PCI_DEVICE_ID_REALTEK_8139;
	dev->info.classID = PCI_CLASS_NETWORK_ETHERNET;
	unclaimed_devices.insert( dev->ringnode );
	devices->addNode( myos_name_t(dev->info.systemID), *dev );
   */

   DPRINTK( "\nScanPCI: found %d devices", unclaimed_devices.size() );
}

s32
IPCISupportImpl::probeDevice( PCIDeviceHandle id, PCI_RESOURCE_INFO &info )
{
	E_PCI_ID vendor = PCICONFIG_FIXED.vendor.read( id );
	if (!(vendor == PCI_VENDOR_ID_UNKNOWN || vendor == PCI_VENDOR_ID_INVALID ))
	{
		// DPRINTK( "\nFound PCI device: %x vendor %x", (u32) id , (u32) vendor );
		IPCIDeviceImpl *dev = new IPCIDeviceImpl( vendor, id );

        // TODO: Apply quirk fixes?
//        if (vendor == PCI_VENDOR_ID_VIA && dev->getDeviceID() == PCI_DEVICE_ID_VIA_82C686) {
//            // The need for this fix is questioned in Linux source comment
//            u8 irq = PCICONFIG_FIXED.type0.sci_irq_line.read( id ) & 0x0F;
//            if (irq && irq!=2) {
//                DPRINTK( "\nTODO: Fix VIA IRQ to %X", irq );
//            }
//        }

		// register with interface "IUnhandledPCIDevice" :)
        
		if (DriverManagerComponent::getInstance().impl1.registerDevice( E_DEV_PCI, *dev ) >= 0) {
			unclaimed_devices.insert( dev->ringnode );
		} else {
			DPRINTK( "Failed to add PCI device to directory" );
			delete dev;
			return false;
		}

		// Screen::cursor() << '@' << dev;

		// update limits
		info.include( dev->getResources() );
		// if (dev->getClass() == PCICLASS_BRIDGE_HOST)
		//{
		//	new (&hostBridge) PCIBridge( dev ); 	// could derive PCIBridge from PCIDevice
		//}
		return true;
	} else {
		return false;
	}
}


// virtual, called by a driver
bool 
IPCISupportImpl::queryDevices( CPCIDriver &driver, PCIClass_t pciclass ) 
{
    DRINGLIST< IPCIDeviceImpl >::Iterator i( unclaimed_devices );

    s32 count=0;
    while( IPCIDeviceImpl *d = i )
    {
        ++i;    // perhaps no longer valid after handleDevice()
        if (d->info.classID == pciclass) {
            if (driver.handleDevice( *d )) {
                unclaimed_devices.remove( d->ringnode );
                d->setDriver(driver);
                claimed_devices.insert( d->ringnode );
                ++count;
                CHECKPOINT('!');
            } else CHECKPOINT('*');
        } 
    }
    return count>0;
}

/*
bool
IPCISupportImpl::registerDriver( CPCIDriver &drv, u32 PCICLASS )
{
	// LOG( DEVICE_SUPPORT, "Register PCI driver %s", drv.getDriverName() );

	DRINGLIST< PCIDevice >::Iterator i( unclaimed_devices );

	s32 count=0;
	while( PCIDevice *d = i )
	{
		++i;    // perhaps no longer valid after handleDevice()

		if (d->info.classID == PCICLASS) {
			if (IPCIDevice *impl = drv.handleDevice( *d )) {
			    unclaimed_devices.remove( d->ringnode );
					d->setDriver(impl);
			    claimed_devices.insert( d->ringnode );
			    ++count;
            CHECKPOINT('!');
			} else CHECKPOINT('*');
		}
	}

	if (count>0) {
		// add driver to repository
		// LOG( DEVICE_SUPPORT, "..driver handles %u devices", count );
      PRINTF( "..driver handles %u devices", count );
		return drivers->addNode( myos_name_t(drv.getDriverName()), drv ) == E_MYOS_SUCCESS;
	} else {
		// LOG( DEVICE_SUPPORT, "..no devices found" );
		return false;
	}
}

void
IPCISupportImpl::unregisterDriver( IPCIDriver &drv )
{
	// LOG( DEVICE_SUPPORT, "PCI:unregister driver %s", drv.getDriverName() );
	drivers->removeNode( myos_name_t(drv.getDriverName()) );
}
*/

// virtual
bool
IPCISupportImpl::enableDevice( IPCIDevice &device, u32 flags )
{
	// direct cast to PCIDevice, interface users *must* not copy info!
	IPCIDeviceImpl& d = *(IPCIDeviceImpl*) &device;
	return d.enable( *DriverManagerComponent::getInstance().iVirtualMemory, flags );
}

// virtual
bool
IPCISupportImpl::enableIRQ( IPCIDevice &device, IInterruptContext& h ) const
{
    IPCIDeviceImpl& d = *(IPCIDeviceImpl*) &device;  
	const PCIDeviceInfo_t& devinfo = d.getDeviceInfo();

   if ( devinfo.assignedIRQ >0 && devinfo.assignedIRQ <=15) {
       	// set the interrupt handler, will run with ints enabled (!) Can cause problems
        DPRINTK( "\nPCISupport enable irq=%X", devinfo.assignedIRQ );
   	    DriverManagerComponent::getInstance().iInterruptHandling->setIRQHandler(
         (E_IRQ) (devinfo.assignedIRQ + InterruptHandling::E_IRQ0),
         h, E_NONE /*E_ENABLE_INTS*/ );
      return true;
   } else {
      DPRINTK( "\nPCISupport irq #INVALID#=%X", devinfo.assignedIRQ );
      return false;
   }
}

/// Return a device handled by the driver
void
IPCISupportImpl::releaseDevice( IPCIDevice &device )
{
/*  
	/// Remove the handled device if it exists
	if (devices->removeNode( myos_name_t(device.getInstanceID()) )) {
			PCIDevice *d = (PCIDevice*) &device.getDevice();
			claimed_devices.remove( d->ringnode );
			d->releaseResources( *DeviceSupportComponent::getInstance().memory() );

			const PCIDeviceInfo_t& devinfo = d->getDeviceInfo();
	      DeviceSupportComponent::getInstance().irqsupport()->removeIRQHandler(
            (E_IRQVector) (devinfo.assignedIRQ + INTProcessing::E_IRQ0), 0 );

			// re-install the unhandled device
			unclaimed_devices.insert( d->ringnode );
			devices->addNode( myos_name_t(devinfo.systemID), device.getDevice() );
	} // else REMARK( "Failed to release PCI device!" );	this occurs when dev.initialization failed
*/   
}

/*
IUnhandledPCIDevice*
IPCISupportImpl::getUnhandledDevice( u32 id ) const
{
	DRINGLIST< IPCIDeviceImpl >::Iterator it( unclaimed_devices );

	while (IPCIDeviceImpl *d = it)
	{
        if ((id--)==0) {
            return d;
        }
		++it;
	}
	return 0;
}
*/

}}}	// namespace
