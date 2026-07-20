/***************************************************************************
                          TCPFinWait1.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPFinWait1.h"
#include "../TCPHeader.h"

/// State transitions
#include "TCPEstablished.h"		// code sharing
#include "TCPTimeWait.h"
#include "TCPClosing.h"
#include "TCPFinWait2.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPFinWait1::receive( CTCPSocket &s, TCPHeader &p, size_t ippayload )
{
	DPRINTK( "\nTCPFinWait1::rx ACK %x expecting FIN=%x", p.getACK(), s.maxSeqSent );

	// 2. Check RST bit
	if (p.isReset()) {
		return s.receiveRST();
	}

	// 3. Check security/precedence, TBD

	// 4. check SYN
	if (p.isSYN()) {
		return s.receiveSYN(p);
	}

	// 5. check ACK
	u32 ack;
	if (p.hasACK()) {
		ack = p.getACK();
		if (s.receiveACK(p)) {
			if (ack==s.maxSeqSent) {		// FIN acked ?
				changeState( s, TCPFinWait2::instance() );
				// continue processing as in FIN2: (below)
				// if retransmissionqueue empty, ACK the local close()
			}
		} else return;	// done
	} else return;	// drop

	// 6. Check URG, TBD
	// 7. Process data

	// combine ACK for FIN with ACK for data (also if there is not any)
	// if (p.getHSizeBytes() < ippayload) {
	bool isFIN = p.isFinish();
	s.receiveData( p, ippayload );
	// }

	// 8. Check FIN
	if (isFIN) {
		s.receiveFIN();  			// dont sent ack here!
		if (ack==s.maxSeqSent) {	// our FIN acked ?
			TCPTimeWait::enter(s);
		} else {
     	   changeState(s, TCPClosing::instance() );
		}
	}
}

myos_result_t
TCPFinWait1::close( CTCPSocket &s )
{
    DPRINTK( "\nTCPFinWait1::close" );
	return E_MYOS_SUCCESS;
}

}}}} // namespace
