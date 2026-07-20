/***************************************************************************
                          Dec21041Device.h  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef Dec21041DEVICE_H
#define Dec21041DEVICE_H

#include "Core/PCIDevice.h"
#include "Dec21041Ethernet.h"

namespace Dec21041 {

class Dec21041Device : public I_PCIDevice
{
public:
	Dec21041Device( I_PCIDriver &driver );
	bool initialize( I_UnhandledPCIDevice& dev ) INITSECTION;
	void deinitialize();

	// I_PCIDevice
	virtual bool open( /* params? */ );
	virtual bool close( /* params? */ );
	virtual bool suspend();
	virtual bool resume();
	virtual I_PCIDriver& getDriver() const { return driver; }
	virtual I_UnhandledPCIDevice& getDevice() const;
	virtual I_Interface& getFunctionality() { return ethernet; }

	virtual s32 list( PacketWriter& buf, u32 closeTag /*=true*/ ) const;
	virtual s32 get( const char* callarguments, PacketWriter& buf );
	virtual const char* getInstanceID( char assign /*='\0'*/ );

private:
	/// for now, 1 instance
	static Dec21041Device* instance;

	const char instanceID[6];
	I_PCIDriver &driver;
	I_UnhandledPCIDevice *device;
	Dec21041Ethernet ethernet;

	/// Could have one for each instance
	static void IRQHandler( u32 irq );	
};

};	// namespace

#endif
