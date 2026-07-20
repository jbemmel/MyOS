/***************************************************************************
                          TCPSynReceived.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TCPSYNRECEIVED_H
#define TCPSYNRECEIVED_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
/**
 * 	Non-synchronized state after TCPClosed, waiting for handshake to be completed
 */
class TCPSynReceived : public ITCPState
{
public:
	static ITCPState* instance() { static TCPSynReceived _instance; return &_instance; }

	/// I_TCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid calls
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	TCPSynReceived() {}
};

}}}}	// namespace

#endif
