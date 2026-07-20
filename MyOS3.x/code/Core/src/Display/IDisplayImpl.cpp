#include "IDisplayImpl.h"

#include "IStream.h"
#include "../../../build/gen_src/myosconfig.h" // builddate

namespace MyOS { namespace Devices { namespace Display {

// const uuid_t IDisplay::ID = "abe1beba-0457-4375-8a3e-48809a0d5838";

IDisplayImpl::IDisplayImpl( MyOS::Core::IComponent& c ) throw()
: IDisplay( c, VERSION(1,0) )
, theScreenBuffer( (void*) 0xB8000 )
, coutstream( theScreenBuffer.cursor(), false )
, cerrstream( theScreenBuffer.cursor(), true )
{

}

bool
IDisplayImpl::init( Context::IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // clear(); Done in ScreenBuffer constructor already
#ifdef DEBUG
#define _X "DEBUG"
#else
#define _X "RELEASE"
#endif

	coutstream.printf(
		"#1f#MyOS V3.x (" MYOS_BUILDDATE " build " MYOS_BUILD ") " _X
		"\nCopyright(C) 2001-2012 by Jeroen van Bemmel (jbemmel@zonnet.nl)"
	);
	return true;
}

void
IDisplayImpl::deinit( Context::IContext& context )
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
