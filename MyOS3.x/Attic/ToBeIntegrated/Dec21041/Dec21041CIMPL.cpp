/***************************************************************************
                          Dec21041CIMPL.cpp  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
***************************************************************************/
#include "Dec21041CIMPL.h"
#include "InterfaceRepository.h"
#include "memory.h"

namespace Dec21041 {

COMPONENTIMPLEMENTATION( Dec21041::DEC21041, 1, 1 );

COMPONENTDESCRIPTION DEC21041_IDL =
"<COMPONENT name=\"MyOS.Drivers.net.Dec21041\" version=\"1.1\" language=\"C++\">\n"
	"<FUNCTION name=\"idl\" method=\"get\" result=\"some DTD\"/>\n"
	"<FUNCTION name=\"destruct\" method=\"get\" result=\"bool\"/>\n"
	"<FUNCTION name=\"replace\" method=\"put\">\n"
		"<PARAM name=\"driver\" type=\"file:*.component\"/>\n"
	"</FUNCTION>\n"
"</COMPONENT>";

DEC21041::DEC21041()	// INITSECTION
	: componentinfo( "MyOS.Drivers.net.Dec21041", VERSION(1,1), DEC21041_IDL ),
		initialized(false)
{
	header.instance = this;
}

u32
DEC21041::initialize( I_InterfaceRepository &r ) // INITSECTION
{
	repository = &r;

	if (!kernelmem.initialize(r,*this)) return false;
	if (!pcisupport.initialize(r,*this)) return false;
	if (!display.initialize(r,*this)) return false;
	if (!timing.initialize(r,*this)) return false;

	*display() << "\nInitializing Dec21041 driver...";
	return (initialized = driver.initialize() );
}

void
DEC21041::deinitialize( I_InterfaceRepository &r )
{
	driver.deinitialize();

	timing.release( r, *this );
	display.release( r, *this );
	pcisupport.release(r, *this);
	kernelmem.release(r, *this);
}

void
DEC21041::destruct()
{
	REQUIRED_INTERFACE< I_ComponentManager > cm;
	if (cm.initialize( *repository, *this )) {
		cm()->removeComponent( *this );

		/// hack: copy some code to the stack, and execute from there!
		RUNONSTACK(100);
		cm()->destroyComponent( header, cm );		// DANGER!! Code section is removed...
	}	
}

s32
DEC21041::get( const char* callarguments, PacketWriter& buf )
{
	if (strcmp(callarguments,"destruct")==0) {
			buf << "<RESULT> DEC21041 was destroyed </RESULT>";

			/// hack: copy some code to the stack, and execute from there!
			RUNONSTACK(100);
			destruct();
			return true;
	} else return I_Interface::get( callarguments, buf );
}

};		// namespace
