#include "myosconfig.h"
#ifdef CONFIG_IDE

#include "IIDEDeviceImpl.h"
#include "Drivers/IDE/IDEDriverComponent.h"

namespace MyOS { namespace Devices { namespace IDE {

IIDEDeviceImpl::IIDEDeviceImpl( IDEController& c, u8 m )
: IIDEDevice( Drivers::IDE::IDEDriverComponent::getInstance(), VERSION(1,0) )
, controller(c), masterSlave(m)
{

}

bool 
IIDEDeviceImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // TODO: Remove this whole method ??
	return true;
}

void 
IIDEDeviceImpl::deinit( IContext& context )
{

}

}}}  // namespaces

#endif
