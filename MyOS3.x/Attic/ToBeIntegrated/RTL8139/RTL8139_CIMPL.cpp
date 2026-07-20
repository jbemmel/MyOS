/***************************************************************************
                          RTL8139_CIMPL.cpp  -  description
                             -------------------
    begin                : Mon Aug 20 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#include "RTL8139_CIMPL.h"
#include "memory.h"		// RUNONSTACK

// debug
#include "Trace.h"

namespace RTL8139 {

/// destruct is 'put', but it really doesnt matter much. This way I dont have to
/// implement get()
COMPONENTDESCRIPTION RTL8139C_IDL =
"<COMPONENT name=\"MyOS.Drivers.net.RTL8139C\" version=\"1.1\" language=\"C++\">\n"
	"<FUNCTION name=\"idl\" method=\"get\" result=\"some DTD\"/>\n"
	"<FUNCTION name=\"destruct\" method=\"put\" result=\"bool\"/>\n"
	"<FUNCTION name=\"hotswap\" method=\"put\">\n"
		"<PARAM name=\"driver\" type=\"file:*.component\"/>\n"
	"</FUNCTION>\n"
"</COMPONENT>";

COMPONENTIMPLEMENTATION( RTL8139::RTL8139C, 1, 1 );

RTL8139C::RTL8139C()
	: componentinfo( "MyOS.Drivers.net.RTL8139C", VERSION(1,1), RTL8139C_IDL )
{
    // Normally done by ComponentManager, should perhaps be in some macro
    header.instance = this;
}

RTL8139C::~RTL8139C()
{
	repository = 0;	
}

u32
RTL8139C::initialize( I_InterfaceRepository &m )
{
    // keep the reference
    repository = &m;

    if (!pcisupport.initialize(m, *this)) return false;
    if (!kernelmem.initialize(m, *this)) return false;
		if (!timing.initialize(m, *this)) return false;
		if (!display.initialize(m, *this)) return false;
    return pcidriver.initialize();
}

void
RTL8139C::deinitialize( I_InterfaceRepository &m )
{
	pcidriver.deinitialize();
	display.release( *repository, *this );
	timing.release( *repository, *this );	
	kernelmem.release( *repository, *this );
	pcisupport.release( *repository, *this );
}

void
RTL8139C::destruct()
{
	REQUIRED_INTERFACE<I_ComponentManager> cm;
	if (cm.initialize( *repository, *this )) {
		cm()->removeComponent( *this );
		RUNONSTACK(100);		
		cm()->destroyComponent( header, cm );		// DANGER!! Code section is removed...
	}	
}


I_Interface*
RTL8139C::getInterface( u32 i ) const
{
    return (i==0) ? (I_Interface*) &pcidriver : 0;
}

s32
RTL8139C::put( const char* callarguments, const Buffer &buf, bool cancel /*=false*/ )
{
	if (cancel) {
			kernelmem()->free( (void*) buf.getBytes(), buf.getSize(), KM_NONE );
			return E_CANCELED;
	}

	bool hswap=false;
	if (strcmp(callarguments,"hotswap")==0) {
		hswap=true;
		// goto	replacecode
	} else if (strcmp(callarguments,"destruct")==0) {
		// goto	replacecode
	} else return I_Interface::put( callarguments, buf, cancel );

	/// To make sure the copied code is all on the stack, put this at end
	RUNONSTACK(100);
	if (hswap)
		return hotswap( buf ) ? buf.getSize() : E_ERROR_IN_PUT;		
	else
		destruct();
	return true;
}

/// Alternate (general) way to create components: through buffers
const Buffer
RTL8139C::getWriteBuffer( const char *forcall, size_t bytecount, u32 block )
{
	return Buffer( (u8*) kernelmem()->allocate( *this, bytecount ), bytecount );
}


bool
RTL8139C::hotswap( const Buffer &withComponent )
{
	REQUIRED_INTERFACE<I_ComponentManager> cm;
	if (!cm.initialize( *repository, *this )) return false;

	/// this will call deinitialize, but not destruct
	I_Component *newComponent = cm()->hotswapComponent( *this, withComponent );
	if (newComponent==0) return false;

	/// direct cast, cannot use 'name'
	RTL8139C &newImplementation = (RTL8139C&) *newComponent;
	newImplementation.pcidriver.takeOver( pcidriver );
	RUNONSTACK(100);
	cm()->destroyComponent( header, cm );		// DANGER!! Code section is removed...
	return true;	
}



};  // namespace