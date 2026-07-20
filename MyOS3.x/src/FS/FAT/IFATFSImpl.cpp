#include "IFATFSImpl.h"
#include "FS/FAT/FATComponent.h"

#include "FATReader.h"
#include "MM/new.h"

namespace MyOS { namespace FS { namespace FAT {

IFATFSImpl::IFATFSImpl( MyOS::Core::IComponent& c )
: IFATFS( c, VERSION(1,0) )
{

}

bool 
IFATFSImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	return true;
}

void 
IFATFSImpl::deinit( IContext& context )
{

}

// virtual 
IMountedFS& 
IFATFSImpl::mount( Devices::IBlockDevice& toDevice ) throw (FSException) 
{ 
   DPRINTK( "\nFATFS::mount" );
   return * new FATReader( toDevice );
}

/*
// virtual 
myos_result_t
IFATFSImpl::enumerateFiles( IO::OStream& out ) const {
   return reader->getRoot().list( myos_name_t(""),0,out );
}
*/

}}}  // namespaces
