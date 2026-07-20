#ifndef IPPACKET_H
#define IPPACKET_H

#include "iptypes.h" // E_IPPROTOCOL

namespace MyOS { namespace Networking { namespace IP {

/**
 *	This class represents an IP packet header as specified by RFC791
 */
class IPPacket
{
	// To set an (more or less) unique ID in each packet sent
	static u16 sUniqueID;
	
public:
	// little-endian !
	enum FLAGS {
		/// bit0 reserved
		IP_DONT_FRAGMENT    = 0x0040,
		IP_MORE_FRAGMENTS   = 0x0020,
		IP_FRAGOFFSETBITS   = 0xFF8F,
	};

	enum { MAXPAYLOAD = 0xFFFF };

	// High nibble in first byte of the header
	enum VERSION { IPv4 = 0x40, IPv6 = 0x60 };


	/**
	 * IPPacket Constructor
	 * Note that for incoming packets that are reused this method is not called!
	 * This means that values for TTL and TOS are then basically undefined
	 */
	inline IPPacket( u16 maxPayload, u16 id = 0 );

	/// Gets
	inline E_IPPROTOCOL getProtocol() const	{ return protocol; }
	inline u8 getTOSbits() const            { return TOS; }
	inline u8 getTTL() const                { return TTL; }
	inline ipadr_t getSource() const		{ return source; }
	inline ipadr_t getDestination() const	{ return destination; }

	inline bool hasMoreFragments() const	{ 
		return flagsOffset&IP_MORE_FRAGMENTS;
	}

	inline size_t getTotalSize() const;
	inline size_t getPayloadSize() const;
	inline bool isFirstFragment() const { return (flagsOffset&IP_FRAGOFFSETBITS)==0; }
	inline bool checkVersion( VERSION expected ) const { return (ver_hsize&0xF0)==expected; }

	inline u16 getFragmentOffset() const { return htons16(flagsOffset&IP_FRAGOFFSETBITS); }

	template <class H>
	inline H& getData() const;

	/// Tweaking sets
	inline void setTOSbits( u8 tos ) { TOS = tos; }
	inline void setTTL( u8 ttl )     { TTL = ttl; }
	inline void dontFragment()       { flagsOffset = IP_DONT_FRAGMENT; }

	void wrapUp( ipadr_t from, ipadr_t to, size_t payload, E_IPPROTOCOL p );

	// Note: This clears the checksum field in the process...
	bool verifyChecksum();

	void debugDump( const char* byteformat = "%X" );

private:    // the rest is nobody's business
    u16 ip_fast_csum();

    u8 ver_hsize;			///< IPv4(4) or IPv6(6), hsize in 32bit words, minimum is 5
    u8 TOS;                 ///< @see TOSBITS
    nw16_t totalLength;     ///< in octets, including IP header and data
    nw16_t identification;  ///< "any identifying value to aid in reassembly"
    u16 flagsOffset;        ///< 3 flag bits, 13 offset bits @see FLAGS
    u8 TTL;
    E_IPPROTOCOL protocol : 8;   /// @see E_IPPROTOCOL
    u16 headerChecksum;          /// Calculated in little-endian, so no swap needed

    ipadr_t source;              /// @see Networking::IP::ipadr_t
    ipadr_t destination;

    /// There may be optional fields: security, routing, ...
    /// They are aligned to 32 bits
} PACKED;

/// According to the IP protocol specs, the ID field should be choosable by
/// upper layers to allow proper retransmission of packets
inline IPPacket::IPPacket( u16 maxPayload, u16 id /* = 0 */ )
    : ver_hsize( IPv4 | 5 ), TOS(0), TTL(0x40), source(0), destination(0)
{
	totalLength = maxPayload+sizeof(IPPacket);
	identification = id;
}

template <class H>
inline H& IPPacket::getData() const
{
	/// Assumes no option fields !
	return * (H*) ((u32)this+sizeof(IPPacket));
}

inline size_t
IPPacket::getPayloadSize() const  {
    return totalLength.hostorder() - ((ver_hsize&0x0F) << 2);
}

inline size_t
IPPacket::getTotalSize() const {
    return totalLength.hostorder();
}

}}}  // namespace

#endif
