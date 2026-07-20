/***************************************************************************
                          TCPLastACK.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPLastACK.h"

#include "../TCPHeader.h"

/// Transitions
#include "TCPLastACK.h"
#include "TCPEstablished.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPLastACK::receive( CTCPSocket &s, TCPHeader &p, size_t )
{
	DPRINTK( "\nTCPLastACK::receive" );

	// RFC793: The only thing that can arrive is an ACK for our FIN
	
}

s32	// virtual
TCPLastACK::send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload )
{
	DPRINTK( "\nTCPLastACK::send TBD" );	
	return -1;
}

myos_result_t	// virtual
TCPLastACK::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPLastACK::close TBD" );
	return E_MYOS_E_NOTIMPL;
}

}}}}
