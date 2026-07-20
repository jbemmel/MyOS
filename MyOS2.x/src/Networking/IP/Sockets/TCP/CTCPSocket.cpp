/***************************************************************************
                          CTCPSocket.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

Currently, some code is here and some in the TCP state(s) -> clean up

 ***************************************************************************/
#include "CTCPSocket.h"
#include "MM/new.h"
#include "TCPHeader.h"
#include "CTCPServerSocket.h"
#include "States/TCPClosed.h"
#include "States/TCPSynSent.h"
#include "States/TCPListen.h"

#include "Networking/IP/IPPacket.h"
#include "Services/Network/TCP/TCPService.h"
#include "Timer/ITiming.h"
#include "Timer/TSC.h"  	// for randomness, TODO: Something else...

#include "debug.h"

using namespace MyOS::Networking::IP::TCP;

// Need this one! For some reason...
// inline void operator delete( void* s ) {
//	MyOS::Services::Network::TCP::operator delete( s, sizeof(CTCPSocket) );
//}

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

using namespace MyOS::Services::Network::TCP;
	
/// Choose ports starting at 1024 (very small chance at duplication)
ipport_t CTCPSocket::nextchoice = USERPORTSPACE;

/**
 * 	Constructor used for actively opened sockets
 */
CTCPSocket::CTCPSocket( IIPEndpoint &ep, ITCPSocketClient *cl )
	: ITCPSocket(ep,0)
    , client(cl), server(0), state( TCPClosed::instance() )
    , initialSeq(0)
    , unacked_head(0), unacked_tail(0)
{
	this->init();
}

/**
 * 	Constructor used for passively opened sockets (through ServerSocket.accept())
 */
CTCPSocket::CTCPSocket( IIPEndpoint &ep, IPPacket &connect, CTCPServerSocket& serv )
	: ITCPSocket(ep,connect.getData<TCPHeader>().getDestinationPort())
    , client(0), server(&serv), state( TCPListen::instance() )
    , unacked_head(0), unacked_tail(0)
{
	this->init();
	
	TCPHeader &tcph = connect.getData<TCPHeader>();
	setRemoteAddress( connect.getSource(), tcph.getSourcePort() );
	state->receive( *this, tcph, connect.getPayloadSize() );
}

void*
CTCPSocket::operator new( size_t )
{
	return allocate( sizeof(CTCPSocket) );	
}


void 
CTCPSocket::operator delete( void* p )
{
	deallocate( p, sizeof(CTCPSocket) );	
}

/**
 * Common initialization
 */
void
CTCPSocket::init()
{
	// protocol defaults
	remoteSegmentSize = 536;	// see RFC 879, default
	localWindowSize   = 1460;	// ???
	
	// implementation fields, could simply zero the whole thing first...
	next = 0;
	inOnPacket = false;
	delayed = 0;
	
	TCPService::getInstance().iTiming->initTimer( timer, *this );
}

/* not virtual, should be called via destroy()...
CTCPSocket::~CTCPSocket()
{
	// don't do this, state machine should not allow code to come here before
	// close() has been called anyway
	// close();
}
*/

ipport_t CTCPSocket::choosePort(ipport_t clientchoice)
{
	if (clientchoice==0) {
		clientchoice = nextchoice;
		/// When nextchoice wraps, there is a slight chance a duplicate port is chosen
		if (++nextchoice<USERPORTSPACE) nextchoice=USERPORTSPACE;
	}

	/// check and iterate using choosePort()
	setLocalPort(clientchoice);
	return clientchoice;
}

// Complete sequence started in the above constructor
bool
CTCPSocket::accept( ITCPSocketClient *cl )
{
	client = cl;

	ASSERTION( getRemoteAddress().isValid() , E_ERROR );

	// now add the TCP socket to the hashtable, check if succes
	if (getEndpoint().addSocket( *this ) == E_MYOS_SUCCESS) {
		// use the cached connect, future: TCPListen should generate a packet, also for retransmission...
		return TCPListen::instance()->accept( *this );
	}
	DPRINTK( "\naccept():Failed to add socket to hashtable, probably retransmitted SYN..." );


	return false;		// C_ServerSocket will destroy the socket, TODO: call client?
}

bool
CTCPSocket::isAcceptable( u32 seq, size_t segsize )
{
	// TCP spec p.26, segsize includes SYN&FIN flags
	if (localWindowSize) {
		if (inWindow(nextSEQExpected,nextSEQExpected+localWindowSize,seq)) return true;
		else return (segsize &&
			inWindow(nextSEQExpected,nextSEQExpected+localWindowSize,seq+segsize-1));
	} else return (segsize==0);
}

