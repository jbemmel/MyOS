/***************************************************************************
                          TCPSynSent.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPSynSent.h"
#include "../TCPHeader.h"

/// State transitions
#include "TCPSynReceived.h"
#include "TCPEstablished.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPSynSent::receive( CTCPSocket &s, TCPHeader &p, size_t ippayload )
{
	DPRINTK( "\nTCPSynSent::receive" );

	// 1. check ACK bit
	if (p.hasACK()) {
		if (p.getACK() <= s.initialSeq || p.getACK() > s.maxSeqSent) {
			if (p.isReset()==false) {
				s.sendTCPPacket(p.init(TCP_RESET), p.createReset(), false, E_SEND_PACKET_REUSED );
			}
		} else if (!CTCPSocket::inWindow(s.una,s.maxSeqSent, p.getACK())) {
			DPRINTK( "..ACK not acceptable:%u", p.getACK() );
			return;
		}
	}
			
	// 2. Check RST bit
	if (p.isReset()) {
		return s.receiveRST();
	}

	// 3. Check precedence/security, TBD

	// 4. Check SYN
	if (p.isSYN()) {
		s.nextSEQExpected = p.getSeq()+1;
		// s.initialRemoteSEQ = p.getSeq();				
		if (p.hasACK()) {
			s.maxSeqSent = p.getACK();
			// TBD: remove segments from buffered/retransmission queue

			if (s.maxSeqSent>s.initialSeq || s.initialSeq==0xFFFFFFFF) {	// wrap, assumes 1 byte

	            // RFC 1122: Following variables must be set
	            s.remoteWindow = p.getWindow();
	            s.WL1 = p.getSeq();
	            s.WL2 = p.getACK();
	
	   	     	changeState( s, TCPEstablished::instance());
	
				// could contain FIN ?
	            // ACK it, add data ?
				s.sendTCPPacket( p.init(TCP_ACK), 0, p.isFinish(), E_SEND_PACKET_REUSED );
				// TBD: buffer/process data from segment
			}	
		} else {	// SYN but no ACK: simultaneous handshake
	     	changeState( s, TCPSynReceived::instance());
			// ACK it, could add data
         	s.sendTCPPacket( p.init(TCP_SYN|TCP_ACK), 0, p.isFinish(), E_SEND_PACKET_REUSED );
			// TBD: buffer data from segment
		}
	}
}

myos_result_t
TCPSynSent::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPSynSent::close" );

	s.sendFIN(false);       // XX wrong if FIN was received...
	return E_MYOS_SUCCESS;  // do more ?? switch state ?
}

}}}} // namespace
