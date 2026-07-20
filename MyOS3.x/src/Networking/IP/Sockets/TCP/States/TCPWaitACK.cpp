/***************************************************************************
                          TCPWaitACK.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPWaitACK.h"
#include "../TCPHeader.h"

/// Transitions
#include "TCPClosed.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPWaitACK::receive( CTCPSocket &s, TCPHeader &tcppacket, size_t )
{
	DPRINTK( "\nTCPWaitACK::receive" );	

	if (tcppacket.hasACK()) {
		changeState( s, TCPClosed::instance() );
	}
}

}}}} // namespace
