/***************************************************************************
                          ITCPState.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef ITCPSTATE_H
#define ITCPSTATE_H

#include "types.h"
#include "../CTCPSocket.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

class TCPHeader;

/// State pattern as described in Design Patterns, p. x
class ITCPState
{
public:
	/// Incoming packets
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload ) = 0;

	// Timers, can be overridden
	virtual void onTimer( CTCPSocket &s ) {}

	/// User actions
	virtual myos_result_t connect( CTCPSocket &s, const sockadr_t &remote, u32 options )
   {
		return E_MYOS_E_FAIL;   // exists
	}
	
	virtual bool listen( CTCPSocket &s ) = 0;
	
	virtual s32 send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload ) {
		return E_NOT_IMPLEMENTED;
	}
	
	virtual myos_result_t close( CTCPSocket &s ) = 0;


protected:
	inline void changeState( CTCPSocket& s, ITCPState* next ) {
		s.changeState(next);
	}

};

}}}}	// namespace

#endif
