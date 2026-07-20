/***************************************************************************
                          ICMP.h  -  description
                             -------------------
    begin                : Thu Jun 27 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef ICMP_H
#define ICMP_H

#include "types.h"

namespace MyOS { namespace Networking {	namespace IP {

// The ICMP protocol as defined in RFC 792 http://sparky.freesoft.org/CIE/RFC/792/4.htm

// Protocol code: 1

// type codes
enum E_ICMP_TYPE {
   E_TYPE_ECHO_REPLY         = 0    << 8,    // "ping reply"
   E_TYPE_HOST_UNREACHABLE   = 3    << 8,
   E_TYPE_SOURCE_QUENCH      = 4    << 8,
   E_TYPE_REDIRECT           = 5    << 8,
   E_TYPE_ECHO               = 8    << 8,    // "ping"
   E_TYPE_TIME_EXCEEDED      = 11   << 8,
   E_TYPE_PARAM_PROBLEM      = 12   << 8,
   E_TYPE_TIMESTAMP          = 13   << 8,
   E_TYPE_TIMESTAMP_REPLY    = 14   << 8,
   E_TYPE_INFO               = 15   << 8,
   E_TYPE_INFO_REPLY         = 16   << 8,
};

/// Header of ICMP packets
struct ICMPHeader
{
   // in network order...
   enum E_TYPE_CODE {
      E_NET_UNREACHABLE       = E_TYPE_HOST_UNREACHABLE | 0,
      E_HOST_UNREACHABLE      = E_TYPE_HOST_UNREACHABLE | 1,
      E_PROTOCOL_UNREACHABLE  = E_TYPE_HOST_UNREACHABLE | 2,
      E_PORT_UNREACHABLE      = E_TYPE_HOST_UNREACHABLE | 3,
      E_FRAG_NEEDED           = E_TYPE_HOST_UNREACHABLE | 4,
      E_SOURCEROUTE_FAILED    = E_TYPE_HOST_UNREACHABLE | 5,

      E_TTL_EXCEEDED          = E_TYPE_TIME_EXCEEDED | 0,
      E_REASSTIME_EXCEEDED    = E_TYPE_TIME_EXCEEDED | 1,

      E_NET_REDIRECT          = E_TYPE_REDIRECT | 0,
      E_HOST_REDIRECT         = E_TYPE_REDIRECT | 1,
      E_TOS_NET_REDIRECT      = E_TYPE_REDIRECT | 2,
      E_TOS_HOST_REDIRECT     = E_TYPE_REDIRECT | 3,

      E_PARAM_PTR_VALID       = E_TYPE_PARAM_PROBLEM | 0,
      E_BACKPRESSURE          = E_TYPE_SOURCE_QUENCH | 0,

      E_PING                  = E_TYPE_ECHO        | 0,
      E_PING_REPLY            = E_TYPE_ECHO_REPLY  | 0,

      E_TS                    = E_TYPE_TIMESTAMP         | 0,
      E_TS_REPLY              = E_TYPE_TIMESTAMP_REPLY   | 0,

      E_INFO                  = E_TYPE_INFO        | 0,
      E_INFO_REPLY            = E_TYPE_INFO_REPLY  | 0,
   } typecode : 16;  // note : nworder!
   u16 checksum;
   union {
      u8 ptr;                 // only valid for parameter problem
      u32 unused;
      // IPADDRESS gateway;      // for redirect
      struct {
         u16 id;
         u16 seq;
      } ping;                 // for ping or timestamp
   } v;
   // IPPacket originalPacket;   // may contain options -> variable size...
   u64 someOriginalData;
};

}}} // namespace

#endif
