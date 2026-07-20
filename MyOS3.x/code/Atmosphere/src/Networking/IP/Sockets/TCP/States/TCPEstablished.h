/***************************************************************************
                          TCPEstablished.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TCPEstablished_H
#define TCPEstablished_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
class TCPEstablished : public ITCPState
{
public:
	static TCPEstablished* instance() { static TCPEstablished _instance; return &_instance; }

	/// I_TCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual s32 send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload );

	/// Invalid calls
	virtual myos_result_t close( CTCPSocket &s );
	virtual bool listen( CTCPSocket &s ) { return false; }

	// Code sharing with other states, returns false if caller should abort processing
	//void receiveRST( CTCPSocket &s );
	//void receiveSYN( CTCPSocket &s, TCPHeader &tcppacket );
	//bool receiveACK( CTCPSocket &s, TCPHeader &tcppacket );
	//void receiveData( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );	
	//inline void receiveFIN( CTCPSocket &s ) {
	//	if (ITCPSocketClient *cl=s.getClient()) cl->onRemoteClosed(s);	
	//}

private:
	inline TCPEstablished() : ITCPState("ESTABLISHED") {}
};

}}}}	// namespace

#endif
