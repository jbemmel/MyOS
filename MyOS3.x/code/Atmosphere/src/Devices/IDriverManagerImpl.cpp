#include "IDriverManagerImpl.h"
#include "CDriverBase.h"
#include "IDevice.h"
#include "DriverManagerComponent.h"

#include "Buffering/CBufOutputStream.h"

// TODO: cleanup? used for usDelay
#include "TimerFacility/TimeUtil.h"
using MyOS::Timer::TimeUtil;

#include "debug.h"

namespace MyOS { namespace Drivers {

IDriverManagerImpl::IDriverManagerImpl( MyOS::Core::IComponent& c )
: IDriverManager( c, VERSION(1,0) )
{

}

bool 
IDriverManagerImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
    // Need custom lookup of root
    IDirectory &root = context.lookup( myos_name_t(".") ).castToExcept<IDirectory>();
    devices = root.createSubdirectory( myos_name_t("dev") );
    return pciSupport.initialize(context);
    	
    // TODO: Could register display and other boot devices here
}

void 
IDriverManagerImpl::deinit( IContext& context )
{
    pciSupport.deinitialize();
    if (devices) {
        devices->destroy();
        devices = 0;
    }
}


// virtual 
myos_result_t
IDriverManagerImpl::listInstalledDrivers( IO::OStream& out ) const {    
    return devices->list( myos_name_t(""), 1, out );
}

// virtual 
bool 
IDriverManagerImpl::addDriver( CDriverBase& driver )
{
	// TODO: Add to some list
	driver.detectDevices();
	return true;	
}

// virtual 
bool 
IDriverManagerImpl::removeDriver( CDriverBase& driver )
{
   // TODO: remove from some list (directory?)
   return true;    
}

// virtual 
int 
IDriverManagerImpl::registerDevice( myos_device_class ofClass, Devices::IDevice& dev )
{
    // create entry under /dev/ e.g. /dev/eth0
    const char *devFormat = "xxx%u";
    switch(ofClass)
    {
		case E_DEV_ETH:			devFormat = "eth%u"; break;
		case E_DEV_FDD:       	devFormat = "fdd%u"; break;
		case E_DEV_PCI:       	devFormat = "pci%u"; break;
		case E_DEV_KEYBOARD:  	devFormat = "kbd%u"; break;
		case E_DEV_UHCI:		devFormat = "uhc%u"; break;
    }      

    u8 buf[8];
    IO::CBufOutputStream bw( buf, sizeof(buf) );
    for (unsigned c=0; c<=99; ++c) {
        s32 len = bw.printf( devFormat, c );
        myos_name_t name( dev.setId( (char*) buf ), len );
        if ( E_MYOS_SUCCESS == devices->addNode( name, dev )) {
            DPRINTK( " Added device %n", &name );
            return c;  
        }
    }
    return -1;   // invalid ID: too many instances
}

// virtual 
void 
IDriverManagerImpl::usDelay( u32 us_count ) const
{
    TimeUtil::delay<Timer::us>( us_count );
}

}}  // namespaces
