#ifndef ACTIVEGATHERBUFFER_H
#define ACTIVEGATHERBUFFER_H

#include "myosresult.h"
#include "Atomic/AtomicQueue.h"
#include "MultiThreading/IMultiThreading.h"
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

   Request( EthernetPacket& p, size_t s, u32 f ) 
   	: packet(p), size(s), flags(f) {}

   Request( const Request& r ) 
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
class ActiveGatherBuffer : public IRunnable, public Devices::IPacketSender
{
	class RQNode : public AtomicQueue::Node {
		Request rq;	// copy! of incoming rq
		// ICompletionHandler& handler;

		friend class ActiveGatherBuffer;
		RQNode( const Request& r ) : rq(r) {}
		const Request& getRequest() const { return rq; }
		// ICompletionHandler& getHandler() const { return handler; }
		
		inline void* operator new( size_t );
		inline void operator delete( void* );
	};

   // should be more general, but for starters...
	IEthernetDevice &device;	// could implement interface including 'addRequest', also by AGB
	AtomicQueue requestQueue;
	Thread* processor;

   // needed for destructor
   inline void operator delete( void* ) {}

public:
	typedef void* REQUESTHANDLE;

	ActiveGatherBuffer( IEthernetDevice& target, size_t maxQueueSize );
	virtual ~ActiveGatherBuffer();

   /**
    * Starts the async buffer thread
    * Easier to have a separate method, else data members need to create a 
    * thread in initializer...
    */
   bool start( Thread& runner );

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


private:
	// IRunnable
	virtual int run( NVPAIR params[] );


	void onComplete( void* request, myos_result_t result );

	// called by driver
	virtual void onPacketSent( const buffer& data, void* act );
	virtual void onPacketAborted( const buffer& data, void* act );

	bool needWakeup;
};


}}

#endif
