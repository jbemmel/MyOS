/***************************************************************************
                          C_3c90xcDevice.h  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef C_3c90xcDevice_H
#define C_3c90xcDevice_H

#include "Devices/PCI/IPCIDevice.h"
#include "Devices/IOPort.h"
#include "3c90xcEthernet.h"

namespace _3c90xc {

class C_3c90xcDevice : public IPCIDevice
{
public:
	C_3c90xcDevice( I_PCIDriver &driver );
	bool initialize( I_UnhandledPCIDevice& dev ) INITSECTION;
	void deinitialize();

	// I_PCIDevice
	virtual bool open( /* params? */ );
	virtual bool close( /* params? */ );
	virtual bool suspend();
	virtual bool resume();
	virtual I_PCIDriver& getDriver() const { return driver; }
	virtual I_UnhandledPCIDevice& getDevice() const { return *device; }
	virtual I_Interface& getFunctionality() { return ethernet; }

	virtual s32 list( PacketWriter& buf, u32 closeTag /*=true*/ ) const;
	virtual s32 get( const char* callarguments, PacketWriter& buf );
	virtual const char* getInstanceID( char assign /*='\0'*/ );

private:
	/// for now, 1 instance
	static C_3c90xcDevice* instance;

	const char instanceID[6];
	I_PCIDriver &driver;
	I_UnhandledPCIDevice* device;
	C_3c90xcEthernet ethernet;

	/// Could have one for each instance
	static void IRQHandler( u32 irq );	
};

};	// namespace

#endif
