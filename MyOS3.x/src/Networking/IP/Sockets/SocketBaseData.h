/*
 *  This file is included directly by SocketBase.h and provides some more 
 *  implementation details that users dont need to deal with
*/
#ifndef SOCKETBASE_H
#error Only include this file via SocketBase.h
#endif

protected:
   // Default values
   enum { DEFAULT_TTL = 0x40, DEFAULT_TOS = TOS_ROUTINE_PRESEDENCE };

   CAbstractSocketBase( IIPEndpoint &ep, E_IPPROTOCOL proto, ipport_t localport )
      : endpoint(ep), key(proto,localport)
      , packetBits( DEFAULT_TTL<<8 | DEFAULT_TOS ) {}

   virtual ~CAbstractSocketBase() = 0;

protected:
   IIPEndpoint& endpoint;

public:
   // all data stored in host order
   struct KEY {

      ipadr_t remote;
      u32 localportProtocol;    // immutable once set
      ipport_t remotePort;

      KEY( E_IPPROTOCOL proto, ipport_t localport ) 
         : remote(0), localportProtocol( proto << 16 | localport ) {}
    
      // hash function used for hashtables, focus on lower bits  (not sure if this is a good hash function)
      inline u32 hash() const	{
		u32 hashv = localportProtocol;	// dont include local IP
		if (remote.isValid()) hashv ^= remote.nw() ^ remotePort;

		return hashv ^ (hashv>>15);
      }

      inline bool matches( const CAbstractSocketBase& o ) const {
         return (o.key.localportProtocol==localportProtocol)
                &&  ( !remote.isValid() || 
                	((o.key.remote==remote)&&(o.key.remotePort==remotePort)));
      }

      ipport_t getPort() const {
         return localportProtocol & 0x0000FFFF;
      }

      // Used to iterate when addition to hashtable fails
      void setPort( ipport_t port ) {
          localportProtocol = (localportProtocol & 0xFFFF0000) | port;
      }

      // used by CIPEndpoint
      void setRemoteAddress( ipadr_t adr, ipport_t port ) {
         remote = adr;
         remotePort = port;
      }
   };

   const KEY& getKey() const { return key; }

   inline const sockadr_t getRemoteSockAddress() const;

   // used by sockets
   void setRemoteAddress( ipadr_t adr, ipport_t port ) {
      key.setRemoteAddress( adr, port );
   }

   // used for iterating over missed hashes
   void setLocalPort( ipport_t port ) {
      key.setPort(port);
   }

protected:
   friend struct KEY;   // needed for some versions of gcc
   KEY key;
   u32 packetBits;      // TOS bits, TTL(<<8)
};

// need to provide dummy
inline CAbstractSocketBase::~CAbstractSocketBase() {}

inline ipport_t CAbstractSocketBase::getLocalPort() const
{ return key.getPort(); }

inline ipport_t CAbstractSocketBase::getRemotePort() const
{ return key.remotePort; }

inline E_IPPROTOCOL CAbstractSocketBase::getProtocol() const  
{ return (E_IPPROTOCOL) (key.localportProtocol>>16); }

inline ipadr_t CAbstractSocketBase::getLocalAddress() const   
{ return endpoint.getLocalAddress(); }

inline ipadr_t CAbstractSocketBase::getRemoteAddress() const 
{ return key.remote; }

inline const sockadr_t CAbstractSocketBase::getRemoteSockAddress() const
{
   return sockadr_t(key.remote,key.remotePort);
}

inline IIPEndpoint& CAbstractSocketBase::getEndpoint() const { return endpoint; }


inline u8 CAbstractSocketBase::getTOSbits() const { return packetBits & 0xFF; }
inline myos_result_t CAbstractSocketBase::setTOSbits( u8 bits )
{
   packetBits = (packetBits & ~0x000000FF) | bits;
   return E_MYOS_SUCCESS;  // for now, always allowed
}

inline u8 CAbstractSocketBase::getTTL() const { return packetBits >> 8; }
inline myos_result_t CAbstractSocketBase::setTTL( u8 hops )
{
   if (hops) {
      packetBits = (packetBits & ~0x0000FF00) | hops;
      return E_MYOS_SUCCESS;  // for now, always allowed
   } else return E_MYOS_EINVAL;
}


/*
inline bool CAbstractSocketBase::getSocketOption( E_SOCK_OPTION option ) const 
{ return socketOptions&option; }

inline void CAbstractSocketBase::enableOption( E_SOCK_OPTION option )          
{ socketOptions |= option; }

inline void CAbstractSocketBase::disableOption( E_SOCK_OPTION option )         
{ socketOptions &= ~option; }
*/


// SocketBase closes file
