/***************************************************************************
                          TCPFinWait1.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TCPFinWait1_H
#define TCPFinWait1_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
/**	
 * FIN-WAIT-1 - represents waiting for a connection termination request
 * from the remote TCP, or an acknowledgment of the connection
 * termination request previously sent.	
 */
class TCPFinWait1 : public ITCPState
{
public:
	static ITCPState* instance() { static TCPFinWait1 _instance; return &_instance; }

	/// I_TCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );

	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid calls
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	TCPFinWait1() {}
};

}}}}	// namespace

#endif
