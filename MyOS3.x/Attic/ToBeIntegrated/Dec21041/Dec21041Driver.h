/***************************************************************************
                          Dec21041Driver.h  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef DEC21041DRIVER_H
#define DEC21041DRIVER_H

#include "Core/PCIDriver.h"
#include "Dec21041Device.h"

namespace Dec21041 {

class Dec21041Driver : public I_PCIDriver
{
public:
	Dec21041Driver() INITSECTION;
	bool initialize() INITSECTION;
	void deinitialize();
private:
	/// I_PCIDriver
	virtual I_PCIDevice* handleDevice( I_UnhandledPCIDevice &dev );
	virtual const char* getDriverName() const { return "Dec21041"; }

	// for now, max. 1 device
	Dec21041Device device;
};

};	// namespace

#endif
