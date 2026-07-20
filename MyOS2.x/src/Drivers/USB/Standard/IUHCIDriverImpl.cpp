#include "Drivers/USB/Standard/UHCIDriver.h"
#include "Drivers/PCI/IPCISupport.h"
#include "Drivers/PCI/IPCIDevice.h"
#include "Devices/USB/IUSBRootHubImpl.h"

namespace MyOS { namespace Drivers { namespace USB { namespace Standard {

using namespace Drivers::PCI;
using Devices::USB::IUSBRootHubImpl;

// CDriverBase, virtual 
myos_result_t 
UHCIDriver::detectDevices( ) // INITSECTION
{
    if (iDriverManager->getPCISupport().queryDevices( *this, PCI_CLASS_SERIAL_USB )) {
		return E_MYOS_SUCCESS;
	}
	return E_MYOS_E_FAIL;
}

// CPCIDriver, virtual 
bool 
UHCIDriver::handleDevice( IPCIDevice &dev )
{
	// for now, only support Via VT82C686A
	const PCIDeviceInfo_t& info = dev.getInfo();
	if (info.id.vendor == PCI_VENDOR_ID_VIA && info.id.device == PCI_DEVICE_ID_VIA_82C586_2) {       
        // MMIO fails (no base set), UHCI 1.1 says registers are in normal I/O space
		iDriverManager->getPCISupport().enableDevice( dev, PCI_USE_PORTIO | PCI_MASTER | PCI_ENABLE_MWI );
        IUSBRootHubImpl* hub = new IUSBRootHubImpl( *this, dev );
        if (hub->init(*iDriverManager)) {
    		iDriverManager->registerDevice( Drivers::E_DEV_UHCI, *hub );
			impl1[0] = hub;
    		return true;
        }            
	} else {
		DPRINTK( "\nUHCIDriver: skipping device id=%x", (u32) info.id );
	}
	return false;
}

}}}}  // namespaces
