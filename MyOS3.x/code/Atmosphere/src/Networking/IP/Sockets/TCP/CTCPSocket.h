/***************************************************************************
                          C_TCPSocket.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

	An implementation of the TCP protocol, as specified in RFC 793
	[ http://www.ibiblio.org/pub/docs/rfc/rfc793.txt ]

	Perhaps in the future make a generic C_IPSocket, and derive both UDP and TCP...

 ***************************************************************************/
#ifndef CTCPSOCKET_H
#define CTCPSOCKET_H

#include "ITCPSocket.h"
#include "ITiming.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

class ITCPState;
class TCPHeader;
class CTCPServerSocket;

using Timer::ITimer;

/**
 * @todo http://www.e-gerbil.net/ras/projects/dos/dos.txt
 *       suggests to use MSS-option filtering to prevent common SYN floods ?
 *       (also 0-ACK filtering, but this both seems very specific)
 */
class CTCPSocket : public ITCPSocket, public Timer::ITimerTarget
{
	// Used for queue of pending connections and list of accepted ones
	CTCPSocket* next;

	// ITCPSocket:: packet reception function
	virtual void onPacket( IPPacket& packet );
	
	// ITimerTarget
	virtual void onTimer( ITimer & ) throw();
	
	ITCPSocketClient *client;
	ITimer &timer;

public:
	CTCPSocket( IIPEndpoint& ep, ITCPSocketClient* client );
	// ~CTCPSocket();
	//void* operator new( size_t );
	//void operator delete( void* );

	// SocketBase
	virtual myos_result_t open( u32 options ) { return E_MYOS_E_FAIL; }  // todo
	virtual myos_result_t close();
	virtual myos_result_t destroy();

   // ITCPSocket
   virtual myos_result_t connect( const sockadr_t& peer, u32 options );
   virtual myos_result_t send( const buffer& data, sendflags_t flags );
   virtual myos_result_t flush();

private:
	enum { USERPORTSPACE=1024 };		// start of portnumbers for user sockets

	// called by CServerSocket when a connect is received (client callback still unknown)
	CTCPSocket( IIPEndpoint &ep, IPPacket &connect, CTCPServerSocket& server );
	void init();

	/// internal
	ITCPSocketClient* getClient() const { return client; }

	// Called by CTCPServerSocket
	bool accept( ITCPSocketClient *cl );

private:

	/// operations used by state classes
	friend class ITCPState;
	friend class TCPListen;
	friend class TCPEstablished;
	friend class TCPSynSent;
	friend class TCPSynReceived;
	friend class TCPClosed;
	friend class TCPClosing;
	friend class TCPCloseWait;
	friend class TCPTimeWait;
	friend class TCPFinWait1;
	friend class TCPFinWait2;

	inline void changeState( ITCPState *s ) { state=s; }
	static inline bool inWindow( u32 start, u32 end, u32 seq );
	bool isAcceptable( u32 seq, size_t segsize );

	// TODO: Merge some of these parameters
	bool sendTCPPacket( TCPHeader &h, size_t tcppayload, bool finReceived, u32 flags=0 );
	void sendReset( TCPHeader &h );	// used by states
	bool sendSYN( bool activeConnect );
	bool sendFIN( bool finReceived );

	// Some routines used by the various states
	void receiveRST();
	void receiveSYN( TCPHeader &syn );
	bool receiveACK( TCPHeader &ack );
	void receiveData( TCPHeader &segment, size_t ippayload );
	inline void receiveFIN() { if (client) client->onRemoteClosed(*this); }

	// Options from TCPHeader
	inline void setRemoteSegmentSize( u32 segsize ) {
      remoteSegmentSize=segsize;
   }
	inline void initSequences( u32 local, u32 remote ) {
		initialSeq = maxSeqSent = una = local;
		nextSEQExpected = remote;
	}

	inline u32 getNextSEQExpected() const { return nextSEQExpected; }
	inline u32 getMaxSequenceSent() const { return maxSeqSent; }
	inline u32 getUna() const { return una; }

	void appendSegment( TCPHeader* sent );
	void cleanupACKed( u32 peerACK );

	friend class CTCPServerSocket;
	friend class CTCPSocketFactory;

	inline void* operator new( size_t );
	inline void operator delete( void* ) throw();
	
	// optional
	CTCPServerSocket* server;

	ITCPState *state;

	/// TCP option fields (per connection)
	u32 remoteSegmentSize, nextSEQExpected;
	u32 maxSeqSent, una;
	u32 localWindowSize;
	u32 initialSeq;		// needed a.o. for a check in TCPSynSent
	u32 WL1, WL2;		// WL1==SEQ that updated win, WL2==ACK that updated win

	// request-ACK-reply-FIN optimization: combine these three segments into 1

	TCPHeader* delayed;		// ptr to delayed reply content
	size_t delayedPayload;	// size of delayed reply content

	// linked list of sent segments that have not been ACKed yet
	struct PacketListEntry {
		
		//inline void* operator new( size_t );
		//inline void operator delete( void* ) throw();
		
		inline PacketListEntry( TCPHeader* seg ) throw() : next(0), segment(seg) {}
		
		bool isACKed( u32 peerACK ) const;
		
		PacketListEntry* next;
		TCPHeader* segment;
	};
		
	PacketListEntry* unacked_head;
	PacketListEntry* unacked_tail;

	bool inOnPacket;		// flag set when in onPacket, i.e. received a packet
	u16 remoteWindow;
	
	ipport_t choosePort(ipport_t clientchoice);
	static ipport_t nextchoice;
};

// Determines if a received segment is in the receive window,
// should be in I_TCPState ?
inline /* static */
bool
CTCPSocket::inWindow( u32 start, u32 end, u32 seq )
{
	// Could use ASM with jump_on_overflow (jo) when adding nextACK+winsize
	return ((seq>=start) && (seq<=end))
	/* rare wrap */ || ((start>end) && ((seq<end) || (seq>=start)) );
}

}}}}	// namespace

#endif
