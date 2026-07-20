/***************************************************************************
                          Dec21041Driver.cpp  -  description
                             -------------------
    begin                : Thu 18/10/2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem

	@see ftp://tulip.sourceforge.net/pub/tulip/specs/21041/
 ***************************************************************************/
#include "Dec21041CIMPL.h"
#include "Dec21041Driver.h"
#include "pci_ids.h"

namespace Dec21041 {

Dec21041Driver::Dec21041Driver()
	: device( *this )
{

}

bool
Dec21041Driver::initialize()
{
	DEC21041& component = GETCOMPONENT( DEC21041 );

	// register yourself with PCI support			
	return (component.pcisupport()->registerDriver( *this, PCI_CLASS_NETWORK_ETHERNET ));
}

void
Dec21041Driver::deinitialize()
{
	DEC21041& component = GETCOMPONENT( DEC21041 );

	// unregister yourself with PCI support
	component.pcisupport()->unregisterDriver( *this );
	device.deinitialize();	
}

I_PCIDevice*
Dec21041Driver::handleDevice( I_UnhandledPCIDevice &dev )
{
	const PCIDEVICEINFO &info = dev.getDeviceInfo();
	I_Display &disp = *GETCOMPONENT( DEC21041 ).display();

	disp 	<< "\nDec21041Driver::handleDevice vendor=" << (u16) info.id.vendor
				<< " device=" << (u16) info.id.device;

	if ((info.id.vendor==PCI_VENDOR_ID_DEC)
	&& (info.id.device==PCI_DEVICE_ID_DEC_TULIP_PLUS))	// && more
	{	
		// normally create a new device, now use 1 device
		if (device.initialize( dev )) {
			return &device;
		}
		else disp << " failed to initialize";
	}
	else disp << " not recognised";
	return 0;	
}

};  // namespace
