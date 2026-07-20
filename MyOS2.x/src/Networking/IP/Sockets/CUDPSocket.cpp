/***************************************************************************
                          C_RawSocket.cpp  -  description
                             -------------------
    begin                : Fri Nov 16 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "CUDPSocket.h"
#include "../IPPacket.h"
#include "buffer.h"
#include "../CIPEndpoint.h"
#include "UDPHeader.h"
#include "MM/new.h"

namespace MyOS { namespace Networking { namespace IP {
	
ipport_t CUDPSocket::nextPort;

ipport_t
CUDPSocket::choosePort()
{
   if (++nextPort<USERPORTSPACE) nextPort=USERPORTSPACE;
   return nextPort;   	
}

CUDPSocket::CUDPSocket( CIPEndpoint& ep, ipport_t port, IDatagramSocketClient* cl )
: IDatagramSocket(ep,E_IPPROTO_UDP,port), client(cl)
{
   // port chosen at open() if 0, so it can be 0 still
   DPRINTK( "\nUDPSocket created on ip=%x port=%u",
      ep.getLocalAddress().host(), port );
}

void
CUDPSocket::operator delete( void *p )
{
	deallocate( p, sizeof(CUDPSocket) );
}

// virtual
void
CUDPSocket::onPacket( IPPacket& ippacket )
{
   // TO DO: defragment if needed (also for TCP sockets...)

	// prevent RACE condition where new packet overwrites remoteIP before accept() ?
	// However, since we're in a callback from an ethernet IRQ handler, it's safe
	// if (receivelock.trylock())
	{
      /// remember the source IP+port as 'connection', for send()      	
      UDPHeader &udp = ippacket.getData<UDPHeader>();
      // DPRINTK( "\nsetRemote src=%x port=%u", ippacket.getSource(), udp->getSourcePort() );
      setRemoteAddress( ippacket.getSource(), udp.getSourcePort() );
		if (client) {
			buffer buf( udp.getData<u8>(), ippacket.getPayloadSize()-sizeof(UDPHeader) );
			client->onRecv( *this, getRemoteSockAddress(), buf );
		}		
		// receivelock.free();
	}
}

// virtual
myos_result_t
CUDPSocket::open( options_t options ) {

   bool wasFixed=getKey().getPort()!=0;
   if (!wasFixed) {
      setLocalPort( choosePort() );
   }

   // may fail due to existing socket
   u32 maxtries = MAXHASHRETRIES;
   do {
      myos_result_t result = endpoint.addSocket( *this );
      if (result==E_MYOS_SUCCESS) {
         return E_MYOS_SUCCESS;
      } else if (!wasFixed) {
         setLocalPort( choosePort() );
      } else break; // wasFixed
   } while ( --maxtries );
   DPRINTK( "\nUnable to open socket on port(range) %u", getKey().getPort() );
   return E_MYOS_EADDRINUSE;
}

// virtual
myos_result_t
CUDPSocket::connect( const sockadr_t& to )
{
   DPRINTK( "\nConnect %x:%u", to.getIP().host(), (u16) to.getPort() );
   myos_result_t r = open(0); // implies open, do this before setting remote address
   setRemoteAddress( to.getIP(), to.getPort() );
   return r;
}
   /** @see CAbstractSocketBase::close() */
// virtual
myos_result_t
CUDPSocket::close()
{
   // make sure that hashkey equals open() -> remove remote info
   setRemoteAddress( 0, 0 );
   return endpoint.removeSocket( *this );
}

   /** @see CAbstractSocketBase::destroy() */
// virtual
myos_result_t
CUDPSocket::destroy()
{
   myos_result_t r = close();
   delete this;
   return r;
}

   /// @return number of bytes sent, or (negative) error on failure
// virtual
myos_result_t
CUDPSocket::send( const buffer& data, options_t flags )
{
   return sendTo( getRemoteSockAddress(), data, flags );
}

// virtual, this could be implemented in base if sizeof(header) is known there
myos_result_t
CUDPSocket::sendTo( const sockadr_t& to, const buffer& data, options_t flags )
{
//  DPRINTK( "\nUDPSocket::sendTo size=%d ", data.getSize() );
//	for (int d=0; d<data.getSize(); ++d) {
//		DPRINTK("%X", data.getData()[d]);	
//	}
	
	size_t payload = data.getSize();
	IPPacket *p = endpoint.createPacket( payload+sizeof(UDPHeader) );
	if (p) {
		memcpy( (&p->getData<UDPHeader>())+1, data.getData(), payload );
		myos_result_t result = sendPacketTo( to, *p, payload, flags );
		// TODO: may crash!
		// endpoint.destroyPacket( *p );
		return result;
	} else return E_MYOS_ENOMEM;
}

// virtual
myos_result_t
CUDPSocket::recv( const buffer& inbuf, options_t flags )
{
   return E_MYOS_E_NOTIMPL;
}

// virtual
myos_result_t
CUDPSocket::recvFrom( const sockadr_t& from, const buffer& inbuf, options_t flags )
{
   return E_MYOS_E_NOTIMPL;
}

// virtual
myos_result_t
CUDPSocket::sendPacket( IPPacket& p, size_t payload, options_t flags )
{
   return sendPacketTo( getRemoteSockAddress(), p, payload, flags );
}

// virtual
myos_result_t
CUDPSocket::sendPacketTo( const sockadr_t& to, IPPacket& p, size_t payload, options_t flags )
{
	if (!to.isValid()) return E_MYOS_ENOTCONN;	// XX should throw exception

	// Apply IP parameter settings
	p.setTTL( getTTL() );
	p.setTOSbits( getTOSbits() );

	// DPRINTK( "\nSending UDP src=%u dest=%u", getLocalPort(), to.getPort() );
	UDPHeader *udp = new ( (void*) &p.getData<u8>() ) UDPHeader( getLocalPort(), to.getPort() );
	return endpoint.send( p, to.getIP(), udp->wrapUp(payload), E_IPPROTO_UDP, flags );
}

// virtual
myos_result_t
CUDPSocket::recvPacket( IPPacket& p, size_t maxpayload, options_t flags )
{
   return E_MYOS_E_NOTIMPL;
}

// virtual
myos_result_t
CUDPSocket::recvPacketFrom(
const sockadr_t& from, IPPacket& p, size_t maxpayload, options_t flags )
{
   return E_MYOS_E_NOTIMPL;
}


}}}   // namespace
