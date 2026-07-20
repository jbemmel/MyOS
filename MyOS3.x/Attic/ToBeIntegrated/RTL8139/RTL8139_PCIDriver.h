/***************************************************************************
                          RTL8139_PCIDriver.h  -  description
                             -------------------
    begin                : Mon Aug 20 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#ifndef RTL8139PCIDRIVER_H
#define RTL8139PCIDRIVER_H

#include "Core/PCIDriver.h"
#include "RTL8139_PCIDevice.h"

class I_UnhandledPCIDevice;

namespace RTL8139 {

class C_RTL8139_PCIDriver : public I_PCIDriver
{
public:
	// called from component
	bool initialize(  ) INITSECTION;
	bool takeOver( C_RTL8139_PCIDriver &from );
	void deinitialize();
	
private:
    virtual I_PCIDevice* handleDevice( I_UnhandledPCIDevice &dev );
    // virtual I_PCIDevice* getDevice( ID id ) const;
    virtual const char* getDriverName() const;

    // perhaps normally, the driver would have an IRQ handler
    // that transfers control to the right device

    // for now, supports 1 device
    C_RTL8139_PCIDevice device;
};

};  // namespace

#endif

