/***************************************************************************
                          IDatagramSocket.h  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef IDATAGRAMSOCKET_H
#define IDATAGRAMSOCKET_H

#include "SocketBase.h"

namespace MyOS { namespace Networking { namespace IP {

/**
 *	DatagramSocket - interface for UDP sockets
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
 *
 * Some recommendations that are currently not implemented:
 *
 * RFC 1122 section 4.1.3.2 : UDP SHOULD pass IP options to the application, and
 *                            vice versa. This currently includes:
 *                            SOURCE_ROUTE, RECORD_ROUTE and TIMESTAMP
 *
 * RFC 1122 section 4.1.3.5 : The application SHOULD be able to specify the
 *                            IP source address to be sent. The intention is that
 *                            this is one of the IP addresses of the host
 *                            -> is implemented implicitly by choosing the endpoint
 */
class IDatagramSocket : public CAbstractSocketBase
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
   virtual myos_result_t connect( const sockadr_t& to ) = 0;

   /** @see CAbstractSocketBase::close() */
   virtual myos_result_t close() = 0;

   /** @see CAbstractSocketBase::destroy() */
   virtual myos_result_t destroy() = 0;   

   /// @return number of bytes sent, or (negative) error on failure
   virtual myos_result_t send( const buffer& data, options_t flags ) = 0;

   /// Sends data (which gets copied) to a remote destination socket	
   /**
    * FOr a more efficient version without copying, @see sendPacket
    */
   virtual myos_result_t sendTo( const sockadr_t& to, const buffer& data, options_t flags ) = 0;
   virtual myos_result_t recv( const buffer& inbuf, options_t flags ) = 0;
   virtual myos_result_t recvFrom( const sockadr_t& from, const buffer& inbuf, options_t flags ) = 0;

   /**
    *	Non-buffered way to send&receive data, not for TCP
    * @return E_MYOS_ENOTCONN if socket has no remote address set
    */
   virtual myos_result_t
   sendPacket( IPPacket& p, size_t payload, options_t flags ) = 0;

   virtual myos_result_t
   sendPacketTo( const sockadr_t& to, IPPacket& p, size_t payload, options_t flags ) = 0;


   virtual myos_result_t
   recvPacket( IPPacket& p, size_t maxpayload, options_t flags ) = 0;

   virtual myos_result_t
   recvPacketFrom( const sockadr_t& from, IPPacket& p, size_t maxpayload, options_t flags ) = 0;

protected:
   IDatagramSocket( IIPEndpoint& ep, E_IPPROTOCOL proto, ipport_t port)
      : CAbstractSocketBase( ep, proto, port ) {}

private:
   // disallowed
   // inline void operator delete( void* ) {}
};

/**
 *	Callback interface for asynchronous operation (optional)
 *	Use by supplying a pointer when the socket is created. When an async
 *	interface is specified, the synchronous recv...() methods return 'E_ASYNC'
 *
 */
class IDatagramSocketClient
{
public:
	/**
	 * Called when data is received on the socket
	 */
	virtual void onRecv( 
		IDatagramSocket& onSocket, 
		const sockadr_t& from, 
		const buffer& data ) = 0;

	/**
	 * (optional) called when socket is about to close, can be used to send last
	 * minute data
	 */
	virtual void onSocketClosing( IDatagramSocket& onSocket ) {}

	/**
	 * Called when an async error occurs on a socket, eg network goes down (?) scalable
	 */
	virtual void onSocketError( IDatagramSocket& onSocket, u32 errorcode ) {}
};

}}} // namespace

#endif
