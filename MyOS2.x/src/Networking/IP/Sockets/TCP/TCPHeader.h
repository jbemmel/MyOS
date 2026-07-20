/***************************************************************************
                          TCPHeader.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TCPHEADER_H
#define TCPHEADER_H

#include "types.h"
#include "Networking/IP/iptypes.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

using namespace Networking;

/// Option bits, use with LE optionfield(!)
enum TCPOPTIONFLAGS {		
	TCP_URG 	= 0x2000,
	TCP_ACK 	= 0x1000,
	TCP_PUSH  	= 0x0800,
	TCP_RESET 	= 0x0400,
	TCP_SYN		= 0x0200,
	TCP_FIN		= 0x0100,

	FLAGMASK	= 0x3F00,
	HSIZEMASK 	= 0x00F0
};

/**
 *	TCPHeader objects are constructed in 2 ways: reusing an incoming TCP packet, and
 *	constructing a new one on top of an IP packet
 */
class TCPHeader {

public:
	/* Should only be used for new, outgoing TCP headers
	inline TCPHeader( u16 src, u16 dest, u32 flags, u8 hsizedwords=5 )
		: srcport( src ), destport( dest ), window(1460), checksum(0)
	{
  	hsizeoptions = flags | (hsizedwords<<4);
	}
	*/

	// inline new
	// void* operator new( size_t, void* m ) { return m; }

	// all other parameters are set just before sending...
	inline TCPHeader( u32 flags, u8 hsizedwords=5 )
	{
		hsizeoptions = flags | (hsizedwords<<4);
	}

	inline TCPHeader& init( u32 flags, u8 hsizedwords=5 ) {
		hsizeoptions = flags | (hsizedwords<<4);
		return *this;
	}

	inline ipport_t getSourcePort() const;
	inline ipport_t getDestinationPort() const;
	inline u16 getHSizeBytes() const;

	u32 getSeq() const 			{ return seqnumber.hostorder(); }
	u32 getACK() const			{ return acknumber.hostorder(); }
	u16 getWindow() const		{ return window.hostorder(); }

	inline bool isUrgent() const 	{ return hsizeoptions&TCP_URG; }
	inline bool hasACK() const 		{ return hsizeoptions&TCP_ACK; }
	inline bool isPush() const		{ return hsizeoptions&TCP_PUSH; }
	inline bool isReset() const		{ return hsizeoptions&TCP_RESET; }
	inline bool isSYN() const		{ return hsizeoptions&TCP_SYN; }
	inline bool isFinish() const	{ return hsizeoptions&TCP_FIN; }
	inline u16 getFlags() const		{ return hsizeoptions&FLAGMASK; }

	/// Reusing the incoming packet...
	inline void setFlags( u16 flags ) {
		hsizeoptions = (hsizeoptions&~FLAGMASK) | flags;
	}

	inline void setPorts( u16 local, u16 remote ) {
		srcport  = local;
      	destport = remote;
	}

	/// Sets the header size (in bytes, must be multiple of 4)
	inline void setHSizeBytes( u8 sizeInBytes ) {
		hsizeoptions = (hsizeoptions & ~HSIZEMASK) | (sizeInBytes<<2);
	}

	inline void setACK( u32 ack ) 	{ acknumber = ack; }
	inline void setSeq( u32 seq ) 	{ seqnumber = seq; }
	inline void setWindow( u16 w ) 	{ window    = w;   }

	/// Parsing of additional options
	struct TCPOPTIONS {
		u32 maxsegsize;
		u32 timestamp;
		u8 windowscale;		
	};

	enum OPTIONFIELDS {
		TCP_MSS			= 0x1,
		TCP_TIMESTAMP	= 0x2,
		TCP_WINDOWSCALE = 0x10,

		// error conditions
		E_TCP_INVALID_OPTION_LENGTH   = 0x80000000,
	};

	u32 parseOptions( TCPOPTIONS &result ) const;
	bool verifyChecksum( ipadr_t from, ipadr_t to, size_t totaloctets );

	/// Simultaneous check for multiple
	inline bool flagsSet( u32 flags ) { return hsizeoptions&flags; }

	inline size_t wrapUp( ipadr_t from, ipadr_t to, size_t ippayload );

	template <class H>
	inline H* getData() const;

	// Creates a RESET pakket in response to this TCPHeader, reusing the packet
	size_t createReset();

private:
	void calcChecksum( ipadr_t from, ipadr_t to, size_t totalOctets );

	nw16_t srcport;     /// Both in network order (!)
	nw16_t destport;
	nw32_t seqnumber;	/// seq. number of first octet in segment
	nw32_t acknumber;	
	u16 hsizeoptions;	/// note: mind byteorder!
	nw16_t window;
	u16 checksum;
	nw16_t urgentptr;	/// positive offset from sequence number

	/// Several options may follow
} PACKED;

inline u16
TCPHeader::getSourcePort() const        { return srcport.hostorder(); }

inline u16
TCPHeader::getDestinationPort() const   { return destport.hostorder(); }

inline u16	// in bytes, 4 most significant bits (little endian)
TCPHeader::getHSizeBytes() const   			{ return (hsizeoptions&HSIZEMASK)>>2; }

template <class H>
inline H* TCPHeader::getData() const
{
    return (H*) (((u32)this) + getHSizeBytes());
}

inline size_t
TCPHeader::wrapUp( ipadr_t from, ipadr_t to, size_t totalpacketsize )
{
	calcChecksum(from,to,totalpacketsize);
	return totalpacketsize;
}

}}}} // namespace

#endif
