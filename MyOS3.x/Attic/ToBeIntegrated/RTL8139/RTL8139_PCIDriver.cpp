/***************************************************************************
 RTL8139_PCIDriver.cpp  -  description
 -------------------
 begin                : Mon Aug 20 2001
 copyright            : (C) 2001 by jeroen
 email                : jeroen@thebem
 ***************************************************************************/
#include "RTL8139_PCIDriver.h"

#include "RTL8139_CIMPL.h"
#include "Devices/Network/RTL8139/RTL8139_PCIDevice.h"
#include "Devices/Network/RTL8139/RTL8139_Ethernet.h"
#include "../../PCI/pci_ids.h"

#include "../../PCI/IPCISupport.h"

// debugging
#include "debug.h"

namespace RTL8139
{

bool C_RTL8139_PCIDriver::initialize()
{
    RTL8139C& component = GETCOMPONENT(RTL8139C );

    // register yourself with PCI support
    if (component.pcisupport()->registerDriver( *this, PCI_CLASS_NETWORK_ETHERNET ))
    {
        CHECKPOINT( 'R' );
        return true;
    }CHECKPOINT( '\\' );
    return false;
}

void C_RTL8139_PCIDriver::deinitialize()
{
    RTL8139C& component = GETCOMPONENT(RTL8139C );

    // unregister yourself with PCI support
    component.pcisupport()->unregisterDriver( *this);
    device.deinitialize();
}

I_PCIDevice*C_RTL8139_PCIDriver::handleDevice(I_UnhandledPCIDevice &dev)
{
    const PCIDEVICEINFO &info = dev.getDeviceInfo();
    if ((info.id.vendor==PCI_VENDOR_ID_REALTEK)&& (info.id.device
            ==PCI_DEVICE_ID_REALTEK_8139)) // && more
    {
        // normally create a new device, now use 1 device
        CHECKPOINT( 'L' );
        // new (&device) C_RTL8139_PCIDevice();
        if (device.initialize(dev ))
        {
            return &device;
        }
    }
    return 0;
}

const char*C_RTL8139_PCIDriver::getDriverName() const
{
    return "rtl8139";
}

bool C_RTL8139_PCIDriver::takeOver(C_RTL8139_PCIDriver &from)
{
    return device.takeOver(from.device);
}

}
; // namespace
