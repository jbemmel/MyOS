/***************************************************************************
                          TCPClosed.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPCLOSED_H
#define TCPCLOSED_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

/**
 * 	Fictive state where there is no connection
 */
class TCPClosed : public ITCPState
{
public:
	static TCPClosed* instance() { static TCPClosed _instance; return &_instance; }

	/// I_TCPState methods
	virtual myos_result_t connect( CTCPSocket &s, const sockadr_t& remote, u32 options );
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );

	// Use I_ServerSocket for passive opens	
	virtual bool listen( CTCPSocket &s ) { return false; }
	virtual myos_result_t close( CTCPSocket &s ) { return E_MYOS_E_FAIL; }

	// static method, here to gather all closed related handling in this class
	// void handleTCPPacket( CIPEndpoint &onDevice, IPPacket &tcp );

private:
	TCPClosed() : ITCPState("CLOSED") {}
};

}}}}	// namespace

#endif
