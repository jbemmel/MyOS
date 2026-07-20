/***************************************************************************
                          CIPEndPoint.cpp  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "CIPEndpoint.h"
#include "IPPacket.h"
#include "Sockets/CRawSocket.h"
#include "Sockets/CUDPSocket.h"
#include "Sockets/UDPHeader.h"
#include "Sockets/TCP/ITCPSocketFactory.h"

// include these to handle invalid tcp packets
#include "Sockets/TCP/TCPHeader.h"
// #include "Sockets/TCP/States/ITCPState.h"

// debug
#include "debug.h"

namespace MyOS { namespace Networking { namespace IP {
	
CIPEndpoint::CIPEndpoint( ipadr_t local, ipadr_t subnet, IIPLayerImpl& l )
: IIPEndpoint( local, subnet ), layer(l)
{
   // doesnt check the address(!)
}

void
CIPEndpoint::destroyEndpoint()
{
   layer.removeEndpoint( *this );

   SOCKETSET::Iterator i = sockets.iterator();
   while (CAbstractSocketBase* s = i.next() ) {
      // this will call remove...works ?
      s->destroy();
      // no need for remove
   }
}

void	// packetsize may include padding bytes...
CIPEndpoint::receive( IPPacket &ip, size_t packetsize )
{
	// LOG( C_IPSocketLayer, "%s:Received packet @%x size=%d protocol=%x",
	//	device->getDevice().getInstanceID(), &p, packetsize, (u16) p.getProtocol() );
	// DPRINTK( "\nReceived packet @%x size=%d protocol=%x",
	//	&ip, packetsize, (u16) ip.getProtocol() );

	// ip.debugDump( "%c" );

	// Verify header checksum + version, TODO: length too ? hsize ?
	if (RARELY(!(ip.verifyChecksum() && ip.checkVersion( IPPacket::IPv4 )))) {
		PRINTK( "\nSomething wrong with IP packet (wrong version/checksum)");
		return;	
	}

	// RFC 1122 : Should silently discard packets with broadcast or multicast src
	// <not implemented>

	// verify it matches our IP address (should check subnet broadcast match too)
	if (RARELY(ip.getDestination()!=getLocalAddress()
		&& ip.getDestination()!=ipadr_t::BROADCAST()
		&& (ip.getDestination().isSubnetBroadcast(subnet)==false))
	) {

		// XX A router would forward this packet

		PRINTK( "\nIPAddress mismatch: %x != %x -> sent to client=%b",
			ip.getDestination().host(), getLocalAddress().host(),
			getLocalAddress().getSubnetBroadcast(subnet).host(), client!=0 );

      // sent to client below
   } else {
   		
   		// TODO: Process any IP options (before reassembly)
   	
   		// TODO: Implement defragmentation
   		if (ip.hasMoreFragments()) {
   			PRINTK( "\nmore fragments? packetsize=%d fragoffset=%d", packetsize, ip.getFragmentOffset() );
   			// ip.debugDump();
   			BUG( "Defragmentation not implemented yet!" );
   			return;
   		}
   	
		// first check protocol before assuming there is a port
		ipport_t port=0;
		switch (ip.getProtocol())
		{
         /// Only supporting these for now, could use separate hashtable per protocol...
			case E_IPPROTO_TCP:
			{
	            TCP::TCPHeader& tcph = ip.getData<TCP::TCPHeader>();
	            port = tcph.getDestinationPort();
	            CAbstractSocketBase::KEY key( E_IPPROTO_TCP, port );

				//	Sockets are hashed using the 4-tuple
				//		<protocol, remote-IP, remote-port, local-port>
				//	local-IP is the same for all packets arriving at an endpoint
				//  so there is not much point to include it in the hash
				//
				//	ServerSockets are hashed using only <protocol,local-port> 
				//  Therefore, to find a serversocket in the hashtable
				//  (only needed for initial connection attempts) the hash is
				//  different
	            if (tcph.isSYN()==false) {
	               key.setRemoteAddress( ip.getSource(), tcph.getSourcePort() );
	            } // else sockets.get will return a server socket
				CAbstractSocketBase* sock = sockets.get(key);

				// Note: new connection will check if it already exists when the socket
				// is added to the hashtable
				if (sock) {
					sock->onPacket( ip );
				} else {
		    	   DPRINTK( "\nNo open TCP socket found for %x:%d TCP_flags=%x",
					   ip.getDestination().host(), port, tcph.getFlags() );
					
               		// send reset
					unhandledTCPPacket( ip );
					// could try general case
				}
				return;
			}

		case E_IPPROTO_UDP:
            port = ip.getData<UDPHeader>().getDestinationPort();
            // fall through

		case E_IPPROTO_ICMP:   // then ignore port
		{
      		// IP matched, this match based on protocol+(UDP)port only
			const CAbstractSocketBase::KEY key( ip.getProtocol(), port );
			CAbstractSocketBase* sock = sockets.get(key);
			if (sock) {
				// PRINTK( "\nFound socket, passing packet proto=%d", ip.getProtocol() );
				sock->onPacket( ip );
				return;
			} else {
				PRINTK( "\nNo UDP/Raw socket found for %x:%d",
					ip.getDestination().host(),
					ip.getProtocol()==E_IPPROTO_UDP
                	? ip.getData<UDPHeader>().getDestinationPort() 
                  	: 0 
				);
				// RFC 1122: should send an ICMP port unreachable here (for UDP)
			}
			break;
		}

			default:
/*
            // For other protocols : try 1st interface

				for (u32 i=0; i<MAXENDPOINTS; ++i) {
		    	   if (endpoints[i] && endpoints[i]->receive(*ip)) return;
				}
*/
				break;
		}  // end switch
   }

   // if code comes here, packet was not delivered
   if (client) client->onPacket( ip, packetsize );
}

