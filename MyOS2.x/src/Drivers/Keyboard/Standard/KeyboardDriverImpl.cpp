#include "Drivers/Keyboard/Standard/KeyboardDriver.h"
#include "Devices/Keyboard/IKeyboardImpl.h"

namespace MyOS { namespace Drivers { namespace Keyboard { namespace Standard {

using namespace InterruptHandling;
using namespace Devices::Keyboard;

/************
 * KeyboardDriver routine (!)
 ************/
// virtual 
myos_result_t 
KeyboardDriver::detectDevices( ) // INITSECTION
{
	// Note: static
	static IKeyboardImpl dev( *this );
    if (dev.init()) {    	
        iDriverManager->registerDevice( Drivers::E_DEV_KEYBOARD, dev );
        impl1[0] = &dev;
        return E_MYOS_SUCCESS;
    } else {
		return E_MYOS_E_FAIL;
    }
}

}}}}	// namespaces
