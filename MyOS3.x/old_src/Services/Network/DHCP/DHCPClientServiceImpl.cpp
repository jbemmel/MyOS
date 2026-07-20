#include "Services/Network/DHCP/DHCPClientService.h"
#include "DHCPClientSvcInstance.h"

namespace MyOS { namespace Services { namespace Network { namespace DHCP {

using Networking::IP::IIPEndpoint;

// virtual 
bool 
DHCPClientService::onServicePointCreated( void* servicePoint )
{
	// attach new session to this (assumed) IIPEndPoint	
	IIPEndpoint* endpoint = (IIPEndpoint*) servicePoint;
	DPRINTK( "\nDHCPClient onServicePointCreated: %x", endpoint );
	DHCPClientSvcInstance* svc = new DHCPClientSvcInstance( *endpoint );
	return svc->start( 0 /* no client id yet */ );
}

}}}}	// namespace
