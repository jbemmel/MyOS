#include "IISO9660FSImpl.h"

#include "ISO9660Reader.h"

namespace MyOS { namespace FS { namespace ISO9660 {

IISO9660FSImpl::IISO9660FSImpl( MyOS::Core::IComponent& c )
: IISO9660FS( c, VERSION(1,0) )
{

}

bool 
IISO9660FSImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	return true;
}

void 
IISO9660FSImpl::deinit( IContext& context )
{

}

// virtual 
IMountedFS& 
IISO9660FSImpl::mount( Devices::IBlockDevice& toDevice ) throw (FSException) 
{
   return * new ISO9660Reader( toDevice ); 
}


}
}}  // namespaces
