#ifndef ADRRESOLVER_H
#define ADRRESOLVER_H

#include "../ActiveGatherBuffer.h"
#include "Networking/Ethernet/EthernetPacket.h"
#include "Networking/IP/iptypes.h"
#include "Devices/Network/IEthernetDevice.h"
#include "ARPCache.h"
#include "../HASHTABLE.h"

namespace MyOS { namespace Networking { 
	
namespace IP { class IIPEndpoint; }
	
namespace ARP {

using namespace Ethernet;
using namespace IP;
using Devices::Network::IEthernetDevice;

class ARPDiscovery;

/// Resolves addresses for all transmitted ethernet packets (using ARP)
class AdrResolver
{
public:
	AdrResolver( IIPEndpoint& ep, IEthernetDevice& dev );

	void destroy();	// called when IP endpoint is destroyed

	/**
	 * Schedules an Ethernet for transmission to a given IP address
	 * When the destination MAC address is not yet known, an ARP request is
	 * sent first
	 * 
	 * @return E_MYOS_SUCCESS when packet was accepted (no guarantee for delivery)
	 */
	myos_result_t sendPacket( ipadr_t to, EthernetPacket& p, size_t payload, u32 flags );	
	
	/**
	 * Deals with received ARP requests/responses, which may include replying
	 * and/or updating the ARP address table
	 */
	void handleARPPacket( EthernetPacket& arp );

	/** 
	 * ARP provisioning functions, add/remvoe static entries
	 */
	inline bool addEntry( ipadr_t adr, const ethadr_t& target ) {
		return this->addEntry(adr,target,0) != 0;
	}
	bool removeEntry( ipadr_t adr );
	void flush( bool includingStatic );

	/**
	 * Suggested by RFC1122: a mechanism to invalidate entrys, for use by higher
	 * layers
	 */
	void invalidateEntry( ipadr_t forIp );

	// Policy functions
	inline void setReply( bool enable )                   { enableReply=enable; }
	inline void setAcceptUnsollicitedReply( bool enable ) { unsollicitedReply=enable; }
	inline void setProxy( bool enable )                   { proxyARP=enable; }

	
private:
	enum { LOG2_CACHE_SIZE=5 };		// now: 32 entries

	IIPEndpoint& endpoint;	// to obtain current local IP address

	typedef Util::HASHTABLE<ENTRY::KEY,ENTRY,LOG2_CACHE_SIZE> ARPCACHE;
	ARPCACHE cache;
	ASync::ActiveGatherBuffer txbuf;

	/// prototype ARP request packet, for easy generation of ARP requests 
	/// (fields up until sendHwAdr)
	u8 ARPRequestPrototype[ 8 + 6 ];

	// Policy bits
	u32 enableReply : 1;
	u32 proxyARP : 1;             // do *not* enable this on public networks ! Your ISP will not like you ;)
	u32 unsollicitedReply : 1;    // if this endpoint should accept all replies, or only targeted at us

	// Configurable time before an entry is considered outdated, in ms
	u32 cacheTimeoutMs;

	// private functions	
	ENTRY* findEntry( ipadr_t forAddress ) const;
	ENTRY* addEntry( ipadr_t forAddress, const ethadr_t& adr, u32 flags );
	ARPDiscovery* startDiscovery( ipadr_t forAddr, ENTRY* entry = 0 );
};
	
}}}	// namespace

#endif
