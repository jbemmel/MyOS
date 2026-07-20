/***************************************************************************
                          IRawSocket.h  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef IRawSOCKET_H
#define IRawSOCKET_H

#include "SocketBase.h"

namespace MyOS { namespace Networking { namespace IP {

/**
 *	DatagramSocket - interface for UDP and Raw packet sockets
 *
 *	DatagramSockets are packet- rather than stream oriented. Since the
 * underlying protocol is unreliable and not in-order, a 'stream' is not the
 * appropriate abstraction
 *
 * A streaming interface may be added, but in the context of higher layer
 * protocols (eg TFTP) which have their ways to guarantee in-sequence
 * reliable delivery of data
 *
 *	@todo: Socket options like sendbuffersize, timeouts, ...
 */
class IRawSocket : public CAbstractSocketBase
{
public:
   typedef u32 options_t;

   /**
    * Enables reception and transmission of packets
    * @return E_ALREADY_OPENED
    * @return E_NO_PERMISSION
    * @return E_SUCCESS
    */
   virtual myos_result_t open( options_t options ) = 0;

   /**
    * 'connects to' (stores) the peer address for send() and recv()
    */
   virtual myos_result_t connect( const ipadr_t to ) = 0;

   /** @see CAbstractSocketBase::close() */
   virtual myos_result_t close() = 0;

   /** @see CAbstractSocketBase::destroy() */
   virtual myos_result_t destroy() = 0;

   /**
    *	Non-buffered way to send&receive data, not for TCP
    * @return E_MYOS_ENOTCONN if socket has no remote address set
    */
   virtual myos_result_t
   sendPacket( IPPacket& p, size_t payload, options_t flags ) = 0;

   virtual myos_result_t
   sendPacketTo( ipadr_t to, IPPacket& p, size_t payload, options_t flags ) = 0;

   virtual myos_result_t
   recvPacket( IPPacket& p, size_t maxpayload, options_t flags ) = 0;

   virtual myos_result_t
   recvPacketFrom( ipadr_t from, IPPacket& p, size_t maxpayload, options_t flags ) = 0;

protected:
   IRawSocket( IIPEndpoint& ep, E_IPPROTOCOL proto)
      : CAbstractSocketBase( ep, proto, 0) {}

private:
   // disallowed, but gcc has a problem with 'new' being allowed
   // inline void operator delete( void* ) {}
};

/**
 *	Callback interface for asynchronous operation (optional)
 *	Use by supplying a pointer when the socket is created. When an async
 *	interface is specified, the synchronous recv...() methods return 'E_ASYNC'
 *
 */
class IRawSocketClient
{
public:
	/**
	 * Called when data is received on the socket
	 */
	virtual void onRecv( IRawSocket& onSocket, ipadr_t from, IPPacket& data ) = 0;

	/**
	 * (optional) called when socket is about to close, can be used to send last
	 * minute data
	 */
	virtual void onSocketClosing( IRawSocket& onSocket ) {}

	/**
	 * Called when an async error occurs on a socket, eg network goes down (?) scalable
	 */
	virtual void onSocketError( IRawSocket& onSocket, u32 errorcode ) {}
};

}}} // namespace

#endif