/* virtual */
myos_result_t
CTCPSocket::destroy()
{
	DPRINTK( "\nCTCPSocket::destroy remote=%x", getRemoteAddress().host() );
	// TCPService::getInstance().iTiming->cancelTimer( timer ); do this ?
	getEndpoint().removeSocket( *this );

	cleanupACKed( 0xFFFFFFFF );

	// Remove socket from server's accepted queue, if there
	if (server) server->onSocketClose( *this );

	TCPService::getInstance().iTiming->freeTimer( timer );
	
	MyOS::Services::Network::TCP::deallocate( this, sizeof(CTCPSocket) );
	return E_MYOS_SUCCESS;
}

// virtual
myos_result_t
CTCPSocket::connect( const sockadr_t& peer, u32 options )
{
	if (peer.isValid()) {
		return state->connect( *this, peer, options );
	} else return E_MYOS_E_FAIL;
}

// virtual
myos_result_t
CTCPSocket::send( const buffer& data, sendflags_t flags )
{
	// XXX flags not used yet!
	
	// Note: this should be buffered and gathered and retransmitted if needed...
	IPPacket *p = getEndpoint().createPacket( data.getSize()+sizeof(TCPHeader) );
	if (p) {
		TCPHeader &tcph = p->getData<TCPHeader>();
		memcpy( (&tcph)+1, data.getData(), data.getSize() );
		s32 result;		
		if ( result = state->send( *this, tcph, data.getSize() ) ) {
			maxSeqSent += data.getSize();
			
			// Need to keep this segment until ACKed, for retransmission!
			appendSegment( &tcph );
		
			// TODO: Start a retransmission timer if not already running
			// TCPService::getInstance().iTiming->startTimer( timer, 2000 * 1000 );
		} else {
			DPRINTK( "\nNot success but: %x", result );	
		}
		return (myos_result_t) result;   // XX todo: change s32 into myos_result_t
	} else return E_MYOS_E_FAIL;
}

// virtual
myos_result_t
CTCPSocket::flush()
{
   // todo
	DPRINTK( "\nCTCPSocket::flush not implemented" );   
	return E_MYOS_E_NOTIMPL;
}


// virtual
myos_result_t
CTCPSocket::close()
{
	return state->close(*this);
}

// private, internal
void
CTCPSocket::appendSegment( TCPHeader* sent )
{
	PacketListEntry* entry = new PacketListEntry( sent );
	if (unacked_head) {
		unacked_tail->next = entry;
	} else {
		unacked_head = entry;
	}
	unacked_tail = entry;
}

// virtual
void
CTCPSocket::onPacket( IPPacket &pkt )
{
	size_t ippayload = pkt.getPayloadSize();
	TCPHeader &tcp = pkt.getData<TCPHeader>();
	DPRINTK( "\n[%d] TCPrx ACK=%b SYN=%b RES=%b FIN=%b seq#=%x ack#=%x len=%u",
		(u32) TCPService::getInstance().iTiming->now( Timer::ms ),
		tcp.hasACK(), tcp.isSYN(), tcp.isReset(), tcp.isFinish(),
		tcp.getSeq(), tcp.getACK(), ippayload - tcp.getHSizeBytes() );

	// DPRINTK( "\nTCPHdr:%x Data:%x", &tcp, tcp.getData<u8>() );
	//pkt.debugDump( "%c" );	// Here, it's still OK

	if (tcp.verifyChecksum(pkt.getSource(), pkt.getDestination(), ippayload)) {

		// Calculate segsize including SYN+FIN flags (can both be set?)
		// This code should be in all states, but it is so similar for all...
		size_t seglength 	= ippayload - tcp.getHSizeBytes() 
							+ tcp.isSYN() + tcp.isFinish();

		// SynSent: special case, TCP p. 37
		if (isAcceptable( tcp.getSeq(), seglength ) || (state==TCPSynSent::instance())) {
			// pkt.debugDump( "%c" );	// Here, it's still OK
			state->receive( *this, tcp, ippayload );
		} else {
			DPRINTK( "..segment not acceptable(win=%x nextSEQExpected=%x", localWindowSize, nextSEQExpected );
			if (!tcp.isReset()) {
				
				// if any unacked data, sent that again (select best match wrt peer ACK)
				// clean up ACKed data (?)
				if (tcp.hasACK()) cleanupACKed( tcp.getACK() );

				if (unacked_head==0) {
					tcp.init( TCP_ACK );
					sendTCPPacket( tcp, 0, false, E_SEND_PACKET_REUSED ); // ACK it, no data
				} else {
					DPRINTK( " retransmitting data" );
					TCPHeader *reply = unacked_head->segment;
					IPPacket *ip = (((IPPacket*) reply)-1);
					myos_result_t r = getEndpoint().send( *ip, getRemoteAddress(), ip->getPayloadSize(), E_IPPROTO_TCP, E_SEND_PACKET_REUSED );
				}
				
			}
		}
	} else DPRINTK( "..Invalid TCPChecksum" );
}

