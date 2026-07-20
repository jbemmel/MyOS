#ifndef CUDPSocket_H
#define CUDPSocket_H

#include "IDatagramSocket.h"
#include "Networking/IP/IPComponent.h"

// #include "SpinLock.h"

namespace MyOS { namespace Networking { namespace IP {

using MyOS::buffer;
class CIPEndpoint;

/**
 *
 */
class CUDPSocket : public IDatagramSocket
{
   // packet reception function
   virtual void onPacket( IPPacket& packet );

public:
	CUDPSocket( CIPEndpoint& ep, ipport_t port, IDatagramSocketClient* client );

	void operator delete( void *p );

   /**
    * Enables reception and transmission of packets
    * @return E_ALREADY_OPENED
    * @return E_NO_PERMISSION
    * @return E_SUCCESS
    */
   virtual myos_result_t open( options_t options );

   /**
    * 'connects to' (stores) the peer address for send() and recv()
    */
   virtual myos_result_t connect( const sockadr_t& to );

   /** @see CAbstractSocketBase::close() */
   virtual myos_result_t close();

   /** @see CAbstractSocketBase::destroy() */
   virtual myos_result_t destroy();

   /// @return number of bytes sent, or (negative) error on failure
   virtual myos_result_t send( const buffer& data, options_t flags );
   virtual myos_result_t sendTo( const sockadr_t& to, const buffer& data, options_t flags );
   virtual myos_result_t recv( const buffer& inbuf, options_t flags );
   virtual myos_result_t recvFrom( const sockadr_t& from, const buffer& inbuf, options_t flags );

   /**
    *	Non-buffered way to send&receive data, not for TCP
    * @return E_MYOS_ENOTCONN if socket has no remote address set
    */
   virtual myos_result_t
   sendPacket( IPPacket& p, size_t payload, options_t flags );

   virtual myos_result_t
   sendPacketTo( const sockadr_t& to, IPPacket& p, size_t payload, options_t flags );

   virtual myos_result_t
   recvPacket( IPPacket& p, size_t maxpayload, options_t flags );

   virtual myos_result_t
   recvPacketFrom( const sockadr_t& from, IPPacket& p, size_t maxpayload, options_t flags );

private:
   enum { USERPORTSPACE=1024, MAXHASHRETRIES=100 };
   static ipport_t nextPort;
   static ipport_t choosePort();

   IDatagramSocketClient* client;   // optional async client
};

}}}	// namespace

#endif //CUDPSocket_H
