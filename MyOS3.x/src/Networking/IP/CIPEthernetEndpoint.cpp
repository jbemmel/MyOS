/***************************************************************************
                          CIPEndPoint.cpp  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "MM/new.h"
#include "CIPEthernetEndpoint.h"
#include "Networking/Ethernet/EthernetPacket.h"
#include "IPPacket.h"
#include "Sockets/CRawSocket.h"
#include "Sockets/UDPHeader.h"

#include "MultiThreading/IMultiThreading.h"
#include "TimerFacility/ITiming.h"

// debug
#include "debug.h"

namespace MyOS { namespace Networking {namespace IP {

using Ethernet::EthernetPacket;
using namespace ARP;
using namespace MultiThreading;

#define ITIMING IPComponent::getInstance().iTiming

CIPEthernetEndpoint::CIPEthernetEndpoint( IEthernetDevice& dev, ipadr_t local, ipadr_t subnet, IIPLayerImpl& l )
: CIPEndpoint( local, subnet, l )
, arp(*this,dev), device(dev)
{
	// for now, only rx (working on async queueing)
	device.open( *this, 0 );	// TODO: check result
}

// virtual
void
CIPEthernetEndpoint::destroy()
{
   device.close();
   arp.destroy();
   destroyEndpoint();
   deallocate( this /*, sizeof(CIPEthernetEndpoint)*/ );
}

inline EthernetPacket&
CIPEthernetEndpoint::toEth( IPPacket& p )
{
   // assumes ethernetpacket precedes IP header
   return *( ((EthernetPacket*) &p) - 1 );
}

// virtual
IPPacket*
CIPEthernetEndpoint::createPacket( size_t maxDataSize )
{
	size_t bytes = sizeof(IPPacket) + maxDataSize;
	if (bytes<46) bytes = 46;		// Minimum Ethernet payload size is 46 bytes
	bytes += sizeof(EthernetPacket);
	
	EthernetPacket* p = (EthernetPacket*) new u8[ bytes ];
	
	// Clear always, especially needed when padding...
	memset_aligned( p, 0, bytes );
	p->setProtocol( ETH_P_IP );		// fill this in already
	return ::new ((void*)(p+1)) IPPacket(maxDataSize);
}

// virtual, also in AGB
void
CIPEthernetEndpoint::destroyPacket( IPPacket *p )
{
	size_t bytes = p->getTotalSize();
	if (bytes<46) bytes = 46;		// Minimum Ethernet payload size is 46 bytes
	
	deallocate( ((EthernetPacket*) p) - 1 /*, bytes + sizeof(EthernetPacket)*/ );
	// device.releasePacket( toEth(p) );
}

// virtual
myos_result_t
CIPEthernetEndpoint::send( IPPacket& p, ipadr_t to, size_t payload, E_IPPROTOCOL proto, u32 flags )
{
	if (!to.isValid()) {
		PRINTK( "\nCIPEthernetEndpoint::send invalid to adr: %x", to.host() );
		return E_MYOS_EINVAL;	// XX throw instead
	}

	// catch packets to self should go to loopback interface
	if (to == getLocalAddress() || to == ipadr_t::LOOPBACK()) {
		PRINTK( "\ntodo: packets to self" );
		return E_MYOS_E_NOTIMPL;
	}
	p.wrapUp( getLocalAddress(), to, payload, proto );	// finish IP part

	if ( to.inSameSubnet( getLocalAddress(), getSubnet() )) {
		// OK, can be broadcast too
	} else if (gateway.isValid()) {
		to = gateway;
	} else {
		PRINTK( "\nARP: IP address %x not on LAN and no gateway", to.host() );
		return E_MYOS_EHOSTUNREACH;
	}
	
	return arp.sendPacket( to, toEth(p), p.getTotalSize(), flags );
}

// virtual
void	// packetsize may include padding bytes...
CIPEthernetEndpoint::onPacket( const buffer& data, u32 flags )
{
	// TODO: (IP?) processing in a separate thread / work item? this is IRQ!
	
	// reinterpret as ethernet packet
	EthernetPacket& p = data.getDataAs<EthernetPacket>();	
	
	// LOG( C_IPSocketLayer, "%s:Received packet @%x size=%d protocol=%x",
	//	device->getDevice().getInstanceID(), &p, packetsize, (u16) p.getProtocol() );

	// DPRINTK( "\nIP:Received packet @%x size=%d protocol=%x",
	//	&p, packetsize, (u16) p.getProtocol() );

	switch (p.getProtocol())
	{
	// TODO: Could receive IEEE 802.1 encapsulated here too, see 
	// http://www.faqs.org/rfcs/rfc1042.html (guess it would be 0xAAAA)
		
	case ETH_P_IP:
    {
      IPPacket& ip = p.getData<IPPacket>();

      // (always) add to ARP cache (?) DoS attack ??
      // see http://www.sics.se/~adam/uip/uip-0.9-refman/a00083.html#a15 who do       
      //
      // For Bochs, doing this since ARP discovery fails (maybe wireless router
      // does not send unicast to non-registered MAC, ARP packet format seems ok
      //
      //if (getLocalAddress().inSameSubnet( ip.getSource(), subnet )) {
	  //  arpcache.addEntry( ip.getSource(), p.getSource() );
      //}
      
      // TODO: Schedule APC here, or add 2nd AGB for rx part
      
      CIPEndpoint::receive( ip, data.getSize() );	// TODO: Pass flags?
      return;
   }

	case ETH_P_ARP:
		// no check on payload size
		arp.handleARPPacket( p );
		break;

	default:
		// LOG( C_IPSocketLayer,  "Unsupported ethernet protocol:%x", p.getProtocol() );
		if (p.isValidLength())
			PRINTK( "\nEthernetpacket of size %u dropped", p.getIEEE802_3Size() );
		else
			PRINTK( "\nUnsupported ethernet protocol:%x", p.getProtocol() );
		break;
	}
}

