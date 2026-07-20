/***************************************************************************
                          TCPSynReceived.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPSynReceived.h"
#include "../TCPHeader.h"

/// State transitions
#include "TCPEstablished.h"
#include "TCPCloseWait.h"
#include "TCPFinWait1.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPSynReceived::receive( CTCPSocket &s, TCPHeader &p, size_t ippayload )
{
	DPRINTK( "\nTCPSynReceived::receive" );

	// 2. Check RST
	if (p.isReset()) {
  		if (!s.server) {		// was actively opened
			if (ITCPSocketClient *c=s.getClient()) c->onRemoteRefused(s);
		}
		s.destroy();
		return;	  	
	}

	// 3. Check security/precedence, TBD

	// 4. Check SYN
	if (p.isSYN()) {	// error
		return s.receiveSYN(p);
	}

	if (p.hasACK()) {
		// Note: left edge different from Established!
		if (CTCPSocket::inWindow(s.una,s.maxSeqSent, p.getACK())) {

        	// RFC 1122: Following variables must be set, same as SYN_SENT state
         	s.remoteWindow = p.getWindow();
			s.WL1 = p.getSeq();
			s.WL2 = p.getACK();

			changeState( s, TCPEstablished::instance() );
			// continue processing, in ESTABLISHED ?
		} else {
			s.sendReset(p);
			DPRINTK( "..ACK out of window expect=%u received=%u", s.una, p.getACK() );
			return;	// ???
		}
	} else return;	// drop it and return

	// 6. Check URG does not apply for this state

	// 7. Process segment text, as in ESTABLISHED ??
		

	// 8. Check FIN (it implies PUSH towards local user...)
	if (p.isFinish()) {
		++s.nextSEQExpected;
  		if (ITCPSocketClient *c=s.getClient()) c->onRemoteClosed(s);
  		// ACK the FIN, could carry data
		s.sendTCPPacket( p.init(TCP_ACK), 0, p.isFinish(), E_SEND_PACKET_REUSED );
		changeState( s, TCPCloseWait::instance() );
	}
}

myos_result_t
TCPSynReceived::close( CTCPSocket &s )
{
	DPRINTK( "TCPSynReceived::close" );
	
	s.sendFIN(false);    // XX could have received a FIN too??
	changeState( s, TCPFinWait1::instance() );

	return E_MYOS_SUCCESS;
}

}}}} // namespace
