/***************************************************************************
                          C_3c90xcDriver.cpp  -  description
                             -------------------
    begin                : Thu 18/10/2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#include "3c90xcCIMPL.h"
#include "3c90xcDriver.h"
#include "../../PCI/pci_ids.h"

namespace _3c90xc {

C_3c90xcDriver::C_3c90xcDriver()
	: device( *this )
{

}

bool
C_3c90xcDriver::initialize()
{
    C_3c90xc& component = GETCOMPONENT( C_3c90xc );

    // register yourself with PCI support
    if (component.pcisupport()->registerDriver( *this, PCI_CLASS_NETWORK_ETHERNET )) {
        return true;
    }
    return false;
}

void
C_3c90xcDriver::deinitialize()
{
	C_3c90xc& component = GETCOMPONENT( C_3c90xc );

	// unregister yourself with PCI support
	component.pcisupport()->unregisterDriver( *this );
	device.deinitialize();	
}

I_PCIDevice*
C_3c90xcDriver::handleDevice( IPCIDevice &dev )
{
	const PCIDEVICEINFO &info = dev.getDeviceInfo();

#ifndef COMPONENTCOMPILATION
	Screen::cursor() << "\n3c90xc::handleDevice vendor="
		<< (u16) info.id.vendor << " device=" << (u16) info.id.device;
#endif

	/// 3c90xc.pdf p.70
	if (info.id.vendor==PCI_VENDOR_ID_3COM) {
		switch (info.id.device) {	
			case PCI_DEVICE_ID_3COM_3C900TPO:
			case PCI_DEVICE_ID_3COM_3C900COMBO:
			case PCI_DEVICE_ID_3COM_3C905TX:
			case PCI_DEVICE_ID_3COM_3C905T4:
			case PCI_DEVICE_ID_3COM_3C905B_TX:

			/// These can be found in the linux driver
			case 0x9200:	// CH_3C905C     * in PC at work
			case 0x9058:  // CH_3C905B_2
			case 0x905A: 	// CH_3C905B_FX

	#ifndef COMPONENTCOMPILATION	
				Screen::cursor() << "\nFound a known chip: "<< info.id.device;
	#endif
				break;
			default:
				return false;
		}
		// normally create a new device, now use 1 device
		if (device.initialize( dev )) {
			return &device;
		}
#ifndef COMPONENTCOMPILATION
			else Screen::cursor() << " failed to initialize";
#endif
	}
	return 0;	
}

};  // namespace
