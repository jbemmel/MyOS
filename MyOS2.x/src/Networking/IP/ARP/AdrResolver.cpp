#include "MM/new.h"
#include "AdrResolver.h"
#include "Networking/IP/IPComponent.h"
#include "Networking/IP/IIPEndPoint.h"
#include "ARPPacket.h"
#include "Timer/ITiming.h"
#include "ARPDiscovery.h"

namespace MyOS { namespace Networking { namespace ARP {

#define ITIMING IPComponent::getInstance().iTiming

// addEntry flags
enum UPDATEFLAGS { UPDATE_ONLY = 0x1, STATIC_ENTRY = 0x2 };

using MultiThreading::Thread;

void 
ENTRY::operator delete( void *p ) {
	deallocate( p, sizeof(ENTRY) );
}

void 
ARPDiscovery::operator delete( void *p ) {
	deallocate( p, sizeof(ARPDiscovery) );
}


AdrResolver::AdrResolver( IIPEndpoint& ep, IEthernetDevice& dev )
: endpoint(ep), txbuf(dev,5), enableReply(1), proxyARP(0), unsollicitedReply(0)
{
	// Default: 1 minute timeout
	cacheTimeoutMs = 60 * 1000;
	
	/// Create the ARP request packet prototype
	new ( &ARPRequestPrototype ) ARPPacket( dev.getLocalAddress() );
		
	Thread &m = IPComponent::getInstance().iMultiThreading->createThread( 0 );
	txbuf.start(m);		
}

void
AdrResolver::destroy()
{
	txbuf.stop();	
	
	// Cancel all outstanding discoveries, free memory
	this->flush(true);
}

// private
ENTRY*
AdrResolver::findEntry( ipadr_t forAddress ) const
{
	ENTRY* entry = cache.get( ENTRY::KEY(forAddress) );
	if (entry) {	
		/// age entries (also static ones)
		entry->lastUsedAt_ms = ITIMING->now( Timer::ms );
		// PRINTK( "\nARP entry found at %d", i );
	}
	return entry;
}

// private, @return entry that was added or updated
ENTRY* 
AdrResolver::addEntry( ipadr_t adr, const ethadr_t& hwAddress, u32 flags )
{
	ENTRY* entry = cache.get( ENTRY::KEY(adr) );
	if (entry) {
		// continue below
		// DPRINTK( "\nARP: Refreshing entry %x @%x", entry->ip.host(), entry );
	} else if ((flags & UPDATE_ONLY)==0) {
		entry = new ENTRY( adr );
		if (!cache.put( ENTRY::KEY(adr), *entry )) {
			// Then replace oldest entry
			ENTRY *e, *oldest=0;
			for( ARPCACHE::Iterator i = cache.iterator(); (e = i.next()); ) {
				if (e->isStatic()) continue;	// skip static entries
				if (!oldest || (e->lastUsedAt_ms < oldest->lastUsedAt_ms) ) {
					oldest = e;
				}
			}
			delete cache.remove( oldest->getKey() );
			if (cache.put( ENTRY::KEY(adr), *entry )) {
				DPRINTK( "\nARP: old entry removed, new one inserted" );				
			} else {
				BUG( "ARP Concurrency?	Remove->add failed" );
			}
		} else {
			DPRINTK( "\nARP: entry for %x inserted @%x", adr.host(), entry );	
		}
	} else {
		// DPRINTK( "...not found and UPDATE_ONLY set" );
		return 0;
	}
	
	// assumes size is still the same
	entry->setAddress( hwAddress );
	entry->lastUsedAt_ms = ITIMING->now( Timer::ms );
	entry->flags |= ENTRY::E_VALID;
	return entry;	
}

bool
AdrResolver::removeEntry( ipadr_t forAddress )
{
	ENTRY* e = cache.remove( ENTRY::KEY(forAddress) );
	if (e) {
		delete e;
		return true;	
	}
	return false;	
}


myos_result_t
AdrResolver::sendPacket( ipadr_t to, EthernetPacket& p, size_t payload, u32 flags )
{
	if (RARELY(to.isBroadcast())) {	// TODO: Local subnet broadcast
		p.setDestination( EthernetPacket::BROADCAST );
	} else {
		ENTRY* entry = findEntry( to );
		if (entry) {
			if (entry->isValid( ITIMING->now( Timer::ms ), cacheTimeoutMs )) {
				p.setDestination( entry->hwAddress );
				
			//
			// This more or less prevents ARP flooding as required by RFC1122
			// (although if the reply is received within a second, a second
			// discovery could theoretically follow...
			//
			} else if (entry->discovery!=0) {	// TODO: atomic
				DPRINTK( "\nEnqueueing packet in ongoing discovery..." );
				// see RFC1122 2.3.2.2  ARP Packet Queue
				entry->discovery->enqueue( p, flags, payload );
				return E_MYOS_EINPROGRESS;
			} else {
				// invalid entry without ongoing discovery, start it ?
				startDiscovery( to, entry );	// XX add p, payload, flags
				return E_MYOS_EINPROGRESS;
			}
			// continue below after send:
		} else {
			// Start new ARP discovery, add initial entry in ARP cache
			ARPDiscovery *d = startDiscovery( to );	// XX add p, payload, flags
			d->enqueue( p, flags, payload );
			return E_MYOS_EINPROGRESS;
		}
	}
	txbuf.addRequest( ASync::Request( p, payload, flags ) );
	return E_MYOS_SUCCESS;	
}

/**
 * @note Not handling duplicate address detection (RFC2131) with src==0 yet (like linux does)
 * 
 */
void 
AdrResolver::handleARPPacket( EthernetPacket& arp )
{
	/// conforms to RFC 826 suggested handling
	ARPPacket &p = arp.getData<ARPPacket>();
	u32 hwType   = p.hwType.netorder();

	if (((hwType!=HTONS16(ARP_ETHER))&&(hwType != HTONS16(ARP_IEEE802)))
	   || (p.protocol.netorder() != HTONS16(Ethernet::ETH_P_IP))
	   || p.hwALength != sizeof(ethadr_t) )
	{
   		DPRINTK( "\nARP: Unsupported hwType and/or protocol %x and/or len %d", 
   			p.protocol.hostorder(), p.hwALength );
		return;
	}

//	DPRINTK( "\nARP: hwType=%x proto=%x opcode=%X alength=%X",
//		hwType, p.protocol.hostorder(), p.opcode.hostorder(), p.hwALength );

	ipadr_t senderIP( *((u32*) p.getSenderProtocolAdr()), true ); // nw order
	ipadr_t targetIP( *((u32*) p.getTargetProtocolAdr()), true ); // nw order

	/// Update existing entry only, do not add as new!
	ENTRY* updated = addEntry( senderIP, * (ethadr_t*) p.getSenderHwAdr(), UPDATE_ONLY );

	/// See if they are asking for us, could search *all* NICs ?
	if ( endpoint.getLocalAddress()==targetIP ||
		(  proxyARP
			&& !targetIP.isSubnetBroadcast(endpoint.getSubnet())  // filter rq for (local) broadcast
			&& !targetIP.isLoopback() )                           // filter rq for loopback
		)
	{
		
		/// Add a new entry if needed, for both requests and replies
        if (updated==0) {
			updated = addEntry( senderIP, * (ethadr_t*) p.getSenderHwAdr(), 0 );
		}

		if ( p.opcode.netorder() == HTONS16(ARPOP_REQUEST) ) {
			
			if (!enableReply) return;

			/// Enter ethernet address of target, before swapping addresses !
			arp.wrapUp( *((ethadr_t*) p.getSenderHwAdr()), Ethernet::ETH_P_ARP );

			/// swap target and sender addresses (assumes right lengths...)
			p.setTargetHwAdr( p.getSenderHwAdr() );
			p.setTargetProtocolAdr( senderIP );
			p.setSenderHwAdr( &ARPRequestPrototype[8] );	// copy from proto
			p.setSenderProtocolAdr( targetIP );

			/// Change it into a reply and send it (non-blocking, just pray)
			p.opcode.setNetOrder( HTONS16(ARPOP_REPLY) );

			txbuf.addRequest( ASync::Request( arp, sizeof(ARPPacket), E_SEND_PACKET_REUSED) );
		} else {
        	
        	/* For replies, check that we were indeed asking for it...
        	 * Done by adding entry before starting discovery process
        	 */
			if (updated->discovery) {
				updated->discovery->end();
			} else {
				DPRINTK( "\nUnsollicited reply addressed to me! Hacker?" );
				// TODO: Remove it again?
			}        				
		}
	} else if (unsollicitedReply) {
		// Also process replies that were not asked by this machine?
		DPRINTK( "\nTODO: Unsollicited replies" );
	}
}

ARPDiscovery* 
AdrResolver::startDiscovery( ipadr_t forIP, ENTRY* entry )
{
	DPRINTK( "startDiscovery for " );
	forIP.print( COUT );
	
	EthernetPacket* rq = (EthernetPacket*) new u8[ sizeof(ARPPacket) + sizeof(EthernetPacket) ];
	ARPPacket &arp = rq->getData<ARPPacket>();

	// Copy prototype (which contains local ethadr_t)
	memcpy( &arp, ARPRequestPrototype, sizeof(ARPRequestPrototype) );
	arp.setSenderProtocolAdr( endpoint.getLocalAddress() );
	arp.setTargetProtocolAdr( forIP );
	memset( arp.getTargetHwAdr(), 0, sizeof(ethadr_t) );		// 0, irrelevant?
	
	/// Enter ethernet broadcastaddress as target
	rq->wrapUp( EthernetPacket::BROADCAST, Ethernet::ETH_P_ARP );

	// Create dummy entry in ARP cache, with cleared address
	if (entry==0) {
		entry = new ENTRY( forIP );
		cache.put( ENTRY::KEY(forIP), *entry );
	}
	ARPDiscovery* discovery = new ARPDiscovery( txbuf, *rq, *entry );
	entry->discovery = discovery;
	discovery->start();
	return discovery;
}
	
void
AdrResolver::flush( bool removeStatics )
{
	ENTRY *e;
	for( ARPCACHE::Iterator i = cache.iterator(); (e = i.next()); ) {
		if ( removeStatics || !e->isStatic()) {
			DPRINTK( "\nARP: Flushing entry IP=%x isStatic=%b", 
				e->ip.host(), e->isStatic() );
			ENTRY *e = cache.remove( e->getKey() );
			if (e->discovery) {
				// cancel ongoing discovery, this frees it and the entry
				e->discovery->abort();
			} else {
				delete e;
			}
		}
	}	
}

void
AdrResolver::invalidateEntry( ipadr_t forIP ) 
{
	ENTRY *e = this->findEntry(forIP);
	if (e) e->invalidate();
}
	
}}}	// namespace
