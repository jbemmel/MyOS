/***************************************************************************
                          UDPHeader.h  -  description
                             -------------------
    begin                : Sun Sep 2 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#ifndef UDPHEADER_H
#define UDPHEADER_H

#include "types.h"

namespace MyOS { namespace Networking {	namespace IP {

// exported in Headers for now, must be internal later
struct UDPHeader {

    // @note: checksum(0) means *disabled*
    inline UDPHeader( u16 src, u16 dest )
        : checksum(0) 
    {
    	srcport   = src;
    	destport  = dest;
    	UDPlength = 0;
    }

    inline u16 getSourcePort() const;
    inline u16 getDestinationPort() const;
    inline u16 getPayloadSize() const;         // in octets, excluding UDP header

    inline size_t wrapUp( size_t payload );

    template <class H>
    inline H* getData() const;

private:
    nw16_t srcport;     /// Both in network order (!)
    nw16_t destport;
    nw16_t UDPlength;   /// size of packet in octets, including UDP header
    u16 checksum;
} PACKED;

inline u16
UDPHeader::getSourcePort() const        { return srcport.hostorder(); }

inline u16
UDPHeader::getDestinationPort() const   { return destport.hostorder(); }

inline u16
UDPHeader::getPayloadSize() const       { return UDPlength.hostorder() - sizeof(UDPHeader); }

inline size_t
UDPHeader::wrapUp( size_t payload ) {
    UDPlength = ( payload += sizeof(UDPHeader) );
    // could calc checksum here too
    return payload;
}

template <class H>
inline H* UDPHeader::getData() const
{
    return (H*) ((u32)this + sizeof(UDPHeader));
}

}}} // namespace

#endif
