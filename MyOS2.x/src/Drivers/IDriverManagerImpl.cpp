#include "IDriverManagerImpl.h"
#include "CDriverBase.h"
#include "Devices/IDevice.h"
#include "Drivers/DriverManagerComponent.h"

// TODO: cleanup? used for usDelay
#include "Timer/TimeUtil.h"
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
    IDirectory* root = 0;
    if (context.lookup( myos_name_t("."), (IInterface*&) root ) == E_MYOS_SUCCESS) {  
        devices = root->createSubdirectory( myos_name_t("dev") );
    	return pciSupport.initialize(context);
    	
    	// TODO: Could register display and other boot devices here
    }
    return false;        
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
    #define DEV(a,b,c) (a<<0|b<<8|c<<16)
    #define SETID(e,id) (e=(e&0x00FFFFFF)|(id<<24))
    int entry;
    switch(ofClass)
    {
		case E_DEV_ETH:			entry = DEV('e','t','h'); break;
		case E_DEV_FDD:       	entry = DEV('f','d','d'); break;
		case E_DEV_PCI:       	entry = DEV('p','c','i'); break;
		case E_DEV_KEYBOARD:  	entry = DEV('k','b','d'); break;
		case E_DEV_UHCI:		entry = DEV('u','h','c'); break;
      
		default:              entry = DEV('x','x','x');
    }      

    for (char c='0'; c<='9'; ++c) {
        SETID( entry, c );
        myos_name_t name( dev.setId( entry ), 4 );
        if ( E_MYOS_SUCCESS == devices->addNode( name, dev )) {
            DPRINTK( "\nAdded device %n", &name );
            return (c-'0');  
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
