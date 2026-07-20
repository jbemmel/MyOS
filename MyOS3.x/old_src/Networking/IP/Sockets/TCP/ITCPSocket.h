/***************************************************************************
                          ITCPSocket.h  -  description
                             -------------------

    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl

 ***************************************************************************/
#ifndef ITCPSOCKET_H
#define ITCPSOCKET_H

#include "../SocketBase.h"
#include "buffer.h"

namespace MyOS { namespace Networking { namespace IP {

enum E_SOCK_OPTION {
   // E_NONE      = 0,
   E_SO_LINGER    = 1,
};

enum E_SEND_FLAGS {
   // E_NONE = 0,
   E_PUSH	= 1,
};

class ITCPSocket : public CAbstractSocketBase
{
public:
   typedef u32 sendflags_t;	

   virtual myos_result_t connect( const sockadr_t& peer, u32 options ) = 0;

   /** @see CAbstractSocketBase::close() */
   virtual myos_result_t close() = 0;

   /** @see CAbstractSocketBase::destroy() */
   virtual myos_result_t destroy() = 0;   

   /// @return number of bytes sent, or (negative) error on failure
   virtual myos_result_t send( const buffer& data, sendflags_t flags ) = 0;

   /**
    * Force transmission of pending data
    */
   virtual myos_result_t flush() = 0;

protected:
   ITCPSocket( IIPEndpoint& ep, ipport_t port)
      : CAbstractSocketBase(ep,E_IPPROTO_TCP,port) {}

};



// overlap with myos_result_t ??
enum E_SOCKET_ERRORCODE {

	E_SOCKET_OK 				= +0,
	E_SOCKET_ASYNC 			= -1,
	E_SOCKET_CLOSED			= -2,
	E_SOCKET_NODEST			= -3,
	E_SOCKET_NETWORKERROR	= -4,
	E_INVALID_SOCKADDRESS   = -5,
	E_NOT_IMPLEMENTED			= -6,
	E_CONN_EXISTS				= -7,
	E_SOCKET_ERROR				= -8
};


/**
 * Asynchronous callback interface, optional
 */

class ITCPSocketClient
{

public:
	virtual void onRecv( ITCPSocket& onSocket, const buffer& data ) = 0;
	virtual void onRemoteClosed( ITCPSocket& onSocket ) {}
	virtual void onConnectionReset( ITCPSocket& onSocket ) {}

	// same as onRemoteReset ?
	virtual void onRemoteRefused( ITCPSocket& onSocket ) {}
	virtual void onSocketError( ITCPSocket& onSocket, E_SOCKET_ERRORCODE errorcode )	{}
};

}}} // namespace

#endif
