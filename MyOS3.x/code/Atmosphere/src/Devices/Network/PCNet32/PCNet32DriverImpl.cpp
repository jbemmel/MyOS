#include "PCNet32Driver.h"
#include "../../PCI/IPCISupport.h"
#include "../../PCI/IPCIDevice.h"
#include "PCNet32Device.h"
#include "new.h"

namespace MyOS { namespace Drivers { namespace Network { namespace PCNet32 {

using namespace Drivers::PCI;
using Devices::Network::PCNet32::PCNet32Device;

// CDriverBase, virtual 
myos_result_t 
PCNet32Driver::detectDevices( ) // INITSECTION
{
    if (iDriverManager->getPCISupport().queryDevices( *this, PCI_CLASS_NETWORK_ETHERNET )) {
		return E_MYOS_SUCCESS;
	}
	return E_MYOS_E_FAIL;
}

// CPCIDriver, virtual 
bool 
PCNet32Driver::handleDevice( IPCIDevice &dev )
{
	const PCIDeviceInfo_t& info = dev.getInfo();
	if (info.id.vendor == PCI_VENDOR_ID_AMD && info.id.device == PCI_DEVICE_ID_AMD_LANCE) {       

	    void *mem = allocate( sizeof(PCNet32Device) ); 
		PCNet32Device* pcnet32 = new (mem) PCNet32Device( *this, dev );
		if (pcnet32->initialize( iDriverManager->getPCISupport() )) {
    		iDriverManager->registerDevice( Drivers::E_DEV_ETH, *pcnet32 );
			impl1[0] = pcnet32;
    		return true;				
		} else {
		    deallocate( pcnet32, sizeof(PCNet32Device) );
		}
	} else {
		DPRINTK( "\nPCNet32Driver: skipping device id=%x", (u32) info.id );
	}
	return false;
}

}}}}  // namespaces
