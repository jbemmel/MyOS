/***************************************************************************
                          Dec21041Device.cpp  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

	This code was mostly inspired by Dec.c, the linux driver for this AMD card
 ***************************************************************************/
#include "Dec21041CIMPL.h"
#include "Dec21041Device.h"

// debug
#include "Screen.h"

namespace Dec21041 {

/// Watch it! This will end up in bss if not careful ! Initialize !
Dec21041Device* Dec21041Device::instance = (Dec21041Device*) 0xcdcdcdcd;

Dec21041Device::Dec21041Device( I_PCIDriver &drv )
	:  instanceID( "eth0\0" ), driver(drv), device(0), ethernet(*this)
{
	/// for now, a single instance
	instance = this;
}

I_UnhandledPCIDevice&
Dec21041Device::getDevice() const
{
	return *device;
}

bool
Dec21041Device::initialize( I_UnhandledPCIDevice& dev ) // INITSECTION
{
#ifdef DEC21041_MMIO		// defined in Dec21041Ethernet.h
	#define __MODE__ PCI_USE_MMIO
#else
	#define __MODE__ PCI_USE_PORTIO
#endif

	device = &dev;
	if (GETCOMPONENT( DEC21041 ).pcisupport()->enableDevice(
			*this, IRQHandler, __MODE__ | PCI_MASTER )) {
		GETCOMPONENT( DEC21041 ).display()->print( "\nPCI succeeded, now ethernet..." );
		return ethernet.initialize( dev.getDeviceInfo() );
	}
	return false;
}

void
Dec21041Device::deinitialize()
{
	ethernet.deinitialize();
	GETCOMPONENT( DEC21041 ).pcisupport()->releaseDevice( *this );
}

void	// static
Dec21041Device::IRQHandler( u32 irq )
{
	instance->ethernet.handleIRQ();
}

bool
Dec21041Device::open()
{
	/// irq already requested, device enabled
	return ethernet.open();
}

bool
Dec21041Device::close( /* params? */ )
{
	return false;
}

bool
Dec21041Device::suspend()
{
	return false;
}

bool
Dec21041Device::resume()
{
	return false;
}

s32
Dec21041Device::list( PacketWriter& buf, u32 closeTag ) const
{
	device->list( buf, false );
	buf << ">\n<ADDRESS domain=\"ethernet\" value=\"" << ethernet.getLocalAddress() << "\" />";
	if (closeTag) buf << "\n</DEV>";			
	return buf.getBytesWritten();
}

s32
Dec21041Device::get( const char* callarguments, PacketWriter& buf )
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

const char*
Dec21041Device::getInstanceID( char assign /*='\0'*/ )
{
	if (assign) (char&) instanceID[3] = assign;
	return instanceID;
}


};	// namespace