// virtual
myos_result_t
CIPEthernetEndpoint::setARPPolicy( u32 policyFlags )
{
   arp.setReply( policyFlags&E_ARP_REPLY );
   arp.setAcceptUnsollicitedReply( policyFlags & E_ARP_UNSOLLICITED );
   if (policyFlags&E_ARP_PROXY) {
      // only allow on 192.168 subnet
      if (getLocalAddress().inSameSubnet( ipadr_t(192,168,0,0), subnet) ) {
         arp.setProxy( true );
         return E_MYOS_SUCCESS;
      } else return E_MYOS_EPERM;
   } else arp.setProxy(false);

   return E_MYOS_SUCCESS;
}

// virtual
myos_result_t
CIPEthernetEndpoint::addARPEntry( ipadr_t forAdr, const buffer& hwAdr, u32 isStatic )
{
   return arp.addEntry( forAdr, hwAdr.getDataAs<ethadr_t>() )	// always static
      ? E_MYOS_SUCCESS
      : E_MYOS_E_FAIL;
}

// virtual
myos_result_t
CIPEthernetEndpoint::removeARPEntry( ipadr_t forAdr )
{
   return arp.removeEntry( forAdr )
      ? E_MYOS_SUCCESS
      : E_MYOS_E_FAIL;
}

/* State maintained during each ARP request, TODO associate with cache entry?
struct ARPState {
	CIPEthernetEndpoint* endpoint;
	EthernetPacket* request;
	size_t packetsize;
	u8 retries;
	
	ARPState( CIPEthernetEndpoint* ep, EthernetPacket* rq, size_t s, u8 r ) 
	: endpoint(ep), request(rq), packetsize(s), retries(r) {}

};

bool
CIPEthernetEndpoint::doARPRequest( ipadr_t forAddress, ethadr_t& found )
{
	PRINTK( "\ndoARPRequest for ip=%x", forAddress.host() );

	// rename: discoverAddress ?
	const buffer b = arpcache.findEntry( forAddress );
	if (b.isValid()) {
		found.set( b );
		// TODO: Sent original IP packet then
		return true;	
	}
		
	EthernetPacket* rq = (EthernetPacket*) new u8[ 
		CARPCache::ARP_REQUEST_PACKETSIZE + sizeof(EthernetPacket) ];
	size_t packetsize = arpcache.prepareRequest( getLocalAddress(), forAddress, *rq );
	txBuffer.addRequest( ASync::Request( *rq, packetsize, E_SEND_PACKET_REUSED ) );	
	ITIMING->startTimer( 
		arptimer, 
		ARP_REQUEST_TIMEOUT_MS * 1000, 
		new ARPState(this,rq,packetsize,MAX_ARP_REQUESTS) 
	);
	return false;	// pending...
}

// static
void
CIPEthernetEndpoint::onARPTimeout( timer_t&, void* userdata )
{
	register ARPState* state = (ARPState*) userdata;
	
	// XX Should check cache again?
	
	if (--state->retries) {
		// last time, free the packet
		u32 flags = (state->retries==1) ? 0 : E_SEND_PACKET_REUSED;
		state->endpoint->txBuffer.addRequest( 
			ASync::Request( *state->request, state->packetsize, flags ) 
		);
				
	} else {
		DPRINTK( "\nARP discovery failed" );
		operator delete( state, sizeof(ARPState) );
	}
}
*/

// virtual
myos_result_t
CIPEthernetEndpoint::setGateway( ipadr_t gw )
{
	if (gw.isValid()) {
		gateway = gw;
		DPRINTK( "..gateway set" );
		return E_MYOS_SUCCESS;		
	} else {	
		DPRINTK( "\nInvalid gateway" );
		// gateway = ipadr_t(0);   // clear it?
		return E_MYOS_EHOSTUNREACH;
	}
}

// virtual
myos_result_t
CIPEthernetEndpoint::changeAddress( ipadr_t local, ipadr_t subnet )
{
	myos_result_t r = changeAddressInternal(local,subnet);
	subnetBroadcast = local.getSubnetBroadcast( subnet );

	// XX Should flush ARP cache? Including static entries (cur. no)
	arp.flush( false );

	DPRINTK( "\nchangeAddress result=%x", r );
	return r;
}

/* virtual 
myos_result_t 
CIPEthernetEndpoint::clearAddress()
{
	// arpcache.flush( false ); do this ??
	return CIPEndpoint::clearAddress();
}
*/


}}} // namespace
