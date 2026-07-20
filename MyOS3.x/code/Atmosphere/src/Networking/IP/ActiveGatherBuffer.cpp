#include "ActiveGatherBuffer.h"
#include "IThread.h"

#include "IPComponent.h"	// for new/delete
#include "IIPEndpoint.h"	// E_SEND_PACKET_REUSED

#include "debug.h"

#define VERBOSE

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
	Networking::IP::deallocate( p /*, sizeof(ActiveGatherBuffer::RQNode)*/ );
}


ActiveGatherBuffer::ActiveGatherBuffer( IEthernetDevice& target, size_t maxQueueSize )
: device(target), processor(0), needWakeup(false)
{
    processor = &Networking::IP::IPComponent::getInstance().iMultiThreading->createThread( *this, 0, 0 );
}

/*
ActiveGatherBuffer::~ActiveGatherBuffer()
{
   // what ? cancel pending requests ?
}
*/

void
ActiveGatherBuffer::stop()
{
   // device.removeTXClient( *this );
}

ActiveGatherBuffer::REQUESTHANDLE
ActiveGatherBuffer::addRequest( const Request& rq )
{
    ASSERTION( processor, E_ERROR );
    
	// !! Need to copy rq, it will be destroyed upon exit of this method !!
	RQNode* h = new RQNode(rq);
#ifdef VERBOSE	
	DPRINTK( "\naddRequest bytes=%d node=%x", rq.size, h );
#endif	
	// rq.packet.debugDump(rq.size);
	requestQueue.enqueue( h );	// or fixed size buffer...
	if (needWakeup) {
		needWakeup = false;		// to prevent multiple wakeup calls
#ifdef VERBOSE		
		DPRINTK( "Sending wakeup() call" );
#endif		
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

   BUG( "cancelRequest not working yet" );

   /* first check if it is in the queue, since the node may already have been deleted
   if (requestQueue.contains(*rq)) {

      // XX AtomicQueue currently has no operation to remove arbitrary nodes !
      BUG( "cancelRequest not working yet" );
   }*/
   return 0;
}


// virtual
int
ActiveGatherBuffer::run( NVPAIR params[] )
{
	DPRINTK( "\nActiveGatherBuffer thread started, sleeping..." );

	needWakeup = true;	
	while ( 1 )
	{
	    IThread::do_suspend();
	    
#ifdef VERBOSE			
		DPRINTK( "\nbufferthread awakened!" );
#endif		
		needWakeup = false;

		RQNode* rq;
		u32 sentSuccesfully = 0;
		while ((rq = (RQNode*) requestQueue.dequeue()))
		{
#ifdef VERBOSE				
			DPRINTK( "\nAGB:Transmitting rq=%x packet=%x size=%d", 
			        rq, &rq->getRequest().packet, rq->getRequest().size );
#endif			
			// rq->getRequest().packet.debugDump(rq->getRequest().size);
			Networking::Packet *packet = new Networking::Packet( 
			        (u8*) &rq->getRequest().packet, rq->getRequest().size );
			packet->setToken( rq );
retry:
			myos_result_t r = device.sendPacket( *packet, 0 );   // no flags defined yet
			// this, rq );	// TODO need to pass rq as ACT
			
			if (r != E_MYOS_SUCCESS) {
			   DPRINTK( "#4f#..##ERROR## : %d", r );
			}

            // on link down, hardcoded policy to wait several ticks
            if (r == (myos_result_t) IEthernetDevice::LINK_DOWN) {
               DPRINTK( "\nLink down, sleeping about 1 second..." );
               IThread::do_usSleep( 1000000, 50 );
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
		DPRINTK( "\nAGB going back to suspend..." );
#endif		
	}

	DPRINTK( "\nActiveGatherBuffer thread ending..." );
	return 0;
}

// virtual
void
ActiveGatherBuffer::onPacketSent( Networking::Packet &p )
{
   /*
    * Problems:
    * 1) no efficient way to link packet to original request
    * 2) no return value to send next packet
    * ...
    */
   onComplete( p.getToken(), E_MYOS_SUCCESS );
   p.free();
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
		size_t bytes = max( node->getRequest().size, 46 ) + sizeof(EthernetPacket);	
		Networking::IP::deallocate( (u8*) &node->getRequest().packet /*, bytes */ );
	}

	delete node;

	// Could signal upper node here (used to do that) but now only process here
	if ( requestQueue.getSize() && needWakeup ) {
		processor->wakeup();	// ALT: for-loop over queue, tx from here ?	
	}
}

}}	// namespace
