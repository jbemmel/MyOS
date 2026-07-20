//
//	Taken from linux/include/if_ether.h
//	
//	JVB: I really only need a few of these: ETH_P_IP, 	perhaps ETH_P_ARP
//
//	See also http://www.iana.org/assignments/ethernet-numbers

#ifndef ETHERPROTOS_H
#define ETHERPROTOS_H

namespace MyOS { namespace Networking { namespace Ethernet {
/**
 *	These are the defined Ethernet Protocol ID's. (*not* stored in NW order !)
 *
 *  Interpretation: < 0x600 -> IEEE802.3 length, else these IDs
 */

enum ETHERNET_PROTOCOL_ID
{
  ETH_P_LOOP	= 0x0060		// Ethernet Loopback packet	
, ETH_P_PUP		= 0x0200		// Xerox PUP packet		
, ETH_P_PUPAT	= 0x0201		// Xerox PUP Addr Trans packet	


, ETH_P_IP			= 0x0800	// Internet Protocol packet	
, ETH_P_X25			= 0x0805	// CCITT X.25			
, ETH_P_ARP			= 0x0806	// Address Resolution packet	
, ETH_P_BPQ			= 0x08FF	// G8BPQ AX.25 Ethernet Packet	[ NOT AN OFFICIALLY REGISTERED ID ]
, ETH_P_IEEEPUP		= 0x0a00	// Xerox IEEE802.3 PUP packet
, ETH_P_IEEEPUPAT	= 0x0a01	// Xerox IEEE802.3 PUP Addr Trans packet
, ETH_P_DEC			= 0x6000  // DEC Assigned proto
, ETH_P_DNA_DL		= 0x6001  // DEC DNA Dump/Load
, ETH_P_DNA_RC    	= 0x6002  // DEC DNA Remote Console
, ETH_P_DNA_RT    	= 0x6003  // DEC DNA Routing
, ETH_P_LAT       	= 0x6004  // DEC LAT
, ETH_P_DIAG		= 0x6005  // DEC Diagnostics
, ETH_P_CUST    	= 0x6006  // DEC Customer use
, ETH_P_SCA     	= 0x6007  // DEC Systems Comms Arch
, ETH_P_RARP    	= 0x8035	// Reverse Addr Res packet	
, ETH_P_ATALK		= 0x809B	// Appletalk DDP
, ETH_P_AARP		= 0x80F3	// Appletalk AARP
, ETH_P_IPX		   	= 0x8137	// IPX over DIX
, ETH_P_IPV6		= 0x86DD	// IPv6 over bluebook
, ETH_P_PPP_DISC	= 0x8863	// PPPoE discovery messages
, ETH_P_PPP_SES		= 0x8864	// PPPoE session messages
, ETH_P_ATMMPOA		= 0x884c	// MultiProtocol Over ATM
, ETH_P_ATMFATE		= 0x8884	// Frame-based ATM Transport oE

/// JVB added: VLAN 802.1p/q packets. After this are 2 bytes with VLAN ID & priority, then length
, ETH_P_802_1_Q					= 0x8100	// the "TPID" field

// Non DIX types. Won't clash for 1500 types (because <40 bytes is not allowed?)

,  ETH_P_802_3	    = 0x0001		// Dummy type for 802.3 frames
,  ETH_P_AX25	    	= 0x0002		// Dummy protocol id for AX.25
,  ETH_P_ALL	    	= 0x0003		// Every packet (be careful!!!)
,  ETH_P_802_2	    = 0x0004		// 802.2 frames 		
,  ETH_P_SNAP	    	= 0x0005		// Internal only		
,  ETH_P_DDCMP      = 0x0006    // DEC DDCMP: Internal only
,  ETH_P_WAN_PPP    = 0x0007    // Dummy type for WAN PPP frames
,  ETH_P_PPP_MP     = 0x0008    // Dummy type for PPP MP frames
,  ETH_P_LOCALTALK  = 0x0009		// Localtalk pseudo type 	
,  ETH_P_PPPTALK		= 0x0010		// Dummy type for Atalk over PPP
,  ETH_P_TR_802_2		= 0x0011		// 802.2 frames 		
,  ETH_P_MOBITEX		= 0x0015		// Mobitex (kaz@cafe.net)	
,  ETH_P_CONTROL		= 0x0016		// Card specific control frames
,  ETH_P_IRDA	    	= 0x0017		// Linux-IrDA			
,  ETH_P_ECONET	    = 0x0018		// Acorn Econet			

};

}}} // namespace

#endif	// ETHER_H