// not virtual
myos_result_t
CIPEndpoint::changeAddressInternal( ipadr_t local, ipadr_t newSubnet )
{
   // need to do more ? close existing TCP sockets explicitly ? they will timeout
   if (local.isValid()
      && !local.isSubnetBroadcast(newSubnet.isValid()?newSubnet:subnet)) {
      localAddress = local;
      if (newSubnet.isValid()) subnet = newSubnet;
      return E_MYOS_SUCCESS;
   } else return E_MYOS_E_INVALIDARG;
}

// virtual 
myos_result_t 
CIPEndpoint::clearAddress()
{
	localAddress = ipadr_t::ANY();
	subnet		 = ipadr_t::ANY();
	return E_MYOS_SUCCESS;	
}


// virtual
ITCPServerSocket*
CIPEndpoint::createTCPServerSocket(
   ipport_t localport, ITCPServerSocketClient* client )
{
   ITCPSocketFactory* factory = layer.getTCPFactory();
   return factory ? factory->createTCPServerSocket( *this, localport, client ) : 0;
}

// virtual
ITCPSocket*
CIPEndpoint::createTCPSocket( ITCPSocketClient *client )
{
   ITCPSocketFactory* factory = layer.getTCPFactory();
   return factory ? factory->createTCPSocket( *this, client ) : 0;
}

// virtual
IDatagramSocket*
CIPEndpoint::createUDPSocket( IDatagramSocketClient *client, ipport_t localport /*=0*/)
{
   return new CUDPSocket(*this,localport,client);
}

// virtual
IRawSocket*
CIPEndpoint::createRawSocket( E_IPPROTOCOL proto, IRawSocketClient *client )
{
   return new CRawSocket(*this,proto,client);
}

myos_result_t
CIPEndpoint::addSocket( CAbstractSocketBase& sock )
{
	ASSERTION( sock.getKey().hash() != 0, E_CRITICAL );
	return sockets.put( sock.getKey(), sock ) ? E_MYOS_SUCCESS : E_MYOS_EEXIST;
}

myos_result_t
CIPEndpoint::removeSocket( CAbstractSocketBase& sock )
{
   return sockets.remove( sock.getKey() ) ? E_MYOS_SUCCESS : E_MYOS_ENOENT;
}

// virtual
void
CIPEndpoint::listSockets( IO::OStream& out ) const
{
   SOCKETSET::Iterator i = sockets.iterator();
   while ( CAbstractSocketBase* sock = i.next()) {
      ipadr_t l = sock->getLocalAddress();
      out.printf( "\n<socket local=\"%u.%u.%u.%u:%u\"",
         l[0],l[1],l[2],l[3], (u16) sock->getLocalPort() );
      ipadr_t r = sock->getRemoteAddress();
      if (r.isValid()) {
         out.printf(" remote=\"%u.%u.%u.%u:%u\"",
            r[0],r[1],r[2],r[3], (u16) sock->getRemotePort() );
      }
      out.printf( " protocol=\"%X\"/>", (u8) sock->getProtocol() );
   }
}

void	// called when no socket is found for a packet
CIPEndpoint::unhandledTCPPacket( IPPacket &ip )
{
	TCP::TCPHeader &h = ip.getData<TCP::TCPHeader>();
	DPRINTK( "\ngot unhandled TCP seq=%u", h.getSeq() );	
	if (h.isReset()==false) {		
		// TCP p36: send reset in response (reusing segment)
		size_t payload = h.createReset();
		send( ip, ip.getDestination(), payload, E_IPPROTO_TCP, E_SEND_PACKET_REUSED );
	}
}


}}} // namespace
