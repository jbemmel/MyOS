/***************************************************************************
                          iptypes.h  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef IPTYPES_H
#define IPTYPES_H

#include "types.h"

namespace MyOS { 

// fwd
namespace IO { class OStream; }
	
namespace Networking { namespace IP {

typedef u16 ipport_t;     //< TCP or UDP port

/// The well known IPv4 number format
struct ipadr_t {

	union {
		u8 byte[4];
		nw32_t ipnumber;
	} PACKED u;

   inline ipadr_t( const ipadr_t& adr )
   {
       u.ipnumber = adr.u.ipnumber;
   }

   inline ipadr_t( u32 adr_hostOrder = 0 )
   {
       u.ipnumber = adr_hostOrder;
   }

   /// Constructor with dummy bool argument, to read addresses already in network order
   inline ipadr_t( u32 adr_nwOrder, bool NOT_USED )
   {
       u.ipnumber.setValueNetorder( adr_nwOrder );
   }

   inline ipadr_t( u8 b0, u8 b1, u8 b2, u8 b3 )
   {
       u.byte[0]=b0; u.byte[1]=b1; u.byte[2]=b2; u.byte[3]=b3;
   }

	inline const ipadr_t& operator =( const ipadr_t& a )
	{
		u.ipnumber = a.u.ipnumber;
		return *this;
	}

	inline u8& operator[]( u32 index ) { return u.byte[index]; }
	inline const u8& operator[]( u32 index ) const { return u.byte[index]; }

	// Special addresses
	inline static const ipadr_t ANY() 		{ return ipadr_t(0); }
	inline static const ipadr_t LOOPBACK()	{ return ipadr_t(0x7F000001); }
	inline static const ipadr_t BROADCAST()	{ return ipadr_t(0xFFFFFFFF); }

   /// no implicit conversion, is dangerous
   inline u32 host() const	{ return u.ipnumber.hostorder(); }
   inline u32 nw() const	{ return u.ipnumber.netorder(); }

	// @return Least-significant byte
	inline u8 getLSB() const { return u.byte[3]; }

	inline bool operator ==( const ipadr_t &ia ) const { return u.ipnumber == ia.u.ipnumber; }
	inline bool operator !=( const ipadr_t &ia ) const { return !(*this == ia); }

	inline bool isBroadcast() const { return u.ipnumber.netorder() == 0xFFFFFFFF; }

	/** 
	 * Checks if this address matches the Loopback range (127.*.*.*)
	 */
	inline bool isLoopback() const  { return u.byte[0]==127; }

   // matching with masks
   inline bool inSameSubnet( ipadr_t ia, ipadr_t subnetmask ) const
   {
      // Broadcast is always in the same subnet (todo: local too?)
      if ( isBroadcast() || ia.isBroadcast() ) return true;

      u32 sn = subnetmask.nw();
      return (u.ipnumber.netorder() & sn) == (ia.u.ipnumber.netorder() & sn);
   }

   // @return The subnet broadcast address for this ip address
   inline ipadr_t getSubnetBroadcast( ipadr_t subnetmask ) const {
      return ipadr_t( u.ipnumber.netorder() | ~subnetmask.u.ipnumber.netorder(), true );
   }

   inline bool isSubnetBroadcast( ipadr_t subnetmask ) const {
      u32 sn = ~subnetmask.nw();
      return (u.ipnumber.netorder() & sn) == sn;
   }

	inline bool isValid() const { return u.ipnumber.netorder()!=0; }

	// @todo inline bool isMultiCast() const
	
	/// Prints this IP address in dotted form x.x.x.x to the outstream
	void print( IO::OStream& out ) const;
	
	inline void clear() { u.ipnumber.clear(); }
} PACKED;

/// A socket address is IP address + port (TCP or UDP)
struct sockadr_t {

	inline sockadr_t( const ipadr_t &adr, u16 p )
        : ip(adr) { port = p; }

	inline ipport_t getPort() const  { return port.hostorder(); }
	inline ipadr_t getIP() const     { return ip; }

	// check for validness
	inline bool isValid() const { return ip!=0; }
	inline bool operator ==( const sockadr_t& o ) const
	{
		return (ip==o.ip) && (port==o.port);
	}

private:
    ipadr_t ip;
    nw16_t port;    // TCP/UDP port, stored in network order
} /*PACKED*/;

/// 8-bit IP protocol numbers
enum E_IPPROTOCOL {
    E_IPPROTO_IP      = 0,	   ///<  Dummy protocol for TCP
    E_IPPROTO_ICMP    = 1,    ///< Internet Control Message Protocol
    E_IPPROTO_IGMP    = 2,    ///< Internet Group Management Protocol
    E_IPPROTO_IPIP    = 4,	   ///< IPIP tunnels (older KA9Q tunnels use 94)
    E_IPPROTO_TCP     = 6,    ///< Transmission Control Protocol
    E_IPPROTO_EGP     = 8,	   ///< Exterior Gateway Protocol
    E_IPPROTO_PUP     = 12,	///< PUP protocol
    E_IPPROTO_UDP     = 17,	///< User Datagram Protocol
    E_IPPROTO_IDP     = 22,	///< XNS IDP protocol
    E_IPPROTO_IPV6    = 41,	///< IPv6-in-IPv4 tunnelling
    E_IPPROTO_RSVP    = 46,	///< RSVP protocol
    E_IPPROTO_GRE     = 47,	///< Cisco GRE tunnels (rfc 1701,1702)
    E_IPPROTO_ESP     = 50, 	///< Encapsulation Security Payload protocol
    E_IPPROTO_AH      = 51,		///< Authentication Header protocol
    E_IPPROTO_PIM     = 103,	///< Protocol Independent Multicast
    E_IPPROTO_COMP    = 108,	///< Compression Header protocol
    E_IPPROTO_RAW	  = 255,	///< Raw IP packets
};

/// The TypeOfService value is a per-packet 8-bit number with several flags
enum E_TOSBITS {
	// Upper 3 bits are precedence values
    TOS_ROUTINE_PRESEDENCE              = 0x0<<5,
    TOS_PRIORITY_PRESEDENCE             = 0x1<<5,
    TOS_IMMEDIATE_PRESEDENCE            = 0x2<<5,
    TOS_FLASH_PRESEDENCE                = 0x3<<5,
    TOS_FLASHOVERRIDE_PRESEDENCE        = 0x4<<5,
    TOS_CRITICAL_PRESEDENCE             = 0x5<<5,
    TOS_INTERNETWORKCONTROL_PRESEDENCE  = 0x6<<5,
    TOS_NETWORKCONTROL_PRESEDENCE       = 0x7<<5,
    TOS_PRESEDENCE_MASK     			= 0x7<<5, // values 0..7

    // additional bits from RFC795, now obsolete
    // TOS_LOW_DELAY           = 0x8,
    // TOS_HIGH_THROUGHPUT     = 0x10,
    // TOS_HIGH_RELIABILITY    = 0x20
};

}	// end IP namespace

namespace ARP {

	/// Defined types of ARP hw, there are more in linux/if_arp.h
	enum ARP_HARDWARE {
		ARP_ETHER   = 1,
		ARP_IEEE802 = 6
	};

}	// end ARP namespace

}} // namespace

#endif
