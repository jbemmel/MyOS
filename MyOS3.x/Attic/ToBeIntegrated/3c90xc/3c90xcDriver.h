/***************************************************************************
                          C_3c90xcDriver.h  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef C_3c90xcDRIVER_H
#define C_3c90xcDRIVER_H

#include "Drivers/PCI/CPCIDriver.h"
#include "Devices/Network/3c90xc/3c90xcDevice.h"

namespace _3c90xc {

class C_3c90xcDriver : public CPCIDriver
{
public:
	C_3c90xcDriver() INITSECTION;
	bool initialize() INITSECTION;
	void deinitialize();

private:
	/// CPCIDriver
    virtual bool handleDevice( IPCIDevice &dev ) = 0;
	// virtual const char* getDriverName() const { return "3c90xc"; }

	// for now, max. 1 device
	C_3c90xcDevice device;
};

};	// namespace

#endif
