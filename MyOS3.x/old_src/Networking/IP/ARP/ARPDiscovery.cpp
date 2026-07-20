#include "ARPDiscovery.h"
#include "ARPPacket.h"		// for sizeof

#include "TimerFacility/ITiming.h"
#include "TimerFacility/ITimer.h"

#include "Networking/IP/IPComponent.h"
#include "Networking/IP/IPPacket.h"
#include "Networking/IP/IIPEndpoint.h"	// for E_SEND_PACKET_REUSED


namespace MyOS { namespace Networking { namespace ARP {

using namespace IP;

ARPDiscovery::ARPDiscovery( ActiveGatherBuffer& b, EthernetPacket& rq, ENTRY& e ) 
: request(rq), entry(e), tx(b), retries(0), aborted(false)
, timer( IPComponent::getInstance().iTiming->createTimer(*this) )
{
	memset_aligned( pending, 0, sizeof(pending) );	
}

void
ARPDiscovery::start( )
{
	DPRINTK( "\nARPDiscovery::start" );
	
	this->retries = 3;
	tx.addRequest( ASync::Request( request, sizeof(ARPPacket), E_SEND_PACKET_REUSED ) );	
		
	// schedule timeout, 3 seconds (?)
	timer.start( 3 * 1000 * 1000 );
}

void
ARPDiscovery::abort()
{
	// check if started
	if (retries>0) {
		retries = 0;
		aborted = true;
		timer.trigger();	
	}
}

void
ARPDiscovery::end()
{
	DPRINTK( "\nARPDiscovery::end.." );
	
	// TODO: Prevent multiple calls, also when multiple replies are rx'ed
	
	// cancel timeout? or simply let it fire
	timer.cancel();

	// Avoid more packets getting enqueued
	entry.discovery = 0;
	
	// send queued packets, if any
	for (int p=0; p<4; ++p) {
		EthernetPacket* ep;
		if ((ep = pending[p])) {
			DPRINTK( "pending[%X].", p );
			
			u32 *u = (u32*) ep;
			u32 flags = u[0];
			size_t payload = u[1];
			ep->setDestination( entry.hwAddress );
			tx.addRequest( ASync::Request( *ep, payload, flags) );	
		}
	}
	
	// done with this discovery
	delete this;
}

// virtual
void
ARPDiscovery::onTimer( ITimer &)	// -> means timeout
{
	if (--retries > 0) {
		DPRINTK( "\nARP: onTimer, retransmitting..." );
		tx.addRequest( ASync::Request( request, sizeof(ARPPacket), 
			(retries==1) ? 0 : E_SEND_PACKET_REUSED ) );
			
		// in IRQ
		timer.start( 3 * 1000 * 1000 );	
	} else {
		DPRINTK( "\nARP: timeout after 3 retries. Deleting discovery" );

		// invalidate or delete entry
		if (aborted) {
			delete &entry;	
		} else {
			entry.ip.clear();
			entry.discovery = 0;
		}
		delete this;
				
	}	
}
	
}}} // namespace
