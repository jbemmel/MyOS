#ifndef ARPCACHE_H
#define ARPCACHE_H

#include "Networking/IP/iptypes.h"
#include "Networking/Ethernet/EthernetPacket.h"

namespace MyOS { namespace Networking { namespace ARP {

using namespace IP;
using Ethernet::ethadr_t;

class ARPDiscovery;

// linear cache of entries
struct ENTRY {
	enum { MAX_CACHE_AGE_MS = 20 * 60 * 1000 };

	inline ENTRY( ipadr_t adr ) : ip(adr) {
		hwAddress.clear();
	}

	void operator delete( void* );

	void setAddress( const ethadr_t& e ) {
		hwAddress.set(e);	
	}

	ipadr_t ip;		// could only store LAN suffix
	ethadr_t hwAddress;
	u16 flags;
	mutable u32 lastUsedAt_ms;
	ARPDiscovery* discovery;	// ongoing discovery, if any
	
	// flags
	enum { E_VALID = 0x1, E_STATIC = 0x2 };

	inline bool isValid() const  { return flags & E_VALID; }
	inline bool isStatic() const { return flags & E_STATIC; }

	/**
	 * @param nowMs		: Current time in ms since startup
	 * @param timeoutMs	: Timeout of ARP entries in ms
	 * @return true iff this entry is still valid
	 */
	inline bool isValid( u32 nowMs, u32 timeoutMs ) const {
		return isValid() && ((lastUsedAt_ms+timeoutMs) >= nowMs);
	}


	inline void invalidate() { flags &= ~E_VALID; }

	// const ethadr_t& getTarget() const { return * (ethadr_t*) hwAddress; }

	// Hash key
	struct KEY {
		ipadr_t dest;
		
		KEY( ipadr_t d ) : dest(d) {
		}

		// TODO: hash based on part of dest IP (IP&~subnetmask)		
		inline u32 hash() const { 
			return dest.getLSB();	// using last 8 bits
		}

		inline bool matches( const ENTRY& entry ) const {
			return dest == entry.ip;
		}
	};
	
	inline KEY getKey() const { return KEY(ip); }
	
} /*PACKED*/;

}}} // namespace

#endif