// virtual
void
CTCPSocket::onTimer()
{
	state->onTimer( *this );
}

// not virtual, private
bool
CTCPSocket::sendTCPPacket( TCPHeader &h, size_t tcppayload, bool finReceived, u32 flags )
{
	size_t ippayload = tcppayload + h.getHSizeBytes();

	h.setPorts( getLocalPort(), getRemotePort() );
	h.setWindow( localWindowSize );		   // ?? size ??
	h.setSeq( maxSeqSent );
	h.setACK( nextSEQExpected + finReceived  );   // dont increment for each FIN rx'd

	DPRINTK( "\n[%d] TCPtx ACK=%b SYN=%b RES=%b FIN=%b seq#=%x ack#=%x len=%u",
		(u32) TCPService::getInstance().iTiming->now( Timer::ms ),
		h.hasACK(), h.isSYN(), h.isReset(), h.isFinish(),
		h.getSeq(), h.getACK(), tcppayload );

	h.wrapUp( getLocalAddress(), getRemoteAddress(), ippayload );

	// assuming there is an IPHeader...
	IPPacket *ip = (((IPPacket*) &h)-1);
	myos_result_t r = getEndpoint().send( *ip, getRemoteAddress(), ippayload, E_IPPROTO_TCP, flags  );
	// DPRINTK( "\n#1f#Sending TCP packet @%x...", &h );
	if (r == E_MYOS_SUCCESS || r == E_MYOS_EINPROGRESS) {
		// maxSeqSent += tcppayload;		// advance local seq: in senddata

		// Need to keep this segment until ACKed, for retransmission!
		// appendSegment( ip ); but not here, in sendData
		
		// TODO: Start a retransmission timer if not already running
		// TCPService::getInstance().iTiming->startTimer( timer, 2000 * 1000 );
		
		// And endpoint should not destroy it!

		return true;
	} else {
		DPRINTK( "\n#3f#sendTCPPacket: send failed!" );
		
		// TODO: free the packet then!
		
		return false;	// error
	}
}

// used by states
void
CTCPSocket::sendReset( TCPHeader &h )
{
	IPPacket *ip = ((IPPacket*) &h) -1;
	size_t payload = h.createReset();
	getEndpoint().send( *ip, getRemoteAddress(), payload, E_IPPROTO_TCP, E_SEND_PACKET_REUSED );
}


bool	// TO DO: share code with other methods ?
CTCPSocket::sendSYN( bool activeConnect /* , IPPacket& incoming -> reuse? */ )
{
	// create a 20 byte TCP header, no options
	IPPacket *packet = getEndpoint().createPacket( sizeof(TCPHeader) );

	// Set initial values, if needed for active connect
	u32 flags = TCP_SYN;
	if (activeConnect) {
		initialSeq = maxSeqSent = Timer::TSC::read() ^ 1958345871;
		una = initialSeq+1;
	} else flags |= TCP_ACK;
	bool result = sendTCPPacket( packet->getData<TCPHeader>().init(flags), 0, false, 0 );

	/// SYN flag takes up 1 byte in sequence space
	++maxSeqSent;
	return result;
}


bool	// TO DO: share code with other methods
CTCPSocket::sendFIN( bool finReceived )
{
	// create a 20 byte TCP header, no options
	IPPacket *packet = getEndpoint().createPacket( sizeof(TCPHeader) );

	bool result = sendTCPPacket( packet->getData<TCPHeader>().init( TCP_FIN | TCP_ACK ), 
									0, finReceived, 0 );

	// this must be executed at most once!
	++maxSeqSent;  // make sure ACK is accepted, and proper seq is reported
	return result;
}

