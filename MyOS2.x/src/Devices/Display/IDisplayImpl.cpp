#include "IDisplayImpl.h"

#include "IO/IStream.h"
#include "myosconfig.h" // builddate

namespace MyOS { namespace Devices { namespace Display {

IDisplayImpl::IDisplayImpl( MyOS::Core::IComponent& c )
: IDisplay( c, VERSION(1,0) )
, theScreenBuffer( (void*) 0xB8000 )
, coutstream( theScreenBuffer.cursor(), false )
, cerrstream( theScreenBuffer.cursor(), true )
{

}

bool 
IDisplayImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // clear(); Done in ScreenBuffer constructor already
#ifdef DEBUG
#define _X "DEBUG"
#else
#define _X "RELEASE"
#endif
   
	coutstream.printf(
		"#1f#MyOS V2.x (" MYOS_BUILDDATE " build " MYOS_BUILD ") " _X
		"\nCopyright(C) 2001-2005 by Jeroen van Bemmel (jbemmel@zonnet.nl)"
	);
	return true;
}

void 
IDisplayImpl::deinit( IContext& context )
{

}


// virtual 
myos_result_t
IDisplayImpl::clear(  )  {
   coutstream.seek(0,false);
   cerrstream.seek(0,false);
   theScreenBuffer.clear();
   return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t
IDisplayImpl::print( IO::IStream& in )  {
   return (myos_result_t) coutstream.putBuffer( in.getBuffer() );
}

}}}  // namespaces
