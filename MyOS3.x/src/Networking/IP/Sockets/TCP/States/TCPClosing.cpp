/***************************************************************************
                          TCPClosing.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPClosing.h"
#include "../TCPHeader.h"

/// Transitions
#include "TCPTimeWait.h"
#include "TCPEstablished.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

void	// virtual
TCPClosing::receive( CTCPSocket &s, TCPHeader &p, size_t )
{
	DPRINTK( "\nTCPClosing::receive" );

	// 2. check RST bit
	if ( p.isReset()) {
		return s.receiveRST();
	}

	// TBD 3: check security&presedence

	// 4. check SYN bit
	if (p.isSYN()) {
		return s.receiveSYN(p);
	}

	if (p.hasACK()) {
		// Check if acceptable, if not abort processing (cannot be for our FIN)
		if (s.receiveACK(p)) {
		
			// if ACK is for our FIN
			if (p.getACK()==s.maxSeqSent) {
				s.changeState( TCPTimeWait::instance() );
			}
		}
	} // else drop

	// 6. ignore URG & 7. ignore text

	// 8. no need to check FIN, already received
}

myos_result_t	// virtual
TCPClosing::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPClosing::close = error" );

	// error
   return E_MYOS_E_FAIL;
}

}}}} // namespace
