#include "DHCPClientSvcInstance.h"
#include "Services/Network/DHCP/DHCPClientService.h"
#include "debug.h"

namespace MyOS { namespace Services { namespace Network { namespace DHCP {

using Networking::IP::ipadr_t;
using MultiThreading::Thread;

DHCPClientSvcInstance::DHCPClientSvcInstance( IIPEndpoint& ep )
: endpoint(ep)
{
	
}

void
DHCPClientSvcInstance::operator delete( void* p )
{
	deallocate( p, sizeof(DHCPClientSvcInstance) );	
}

bool
DHCPClientSvcInstance::start( const char* clientId )
{
	// 0. Check that we are dealing with an ethernet device
	IEthernetDevice* eth = endpoint.getDevice().castTo<IEthernetDevice>();
	if (eth==0) {
		PRINTK( "\nError: not an ethernet device!" );
		return false;	
	}

	// TODO: Register for linkdown / linkup events too...
	
	// 1. clear the current IP address to 0.0.0.0
	endpoint.clearAddress( );
	dhcpSocket = endpoint.createUDPSocket( this, E_DHCP_CLIENT_PORT );
	if (dhcpSocket) {
		if (dhcpSocket->open(E_NONE)==E_MYOS_SUCCESS) {
			// Better to create a new thread for this...
			runServiceThread(eth->getLocalAddress(), clientId);
			return true;
		}
		dhcpSocket->destroy();
		dhcpSocket = 0;
	}	
	return false;
}

void 
DHCPClientSvcInstance::runServiceThread( const ethadr_t& ifAdr, const char* clientId )
{
    // TODO: Parameters are arbitrary, not convenient
    NVPAIR params[2] = { NVPAIR("ifAdr", &ifAdr), NVPAIR("clientId", clientId) };
    // DPRINTK( "\nrunServiceThread client-id:%s (%x)", clientId, clientId );
    
    /*IThread& t = */ DHCPClientService::getInstance().iMultiThreading->createThread( *this, params, 2 );	
}

// virtual 
int 
DHCPClientSvcInstance::run( NVPAIR params[] )
{
	DPRINTK( "\nDHCPClientSvcInstance::run" );
	if (doDiscovery( *params[0].getAs<ethadr_t>(), params[1].getAs<char>() )) {
		PRINTK( "\nGot IP address: " );
		options.assignedIP.print( COUT );
		
		// XX For testing ARP discovery locally, works!
		// options.router = ipadr_t( 192, 168, 2, 101 );
		// end XX
		
		endpoint.changeAddress( options.assignedIP, options.subnetMask );		
		endpoint.setGateway( options.router );
		return 0;
	}
	return -1;
}

bool
DHCPClientSvcInstance::doDiscovery( const ethadr_t& ifAdr, const char* clientId )
{
	// Construct a discovery packet, on stack (data is copied by socket.sendTo)
	DHCPPacket discover( ifAdr, E_DHCP_DISCOVER );
	// discover.setBroadcast(); All examples say flags=0, so ..

   // In contrary to the RFC, in practice the reply is broadcast
   // see http://www.j51.com/~sshay/tcpip/dhcp/dhcp.htm for example
   // -> trick ??

   this->clientIdUsed = clientId;   // keep it for subsequent packets
   if (clientId) {
      DPRINTK( "\nUsing client-id:%s (%x)", clientId, clientId );   	
      discover.addClientIdOption( clientId );
   }

   // set flag to false
   gotReply=false;

   // Using ARP static entry, BROADCAST is broadcast on ethernet
   // RFC: *must* set source IP to 0, done below
   dhcpSocket->sendTo(
      sockadr_t( ipadr_t::BROADCAST(), E_DHCP_SERVER_PORT),
      buffer( (u8*) &discover, discover.wrapUp() ), E_NONE );

   // XX Take care not to run under IDLE thread! Cannot sleep...
   if ( sync.waitUs(MAX_WAIT_US) ) {  	
      // DPRINTK( "\nsync.wait ended" );   		
      return options.assignedIP.isValid();
   } else {
      PRINTK( "\nDHCP timeout!" );
      return false;
   }
}

// virtual
void
DHCPClientSvcInstance::onRecv( IDatagramSocket& onSocket, const sockadr_t& from, const buffer& data )
{
	DPRINTK( "\nGot data on my DHCP client socket: %u bytes", data.getSize() );

	const DHCPPacket& dhcp = data.getDataAs<DHCPPacket>();

   //TODO: match with last txId sent
   // if (...)

   // parseOptions checks cookie value
   if (dhcp.isReply() && dhcp.parseOptions( data.getEnd(), options )) {
      // check for DHCP type in options
      switch (options.packetType)
      {
/**/
      case E_DHCP_OFFER:
         // only accept first offer, ignore others
         if (!gotReply) {
            gotReply = true;
            // send request, dont wake thread yet
            // TODO: In other thread! Must copy packet, since it will get deleted
            // OR: Make sendTo usable from within IRQ context
            doRequest( dhcp );
         }
         return;
/**/

      case E_DHCP_NAK:
         // invalidate options, clear flag
         DPRINTK( "\nIP address request denied" );
         options.assignedIP = 0;
         gotReply = false;
         break;

      case E_DHCP_ACK:
         // interpret as accepted, options parsed are valid
         DPRINTK( "\nIP address acknowledged" );
         break;

      default:
         DPRINTK( "\nUnhandled DHCP packet type:%u", options.packetType );
         break;
      }

      // here, wake thread
      sync.signal();
   } else DPRINTK( "Not a reply, or error parsing options" );
}

bool
DHCPClientSvcInstance::doRequest( const DHCPPacket& offer )
{
	// Construct a request
	DHCPPacket request( offer, E_DHCP_REQUEST );

	// RFC 2131: Request MUST contain server id & requested IP (set to youradr)
	request.addServerIdOption( options.serverIp );
	request.addRequestedIPOption( offer.getAssignedIP() );

	// RFC 2131: If clientid is used, it MUST be used in all packets
	if (clientIdUsed) {
		request.addClientIdOption( clientIdUsed );
	}

	DPRINTK( "\nRequesting DHCP address..." );
	dhcpSocket->sendTo(
		// broadcast, such that all servers know which one was chosen
		sockadr_t( ipadr_t::BROADCAST(), E_DHCP_SERVER_PORT),
		buffer( (u8*) &request, request.wrapUp() ), E_NONE );
	return true;
}
	
}}}}	// namespace
