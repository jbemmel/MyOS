#include "Drivers/Network/NE2000/NE2000Driver.h"
#include "Devices/Network/NE2000/DP8390d.h"
#include "MM/new.h"

namespace MyOS { namespace Drivers { namespace Network { namespace NE2000 {

using namespace Devices::Network::NE2000;

// virtual 
myos_result_t 
NE2000Driver::detectDevices() // INITSECTION;
{
	DP8390D* chip = new DP8390D( *this );
	if (chip->init()) {
		iDriverManager->registerDevice( Drivers::E_DEV_ETH, *chip );
		impl1[0] = chip;
		return E_MYOS_SUCCESS;
	}
	return E_MYOS_ERROR;	
}
	
}}}}
