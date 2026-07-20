/***************************************************************************
                          TCPEstablished.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPEstablished.h"
#include "../TCPHeader.h"

/// State transitions
#include "TCPFinWait1.h"
#include "TCPCloseWait.h"
#include "TCPClosed.h"

#include "debug.h"
// #include "Networking/IP/IPPacket.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPEstablished::receive( CTCPSocket &s, TCPHeader &p, size_t ippayload )
{
	// DPRINTK( "\nTCPEstablished::receive" );

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
		if (!s.receiveACK(p)) return;  // include ACK for FIN here
	} else return;	// drop

	// 6. Check URG, TBD
	// 7. Process data, this may destroy the packet -> save FIN flag
	bool isFIN = p.isFinish();
	s.receiveData( p, ippayload );

	// 8. Check FIN
	if ( isFIN ) {
		s.receiveFIN();
		changeState( s, TCPCloseWait::instance() );		// NOTE: In general, perhaps before send?
	}
}

s32	// virtual
TCPEstablished::send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload )
{
	// Delay sending iff executing under receiveData
	// i.e. this is a response to incoming data that has not yet been ACKed
	if (s.inOnPacket) {
		if (s.delayed) {	// previous delayed content, could combine?
			if (s.sendTCPPacket( s.delayed->init( TCP_ACK ), s.delayedPayload, false, E_SEND_PACKET_REUSED )) {
				s.maxSeqSent += s.delayedPayload;
				s.appendSegment( s.delayed );	
			}
		}
		s.delayed = &packet;
		s.delayedPayload = tcppayload;
		return 0;	// don't let caller increment maxSeqSent or appendSegment
	} else {
		if ( s.sendTCPPacket( packet.init( TCP_ACK ), tcppayload, false, E_SEND_PACKET_REUSED ) ) {
		    s.maxSeqSent += tcppayload;		    
		}
		return 1;
	}
}

myos_result_t	// virtual
TCPEstablished::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPEstablished::close->" );

   // first change state
	changeState( s, TCPFinWait1::instance() );

	// Specs: queue until all previous sends have been segmentized
	// Delay this if executing 'onPacket->receiveData'
	if (!s.inOnPacket) {
		s.sendFIN(false);
	}
	return E_MYOS_SUCCESS;
}

}}}} // namespace
