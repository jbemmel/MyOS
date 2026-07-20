/***************************************************************************
                          C_3c90xcDevice.cpp  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

	This code was mostly inspired by 3c90xc.c, the linux driver for this AMD card
 ***************************************************************************/
#include "3c90xcCIMPL.h"
#include "3c90xcDevice.h"

// debug
#include "Screen.h"

namespace _3c90xc {

C_3c90xcDevice* C_3c90xcDevice::instance = (C_3c90xcDevice*) 0xcdcdcdcd;

C_3c90xcDevice::C_3c90xcDevice( I_PCIDriver &drv )
	: instanceID( "eth0\0" ), driver(drv), device( 0 ), ethernet(*this)
{
	/// for now, a single instance
	instance = this;
}

bool
C_3c90xcDevice::initialize( I_UnhandledPCIDevice& dev ) // INITSECTION
{
#ifdef _3c90xc_USE_MMIO
	#define __IO__ PCI_USE_MMIO
#else
	#define __IO__ PCI_USE_PORTIO
#endif

	device = &dev;
	/// Must enable device to get access to registers!
	if (GETCOMPONENT( C_3c90xc ).pcisupport()->enableDevice(
			*this, IRQHandler, __IO__ | PCI_MASTER | PCI_POWER_D0 | PCI_ENABLE_MWI )) {
		if (ethernet.initialize( dev.getDeviceInfo() )) {
    	return true;
		}
		GETCOMPONENT( C_3c90xc ).pcisupport()->releaseDevice( *this );
	}
	return false;
}

void
C_3c90xcDevice::deinitialize()
{

}

void	// static
C_3c90xcDevice::IRQHandler( u32 irq )
{
	instance->ethernet.handleIRQ();
}

bool
C_3c90xcDevice::open()
{
	/// request irq, enable device
	return ethernet.open();
}

bool
C_3c90xcDevice::close( /* params? */ )
{
	return false;
}

bool
C_3c90xcDevice::suspend()
{
	return false;
}

bool
C_3c90xcDevice::resume()
{
	return false;
}

s32
C_3c90xcDevice::list( PacketWriter& buf, u32 closeTag ) const
{
	return -1;
}

s32
C_3c90xcDevice::get( const char* callarguments, PacketWriter& buf )
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
C_3c90xcDevice::getInstanceID( char assign /*='\0'*/ )
{
	if (assign) (char&) instanceID[3] = assign;
	return instanceID;
}


};	// namespace
