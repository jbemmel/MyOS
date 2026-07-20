/***************************************************************************
                          RTL8139_PCIDevice.cpp  -  description
                             -------------------
    begin                : Sun Aug 19 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#include "RTL8139_PCIDevice.h"
#include "RTL8139_CIMPL.h"

using namespace Debug;

namespace RTL8139 {

C_RTL8139_PCIDevice* C_RTL8139_PCIDevice::instance;

C_RTL8139_PCIDevice::C_RTL8139_PCIDevice()
	: deviceID( "eth0\0" ), ethernet(*this)
{
	/// for now, max. one device
	instance = this;
}

I_UnhandledPCIDevice&
C_RTL8139_PCIDevice::getDevice() const
{
    // hope this has been set and is (still) valid...
    return *deviceInfo;
}

I_PCIDriver&
C_RTL8139_PCIDevice::getDriver() const
{
	return GETCOMPONENT( RTL8139C ).pcidriver;
}

bool
C_RTL8139_PCIDevice::initialize( I_UnhandledPCIDevice& dev )  // INITSECTION
{
    // keep this
    deviceInfo = &dev;

    const PCIDEVICEINFO &info = dev.getDeviceInfo();

    // cache the info, to be able to return it in getInfo (hope it stays valid...)
    // Normally, this would be a reference member set in the constructor...

    RTL8139C& component = GETCOMPONENT( RTL8139C );

    // bring the chip out of low power mode, allocate resources, set busmaster		
    if (!component.pcisupport()->enableDevice( *this, handleIRQ,
					PCI_USE_MMIO | PCI_MASTER | PCI_POWER_D0 )) {
        PRINTM( "\nPCISupport could not enable device!" );
        return false;
    }

     // could read chip version here, to check subtype
	if (info.MMIOstart) {
		// initialize ethernet stuff
		if (ethernet.initialize( info.MMIOstart )) {
			return true;		// dont register ethernet interface, it must be done through /dev/ethx
		}
	}
	
	PRINTM( "\nRTL8139C failed to initialize" );
	component.pcisupport()->releaseDevice( *this );
	return false;
}

bool
C_RTL8139_PCIDevice::deinitialize()
{
	RTL8139C& component = GETCOMPONENT( RTL8139C );
	component.repository->unregisterInterface( ethernet );
	ethernet.deinitialize();
	component.pcisupport()->releaseDevice( *this );
	return true;
}

const char*
C_RTL8139_PCIDevice::getInstanceID( char assign /* =0 */ )
{
	if (assign) (char&) deviceID[3] = assign;
	return deviceID;
}

// Start the hardware at open or resume
bool
C_RTL8139_PCIDevice::open()
{	
    return ethernet.resume();
}

bool
C_RTL8139_PCIDevice::close( /* params? */ )
{
	ethernet.close();	
	return true;
}

s32
C_RTL8139_PCIDevice::list( PacketWriter& buf, u32 closeTag ) const
{
	deviceInfo->list( buf, false );
	buf << ">\n<ADDRESS domain=\"ethernet\" value=\"" << ethernet.getLocalAddress() << "\"/>";
	if (closeTag) buf << "\n</DEV>";			
	return buf.getBytesWritten();
}

/**
 * 	copy of function from I_UnhandledPCIDevice, should be in I_PCIDevice
 */
s32
C_RTL8139_PCIDevice::get( const char* callarguments, PacketWriter& buf )
{
#ifdef STATISTICS
	if (strcmp(callarguments, "statistics")==0) {
		return ethernet.statistics( buf );
	} else
#endif
	if (strcmp(callarguments, "list") == 0) {
		return list( buf, true );
	} else return I_Interface::get( callarguments, buf );
}

bool
C_RTL8139_PCIDevice::takeOver( C_RTL8139_PCIDevice &from )
{
	return ethernet.takeOver( from.ethernet );
}

void
C_RTL8139_PCIDevice::handleIRQ( u32 irq )
{
	instance->ethernet.handleIRQ();
}

};  // namespace
