/***************************************************************************
                          TCPClosed.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPClosed.h"

/// Transitions
#include "TCPSynSent.h"
#include "../TCPHeader.h"

#include "Networking/IP/IPPacket.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPClosed::receive( CTCPSocket &s, TCPHeader &tcppacket, size_t )
{
	// IPLOG( "TCPClosed::receive" ));
	return;		// socket must be opened before handling data
}

myos_result_t
TCPClosed::connect( CTCPSocket &s, const sockadr_t& remote, u32 options )
{
	s.setRemoteAddress( remote.getIP(),remote.getPort() );

	// First try to add socket to table, to detect conflicts
	// (cycle a bit if no port specified
	bool added=false;
	ipport_t p;
	do {
		p = s.choosePort(0);
		added = s.getEndpoint().addSocket( s );
	} while (!added && (p != CTCPSocket::USERPORTSPACE));

	if (!added) {
		return (myos_result_t) E_CONN_EXISTS;
	} else if (s.sendSYN(true)) {
		changeState( s, TCPSynSent::instance() );
		return E_MYOS_SUCCESS;
	} else {
		// remove socket again
		s.getEndpoint().removeSocket( s );
		return (myos_result_t) E_SOCKET_ERROR; 	// perhaps reset remote data
	}
}

}}}} // namespace
