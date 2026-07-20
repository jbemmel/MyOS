#include "ActiveGatherBuffer.h"
#include "MultiThreading/IThread.h"

#include "Networking/IP/IPComponent.h"	// for new/delete
#include "Networking/IP/IIPEndPoint.h"	// E_SEND_PACKET_REUSED

#include "debug.h"

// #define VERBOSE

namespace MyOS {
namespace ASync {

inline void*
ActiveGatherBuffer::RQNode::operator new( size_t )
{
	return Networking::IP::allocate( sizeof(ActiveGatherBuffer::RQNode) );
}


inline void
ActiveGatherBuffer::RQNode::operator delete( void *p )
{
	Networking::IP::deallocate( p, sizeof(ActiveGatherBuffer::RQNode) );
}

ActiveGatherBuffer::ActiveGatherBuffer( IEthernetDevice& target, size_t maxQueueSize )
: device(target), processor(0), needWakeup(false)
{

}

ActiveGatherBuffer::~ActiveGatherBuffer()
{
   // what ? cancel pending requests ?
}

bool
ActiveGatherBuffer::start( Thread& runner )
{
	if (processor==0) {
		processor = &runner;
      // device.setTXClient( *this );  moved to other thread
		runner.start( *this, 0, 0 );
		
		// !! Wait until thread has started! Not the most efficient way, but...
		while ( runner.isSleeping() == false ) Thread::do_yield( /* &runner*/ 0 );
		
		return true;
   }
   DPRINTK( "\nFailed to start GatherBuffer thread!" );
   return false;
}

void
ActiveGatherBuffer::stop()
{
   // device.removeTXClient( *this );
}

ActiveGatherBuffer::REQUESTHANDLE
ActiveGatherBuffer::addRequest( const Request& rq )
{
	// !! Need to copy rq, it will be destroyed upon exit of this method !!
	RQNode* h = new RQNode(rq);
#ifdef VERBOSE	
	DPRINTK( "\naddRequest bytes=%d node=%x", rq.size, h );
#endif	
	// rq.packet.debugDump(rq.size);
	requestQueue.enqueue( h );	// or fixed size buffer...
	if (needWakeup) {
		needWakeup = false;		// to prevent multiple wakeup calls
		processor->wakeup();
	} 
#ifdef VERBOSE		
	else DPRINTK( "..no wakeup" );
#endif	
	return (REQUESTHANDLE) h;
}

Request*
ActiveGatherBuffer::cancelRequest( REQUESTHANDLE h )
{
   RQNode* rq = (RQNode*) h;

   // first check if it is in the queue, since the node may already have been deleted
   if (requestQueue.contains(rq)) {

      // XX AtomicQueue currently has no operation to remove arbitrary nodes !
      BUG( "cancelRequest not working yet" );
   }
   return 0;
}


// virtual
int
ActiveGatherBuffer::run( NVPAIR params[] )
{
	DPRINTK( "\nActiveGatherBuffer thread started, sleeping..." );

	needWakeup = true;
	// 1000 seconds, TODO: should be infinite
	while ( processor->usSleep( 1000000000 )	/* != E_ENDED */ || true )
	{
#ifdef VERBOSE			
		DPRINTK( "\nbufferthread awakened!" );
#endif		
		needWakeup = false;

		RQNode* rq;
		u32 sentSuccesfully = 0;
		while ((rq = (RQNode*) requestQueue.dequeue()))
		{
#ifdef VERBOSE				
			DPRINTK( "\nAGB:Transmitting rq=%x packet=%x size=%d", rq, &rq->getRequest().packet, rq->getRequest().size );
#endif			
			// rq->getRequest().packet.debugDump(rq->getRequest().size);
retry:
			myos_result_t r = device.sendPacket( 
				buffer( (u8*) &rq->getRequest().packet, rq->getRequest().size ),
				0,	// flags, none so far
			    this, rq );	// need to pass rq as ACT
			
			if (r != E_MYOS_SUCCESS) {
			   DPRINTK( "..##ERROR## : %d", r );
			}

            // on link down, hardcoded policy to wait several ticks
            if (r == (myos_result_t) IEthernetDevice::LINK_DOWN) {
               DPRINTK( "\nLink down, sleeping about 1 second..." );
               processor->usSleep( 1000000, 50 );
               goto retry;
            } else if (r & E_MYOS_ERROR) {
				delete rq;	// since we won't get a 'onComplete()' call
				break;
			} else {
#ifdef VERBOSE					
			   DPRINTK( " OK!" );
#endif			   
			   ++sentSuccesfully;
			}
		}

      /**
       * Ideal would be to wait for the next onComplete, and avoid new requests
       * to wake this thread (so needWakeup = false in that case)
       */

		// when packets were sent, we assume to get onComplete
		// needWakeup = (sentSuccesfully==0);
		needWakeup = true;
#ifdef VERBOSE			
		DPRINTK( "\nAGB going back to sleep..." );
#endif		
	}

	DPRINTK( "\nActiveGatherBuffer thread ending..." );
	return 0;
}

// virtual
void
ActiveGatherBuffer::onPacketSent( const buffer& data, void* act )
{
   /*
    * Problems:
    * 1) no efficient way to link packet to original request
    * 2) no return value to send next packet
    * ...
    */
   onComplete( act, E_MYOS_SUCCESS );
}

// virtual
void ActiveGatherBuffer::onPacketAborted( const buffer& data, void* act )
{
   onComplete( act, E_MYOS_ERROR );
}

// not virtual, called from IRQ callbacks above
void
ActiveGatherBuffer::onComplete( void* request, myos_result_t result )
{
#ifdef VERBOSE		
	DPRINTK( "\nonComplete request=%x result=%x", request, result );
#endif	

	// need to put ACT in request
	RQNode* node = (RQNode*) request;

	// NEW: Also free packet here (allocated by CIPEthernetEndpoint, move here?
	if ( (node->getRequest().flags & Networking::IP::E_SEND_PACKET_REUSED) == 0) {
		// Not always convenient, e.g. packet reused
		// DPRINTK( "\ndelete mem=%x", &node->getRequest().packet );

		// Check for ethernet padding, min packet is 46 bytes payload
		size_t bytes = (node->getRequest().size > 46 ? node->getRequest().size : 46) + sizeof(EthernetPacket);	
		Networking::IP::deallocate( (u8*) &node->getRequest().packet, bytes );
	}

	delete node;

	// Could signal upper node here (used to do that) but now only process here
	if ( requestQueue.getSize() && needWakeup ) {
		processor->wakeup();	// ALT: for-loop over queue, tx from here ?	
	}
}

}}	// namespace