void
CTCPSocket::receiveRST()
{
	// Combined steps 2&4
	// send reset responses to local, TODO: flush queues, cancel timers
	if (client) client->onConnectionReset(*this);
	this->destroy();	// go the the 'CLOSED' state...
}

void
CTCPSocket::receiveSYN( TCPHeader &syn )
{
	// First send reset, then do processing as in receiveRST
	this->sendReset( syn );
	receiveRST();
}

inline bool
CTCPSocket::PacketListEntry::isACKed( u32 peerACK ) const
{
	// XXX Disregarding wrapover after 2^32 here
	size_t lastSeq = segment->getSeq() + (((IPPacket*) segment)-1)->getPayloadSize() - sizeof(TCPHeader);
	DPRINTK( " lastSeq=%x", lastSeq );
	return lastSeq <= peerACK;
}

void 
CTCPSocket::cleanupACKed( u32 ack )
{
	DPRINTK( "\ncleanupACKed ack=%x head=%x", ack, unacked_head );
	for (PacketListEntry *e = unacked_head; e; ) {			
		if (e->isACKed(ack)) {
			DPRINTK( "\nCleanup ACKed segment" );
			unacked_head = e->next;
			getEndpoint().destroyPacket( ((IPPacket*) e->segment)-1 );
			MyOS::Services::Network::TCP::deallocate( e, sizeof(PacketListEntry) );
			e = unacked_head;
		} else e = e->next;
	}	
}

bool
CTCPSocket::receiveACK( TCPHeader &p )
{
	u32 ack = p.getACK();

	// RFC 1122: SEG.ACK <= SND.UNA
	if (inWindow(una,maxSeqSent,ack))  {
		// Update acknowledged segments, this may cancel some retransmission timers
		una = ack;
		cleanupACKed( ack );		
		
		// update send window
		if (WL1<p.getSeq()||(WL1==p.getSeq()&&WL2<ack)) {	// TO DO: mod 2^32
			remoteWindow = p.getWindow();
			WL1 = p.getSeq();
			WL2 = ack;
		}
	} else if (ack>=una) {	// RFC793 says '<' but I think that's wrong
		// ignore duplicate
		DPRINTK( "Ignoring duplicate ACK #%u", p.getACK() );
	} else {	// not yet sent
		DPRINTK( "Got future ACK #%u expected #%u", ack, una );
		sendTCPPacket( p.init(TCP_ACK), 0, p.isFinish(), E_SEND_PACKET_REUSED );
		return false;		// abort further processing (drop the segment)
	}
	return true;	
}

// private, called by states
void 
CTCPSocket::receiveData( TCPHeader &segment, size_t ippayload )
{
	size_t tcppayload = ippayload - segment.getHSizeBytes();
	if ((tcppayload>0) || segment.isFinish() ) {
		// DPRINTK( "\nTCPEstablished::receiveData size=%u", tcppayload );
		//IPPacket* p = ((IPPacket*) &tcppacket) - 1;
		//p->debugDump( "%c" );	// OK

		/// send ACK (before delivery...TODO: delay this until after onRecv!
		nextSEQExpected += tcppayload;
		
		// delay this until after client callback, combine if possible
		// s.sendTCPPacket( tcppacket.init(TCP_ACK), 0, tcppacket.isFinish(), E_SEND_PACKET_REUSED );
		if (tcppayload>0) {
			if (client) {
				DPRINTK( "..client::onRecv" );
				const buffer b(segment.getData<u8>(),tcppayload);
				inOnPacket = true;
				ITCPState* cur = state;
				client->onRecv( *this, b );		// may do sendData, close
				inOnPacket = false;
				
				// Check for state change -> implies closed
				u32 flags = (state==cur) ? TCP_ACK : (TCP_ACK|TCP_FIN);
				if (delayed) {				
					sendTCPPacket( 
						delayed->init(flags), 
						delayedPayload, 
						segment.isFinish(), 
						0 
					);					
					delayed = 0;
				} else {
					sendTCPPacket( 
						segment.init(flags), 
						tcppayload, 
						segment.isFinish(), 
						E_SEND_PACKET_REUSED 
					);
				}				
				if (flags&TCP_FIN) ++maxSeqSent;
			} else {
				// else error
				BUG( "client==0" );
			}
		} else {
			sendTCPPacket( segment.init(TCP_ACK), 0, segment.isFinish(), E_SEND_PACKET_REUSED );
		}
	}	
}



}}}}	// namespace
