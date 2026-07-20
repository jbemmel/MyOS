/***************************************************************************
                          C_3c90xcCIMPL.cpp  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "3c90xcCIMPL.h"

namespace _3c90xc {

COMPONENTIMPLEMENTATION( C_3c90xc::C_3c90xc, 1, 1 );

C_3c90xc::C_3c90xc()	// INITSECTION
	: componentinfo( "MyOS.Drivers.net.3c90xc", VERSION(1,1), "" )
{
	header.instance = this;
}

u32
C_3c90xc::initialize( I_InterfaceRepository &r ) // INITSECTION
{
	repository = &r;

	if (!kernelmem.initialize(r,*this)) return false;
	if (!pcisupport.initialize(r,*this)) return false;
	if (!display.initialize(r,*this)) return false;
	if (!timing.initialize(r,*this)) return false;
	return driver.initialize();
}

void
C_3c90xc::deinitialize( I_InterfaceRepository &r )
{
	driver.deinitialize();
	timing.release(r, *this);
	display.release(r, *this);
	pcisupport.release(r, *this);
	kernelmem.release(r, *this);
}

void
C_3c90xc::destruct()
{
	REQUIRED_INTERFACE<I_ComponentManager> cm;
	if (cm.initialize( *repository, *this )) {
		cm()->removeComponent( *this );
		RUNONSTACK(100);
		cm()->destroyComponent( header, cm );		// DANGER!! Code section is removed...
	}	
}


};