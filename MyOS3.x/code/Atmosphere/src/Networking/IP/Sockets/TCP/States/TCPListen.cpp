/***************************************************************************
                          TCPListen.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPListen.h"
#include "../TCPHeader.h"
#include "Networking/IP/IPPacket.h"
#include "CPU/TSC.h"

/// State transitions
#include "TCPSynReceived.h"
#include "TCPClosed.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPListen::receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload )
{
	// DPRINTK( "\nTCPListen::receive" );

	// When this is called, RESET==0 && SYN==1 is already checked

	// TBD p 65: Check precedence & security, if not allowed send RESET
	// see RFC1122 for form: <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK>

	// Allocate new connection, set some options. For now, directly on the socket
	TCPHeader::TCPOPTIONS options;
	u32 fields = tcppacket.parseOptions( options );
	if (fields&TCPHeader::TCP_MSS) {
		// DPRINTK( "\nRemote maxsegsize=%d", options.maxsegsize );
		s.setRemoteSegmentSize( options.maxsegsize );
	}

	u32 random = (u32) (Timer::TSC::read() ^ 1958345871);		// or use class Random
	s.initSequences( random, tcppacket.getSeq()+1 );
}

// Complete the sequence started with receive(), non virtual
bool
TCPListen::accept( CTCPSocket &s )
{
	// used to be: reuse received TCPHeader, construct SYN|ACK and send it	
	DPRINTK( "\nTCPListen::accept -> sendSYN" );
	if (s.sendSYN(false)) {
		changeState( s, TCPSynReceived::instance() );
		return true;
	}
	return false;
}


myos_result_t	// virtual
TCPListen::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPListen::close" );
	// remove socket from HashTable, go to Closed state
	// actual remove is done in socket.destroy(), since this is a single sided
	// close (data may still come in from the peer)
	
	// Should also close pending connections? Currently not
	
	changeState( s, TCPClosed::instance() );

	return E_MYOS_SUCCESS;
}

}}}} // namespace
