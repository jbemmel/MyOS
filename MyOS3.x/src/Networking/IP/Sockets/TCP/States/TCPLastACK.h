/***************************************************************************
                          TCPLastACK.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPLastACK_H
#define TCPLastACK_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

class TCPLastACK : public ITCPState
{
public:
	static ITCPState* instance() { static TCPLastACK _instance; return &_instance; }

	/// ITCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual s32 send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload );
	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid methods
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	inline TCPLastACK() : ITCPState("LAST-ACK") {}
};

}}}}	// namespace

#endif
