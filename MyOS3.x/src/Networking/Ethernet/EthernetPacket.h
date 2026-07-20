/*
	An EthernetPacket class.
*/

#ifndef ETHERNETPACKET_H
#define ETHERNETPACKET_H

#include "buffer.h"
#include "types.h"
#include "EthernetProtocols.h"

namespace MyOS { namespace Networking { namespace Ethernet {

using namespace Networking;

/// Represents the 6-byte ethernet hw address
struct ethadr_t
{
	u8 bytes[6];

	inline void set( const ethadr_t& adr ) {
		// memcpy( bytes, adr.bytes, sizeof(bytes) );
		*((u32*) bytes) = *((u32*) adr.bytes);
		*((u16*) (bytes+4)) = *((u16*) (adr.bytes+4));
	}

	inline void set( const buffer& adr ) {
		// memcpy( bytes, adr.bytes, sizeof(bytes) );
		*((u32*) bytes)     = * (u32*) adr.getData();
		*((u16*) (bytes+4)) = * (u16*) (adr.getData()+4);
	}

	inline void clear() {
		*((u32*) bytes)     = 0;
		*((u16*) (bytes+4)) = 0;
	}

	inline void set( u8 b0, u8 b1, u8 b2, u8 b3, u8 b4, u8 b5 ) {
		bytes[0] = b0; bytes[1] = b1; bytes[2] = b2;
		bytes[3] = b3; bytes[4] = b4; bytes[5] = b5;
	}

   // see http://www.erg.abdn.ac.uk/users/gorry/course/lan-pages/mac.html
   bool isMultiCastAddress() const { return bytes[0]&0x1; }

   u8& operator[]( u8 index ) { return bytes[index]; }
   const u8& operator[]( u8 index ) const { return bytes[index]; }

   operator buffer() const { return buffer( (u8*) bytes, sizeof(ethadr_t)); }

   inline bool operator ==( const ethadr_t& other ) const {
      // return memcmp( bytes, other.bytes, sizeof(bytes) )==0;
      return *((u32*) bytes) == *((u32*) other.bytes)
         && *((u16*) (bytes+4)) == *((u16*) (other.bytes+4));
   }
   inline bool operator !=( const ethadr_t& other ) const {
      return !( *this == other );
   }
   
   bool isEmpty() const { 
		return (*((u32*) bytes) == 0) && (*((u16*) (bytes+4)) == 0);
   }
   
};

enum E_PACKETFLAGS {
    INVALID         = 0x0,
    BROADCAST_MATCH = 0x1,
    PHYSICAL_MATCH  = 0x2,
    MULTICAST_MATCH = 0x3,

    // not yet used: Some cards can calculate checksums
    UDP_CHECKSUM_OK    = 0x8,
    UDP_CHECKSUM_ERROR = 0x10,
    TCP_CHECKSUM_OK    = 0x20,
    TCP_CHECKSUM_ERROR = 0x40,
};

/// Ethernet packet, must be created in DMA-suitable memory!
/**
 * TODO: Perhaps extend with 'link' field (top), these things are often queued..
 * Not possible: packets are received by NIC, and must be formatted like this
 * However, the 'source' field could be used as link field until the packet is
 * sent!
 */
class EthernetPacket
{
public:
	static const ethadr_t BROADCAST;

	void wrapUp( const ethadr_t& sendTo, u16 protocolOrSize );


	inline void setLocalAddress( const ethadr_t& local )  { source.set(local); }
	inline void setDestination( const ethadr_t& d )       { destination.set(d); }

	/// Buffer should not be modified...
	inline const ethadr_t getSource() const { return source; }

	/// For setting the destination MAC directly
	inline const ethadr_t getDestination() const { return destination; }

	/// Determine which type to use, length by IEEE802.3 <true> or type <false>
	inline bool isValidLength() const;

	// IEEE 802.3 support
	inline u16 getIEEE802_3Size() const;
	inline void setIEEE802_3Size( u16 s );

	// ethernet protocol definitions support
	inline ETHERNET_PROTOCOL_ID getProtocol() const;
	inline void setProtocol( ETHERNET_PROTOCOL_ID );

    // sizes excluded CRC sum
	enum { HEADERSIZE=14, MINFRAMESIZE=60, MTU = 1500, MAXFRAMESIZE=HEADERSIZE+MTU };

	template <class H>
	inline H& getData() const;

	void debugDump( size_t payload );

private:
	// XX inefficiency
	inline EthernetPacket() {}

	// could add size here but not transmit it...

	// start of ethernet header must be 4-byte aligned (RTL8139)
	ethadr_t destination;
	ethadr_t source;		// not set by chip !

	// NOTE: not an union, GCC makes sizeof(union)==4 !!
	nw16_t ieee802_3_size_or_ethernettype;

	// u8 data[ MTU ];
	// u32 crc;		// calculated by driver

} PACKED;

template <class H>
inline H& EthernetPacket::getData() const
{
    return * (H*) ((u32)this+sizeof(EthernetPacket));
}

inline bool
EthernetPacket::isValidLength() const
{
    register u16 size = ieee802_3_size_or_ethernettype.hostorder();
    return ((size>=40) && (size<=0x600));
}

/*  not needed, structure is fixed -> use structs + cast ptr
inline const Buffer::Reader EthernetPacket::createReader() const
{
    // interpret length: valid -> limit readable bytes
    return Buffer::Reader( Buffer::convert( getData<u8>() ),
        isValidLength() ? ieee802_3_size_or_ethernettype.hostorder() : MTU );
}
*/

/* used in o.a. assertion to write raw ethernet packets *
inline const Buffer::Writer EthernetPacket::createWriter() const
{
    /// This reserves room for the ethernet header
    return Buffer::Writer( Buffer( getData<u8>(), MTU ));
}
*/

inline u16 EthernetPacket::getIEEE802_3Size( ) const   {
    return ieee802_3_size_or_ethernettype.hostorder();
}

inline void EthernetPacket::setIEEE802_3Size( u16 s )  {
    ieee802_3_size_or_ethernettype = s;		// auto converted
}

inline ETHERNET_PROTOCOL_ID EthernetPacket::getProtocol() const	{
    return (ETHERNET_PROTOCOL_ID) ieee802_3_size_or_ethernettype.hostorder();
}

inline void EthernetPacket::setProtocol(ETHERNET_PROTOCOL_ID id)	{
    ieee802_3_size_or_ethernettype = id;
}

inline void
EthernetPacket::wrapUp( const ethadr_t& to, u16 protocolOrType )
{
	destination.set(to);

	/// If too small, padding must be used, but packet size remains unchanged !
	/// if (protocolOrType<MINFRAMESIZE) protocolOrType=MINFRAMESIZE;

	ieee802_3_size_or_ethernettype = protocolOrType;
}


}}}	// namespace

#endif
