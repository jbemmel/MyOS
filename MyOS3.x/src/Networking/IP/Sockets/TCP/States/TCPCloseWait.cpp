/***************************************************************************
                          TCPCloseWait.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPCloseWait.h"

#include "../TCPHeader.h"

/// Transitions
#include "TCPLastACK.h"
#include "TCPEstablished.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPCloseWait::receive( CTCPSocket &s, TCPHeader &p, size_t )
{
	DPRINTK( "\nTCPCloseWait::receive" );

	// 2. Check RST
	if (p.isReset()) {
		return s.receiveRST();
	}

	// 3. Check presedence&security, TBD

	// 4. Check SYN
	if (p.isSYN()) {
		return s.receiveSYN(p);
	}	

	// 5. Check ACK
	if (p.hasACK()) {
		if (!s.receiveACK( p )) return;
	} else return;	// drop	

	// 6. ignore URG (since FIN has been received from remote)
	// 7. ignore text

	// 8. Check FIN
	if (p.isFinish()) {
		s.receiveFIN();
	}
}

s32	// virtual
TCPCloseWait::send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload )
{
	DPRINTK( "\nTCPCloseWait::send" );	
	return s.sendTCPPacket( packet.init(TCP_ACK), tcppayload, true, E_SEND_PACKET_REUSED );
}

myos_result_t	// virtual
TCPCloseWait::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPCloseWait::close" );
   
   // Queue request until all preceding sends have been segmentized

   // error in RFC 793, corrected in RFC 1122
   changeState( s, TCPLastACK::instance() );
   return E_MYOS_SUCCESS;
}

}}}}
