#ifndef ACTIVEGATHERBUFFER_H
#define ACTIVEGATHERBUFFER_H

#include "myosresult.h"
#include "Atomic/AtomicQueue2.h"
#include "IMultiThreading.h"
#include "Networking/Ethernet/EthernetPacket.h"
#include "Devices/Network/IEthernetDevice.h"

namespace MyOS {
namespace ASync {

using namespace MyOS::MultiThreading;
using Devices::Network::IEthernetDevice;
using MyOS::Networking::Ethernet::EthernetPacket;

/**
 * Class used as request (for now)
 */
struct Request {
   EthernetPacket& packet;
   size_t size;
   u32 flags;	// can be E_SEND_PACKET_REUSED

   inline Request( EthernetPacket& p, size_t s, u32 f ) throw() 
   	: packet(p), size(s), flags(f) {}

   inline Request( const Request& r ) throw() 
   	: packet(r.packet), size(r.size), flags(r.flags) {}
};

/**
 * This class buffers requests from many sources to a single bottleneck, for async processing
 * The sink can be async if required, in which case it should use the same ICompletionHandler
 * callback mechanism as this class uses for its clients
 *
 * For now, made specific to ethernet packet handling...
 * 
 * TODO: OS should leave threading / event strategy to applications where possible?
 */
class ActiveGatherBuffer : public IRunnable // , public Devices::IPacketSender
{
	class RQNode : public MyOS::AtomicQueue2::Node {
		Request rq;	// copy! of incoming rq
		// ICompletionHandler& handler;

		friend class ActiveGatherBuffer;
		inline RQNode( const Request& r ) throw() : rq(r) {}
		inline const Request& getRequest() const { return rq; }
		// ICompletionHandler& getHandler() const { return handler; }
		
		inline void* operator new( size_t );
		inline void operator delete( void* );
	};

   // should be more general, but for starters...
	IEthernetDevice &device;	// could implement interface including 'addRequest', also by AGB
	AtomicQueue2 requestQueue;
	IThread* processor;

   // needed for destructor
   // inline void operator delete( void* ) {}

public:
	typedef void* REQUESTHANDLE;

	ActiveGatherBuffer( IEthernetDevice& target, size_t maxQueueSize );
	// ~ActiveGatherBuffer();

	void stop();

	/**
	 * Adds a request to the queue for async processing
	 *
	 * @param request	 : Request to queue (treated as opaque)
	 * @param completion : Completion handler, called upon result (to free resources)
	 *                     This reference should remain valid until either it has been called,
	 *                     or the request was canceled succesfully
	 *
	 * @return handle for cancel, NULL if failed
	 */
	REQUESTHANDLE addRequest( const Request& rq );

	/**
	 * Cancels a pending request if possible
	 *
	 * @return original request if succeeded (to free resources), NULL if unable to cancel
	 */
	Request* cancelRequest( REQUESTHANDLE h );

    // called by CIPEthernetEndpoint via AdrResolver(for now)
    void onPacketSent( Networking::Packet &packet );
	
private:
	// IRunnable
	virtual int run( NVPAIR params[] );
	virtual void handleSignal( u32 code, void* data ) throw() {}

	
	void onComplete( void* request, myos_result_t result );

	void onPacketAborted( const buffer& data, void* act );

	bool needWakeup;
};


}}

#endif
