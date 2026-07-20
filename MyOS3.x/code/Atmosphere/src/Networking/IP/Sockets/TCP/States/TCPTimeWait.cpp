/***************************************************************************
                          TCPTimeWait.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPTimeWait.h"
#include "../TCPHeader.h"

/// Transitions
#include "TCPClosed.h"

#include "ITimer.h"
#include "Services/Network/TCP/TCPService.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

using Services::Network::TCP::TCPService;
	
void	// virtual
TCPTimeWait::receive( CTCPSocket &s, TCPHeader &tcppacket, size_t )
{
	DPRINTK( "\nTCPTimeWait::receive" );	

	// (added by me) Check RST first
	if (tcppacket.isReset()) {
		return s.receiveRST();
	}

	// spec: only thing that can happen is retransmitted FIN -> ACK it
	if (tcppacket.hasACK() && tcppacket.getACK() == s.una ) {
		// resend our own FIN? don't think so...
   		s.sendTCPPacket( tcppacket.init(TCP_ACK), 0, true, E_SEND_PACKET_REUSED );
   		
   		// TODO: restart 2*MSL timeout
	} else {
		DPRINTK( "..ACK mismatch:got %u expected %u", tcppacket.getACK(), s.una );
	}
}

// static
void
TCPTimeWait::enter( CTCPSocket &s )
{
	// DPRINTP( "\nTCPTimeWait::enter" );
	
	// start timer
	// TODO: This should be 2*MSL (max segment lifetime)...cur 2 secs
    s.changeState( TCPTimeWait::instance() );
    s.timer.start( 2000 * 1000 );	
}

// virtual 
void 
TCPTimeWait::onTimer( CTCPSocket &s )
{
	DPRINTK( "\nTimeWait::onTimer (2 sec) -> destroy socket" );
	s.destroy();	
}



}}}} // namespace
