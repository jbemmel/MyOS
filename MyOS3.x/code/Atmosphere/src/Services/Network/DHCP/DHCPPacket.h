/***************************************************************************
                          DHCPPacket.h  -  description
                             -------------------
    begin                : Tue Sep 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef DHCPHEADER_H
#define DHCPHEADER_H

#include "types.h"
#include "iptypes.h"
#include "Networking/Ethernet/EthernetPacket.h"

namespace MyOS { namespace Services { namespace Network { namespace DHCP {

	using namespace Networking::IP;	

/**
 * Some common DHCP constants
 */
enum {
   E_DHCP_SERVER_PORT = 67,   ///< UDP port number for DHCP servers
   E_DHCP_CLIENT_PORT = 68,   ///< UDP port number for DHCP clients

   // Implementation limits
   E_DHCP_MAX_HOSTNAME_LEN  = 63,   ///< Max string length (excl 0) of hostname
   E_DHCP_MAX_DOMAIN_LEN    = 63,   ///< Max string length (excl 0) of domainname
};

// required option in every DHCP packet
enum E_DHCP_MSGTYPE_VALUES {
   E_DHCP_INVALID  = 0,
   E_DHCP_DISCOVER = 1, // mind endianess
   E_DHCP_OFFER    = 2,
   E_DHCP_REQUEST  = 3,
   E_DHCP_DECLINE  = 4,
   E_DHCP_ACK      = 5,
   E_DHCP_NAK      = 6,
   E_DHCP_RELEASE  = 7,
   E_DHCP_INFORM   = 8,
};

// Structure for passing the most commonly used DHCP options
struct dhcpOptions_t {
   E_DHCP_MSGTYPE_VALUES packetType;
   ipadr_t assignedIP;
   ipadr_t subnetMask;
   ipadr_t router;
   ipadr_t primaryDNSServer;
   ipadr_t backupDNSServer;
   ipadr_t serverIp;
   u32 leaseTime; // in seconds relative to client time, 0xFFFFFFFF means infinite
   u32 renewTime;
   u32 rebindTime;
   char hostname[E_DHCP_MAX_HOSTNAME_LEN+1];  // copied from packet
   char domainname[E_DHCP_MAX_DOMAIN_LEN+1];

   // empty constructor
   inline dhcpOptions_t() throw() {}
};

using Networking::Ethernet::ethadr_t;

/**
 * Defines the format of a DHCP packet as defined by RFC 2131
 * DHCP essentially reuses the BOOTP protocol format for backward compliance
 *
 * The general common message exchange is DISCOVER->OFFER->REQUEST->ACK
 * Client packets are broadcast
 */
class DHCPPacket
{
   enum {
      E_BOOT_REQUEST = 1,
      E_BOOT_REPLY   = 2,
   } opcode : 8;
   Networking::ARP::ARP_HARDWARE hwAddressType : 8;
   u8 hwAddressLen;
   u8 hwOptions;
   u32 transactionId;         // should be random number, so no nw order needed
   nw16_t secondsSinceBoot;
   nw16_t flags;  // bit 0 == BROADCAST

   // RFC 2131: Client IP address; only filled in if client is in
   // BOUND, RENEW or REBINDING state and can respond to ARP requests.
   ipadr_t clientIp;

   // Receives the assigned ip adres, used as reply address too
   ipadr_t yourClientIp;

   // RFC 2131: IP address of next server to use in bootstrap;
   // returned in DHCPOFFER, DHCPACK by server.
   ipadr_t serverIp;
   ipadr_t relayAgentIp;
   u8 clientHwAddress[16];
   char serverHostname[64];   // optional
   char bootFilename[128];    // optional

   // Start of vendor-specific area, first 4 bytes is magic cookie	

   enum {
      E_MAGIC_OPTION_COOKIE_LE = (0x63<<24) | (0x53<<16) | (0x82<<8) | (0x63)
   };
   u32 magicOptionValue;   // bytes 99, 130, 83, 99, see RFC 1497

   // after this come options, potentially 312 bytes according to RFC
   u8 optionbytes[60];

public:
   /**
    * Creates a dhcp packet (discover) for ethernet
    * Must clear memory when constructed on stack!
    */
   DHCPPacket( const ethadr_t& localAddress, E_DHCP_MSGTYPE_VALUES type );

   /**
    * Creates a dhcp packet (request) for ethernet
    * Must clear memory when constructed on stack!
    */
   DHCPPacket( const DHCPPacket& offer, E_DHCP_MSGTYPE_VALUES type );

   /**
    * Changes the request into a release packet. Still need eth address?
    */
   void makeRelease();

   /**
    * Fills in the option field for client Id, required by @home
    * @param id - 0-terminated identifier (name), max. len 252
    */
   void addClientIdOption( const char* id );

   /**
    * Fills in the option field for server id, required for REQUEST
    * @param id - serverId from OFFER
    */
   void addServerIdOption( ipadr_t id );

   void addRequestedIPOption( ipadr_t ip );

   /**
    * Prepares packet for transmission, sets transactionId
    */
   size_t wrapUp();

   // Reception methods
   bool isReply() const { return opcode==E_BOOT_REPLY; }

   bool parseOptions( const u8* packetLimit, dhcpOptions_t& results ) const;
   ipadr_t getAssignedIP() const { return yourClientIp; }

   // used for release
   void setClientIp( ipadr_t ip ) { clientIp=ip; }

	// "Client sets this flag when it does not know its own IP address"
	// What is meant, is that by setting this flag the client requests MAC
	// layer broadcast instead of unicast
	inline void setBroadcast() { flags = 0x8000; }

private:
   void setDHCPType( E_DHCP_MSGTYPE_VALUES t );
   void* operator new( size_t, void* m ) { return m; }
} /* PACKED */;

}}}} // namespace

#endif
