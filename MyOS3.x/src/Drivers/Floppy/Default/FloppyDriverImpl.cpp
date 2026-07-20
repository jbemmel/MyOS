#include "Drivers/Floppy/Default/FloppyDriver.h"
#include "Devices/Floppy/IFloppyImpl.h"

namespace MyOS { namespace Drivers { namespace Floppy { namespace Default {

using Devices::Floppy::IFloppyImpl;
   
// virtual 
myos_result_t 
FloppyDriver::detectDevices( ) // INITSECTION
{
    // for now: always create
    IFloppyImpl* floppy = new IFloppyImpl( *this );
    if (floppy->init()) {
        iDriverManager->registerDevice( Drivers::E_DEV_FDD, *floppy );
		impl1[0] = floppy;
    	return E_MYOS_SUCCESS;
    } else {
    	// XX Should call delete on FloppyDriver ??
    	delete floppy;
    }
    return E_MYOS_E_FAIL;       
}

}}}} // namespace
