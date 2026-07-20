/**
 * A slightly more efficient solution for the floppy controller code
 * 
 * I currently have a 'FloppyThread', put all requests in a queue, and process
 * them in this thread. This seems unnescessary, it is more efficient to start
 * next requests in the IRQ handler directly
 */
#ifndef FLOPPYFSM_H
#define FLOPPYFSM_H

#include "MM/BitBuddyAllocator.hpp"
#include "FDC.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "MultiThreading/IMultiThreading.h"

#include "Atomic/Queue2.h"
#include "DMA/DMAController.h"

#include "Devices/IORequest.h"
#include "MM/new.h"

#include "Timer/ITiming.h"


namespace MyOS { namespace Devices { namespace Floppy {

using namespace MultiThreading;
using namespace InterruptHandling;
using DMA::DMAController;

/// State machine logic for the floppy driver
/**
 * See the UML model for design details
 */
class FloppyFSM : public IInterruptContext, public Timer::ITimerTarget
{
public:
	FloppyFSM() {}
	bool init() INITSECTION;
	void deinit();

   /**
    * Will only work in *synchronous* contexts when allocating thread memory...
    */
   void addRequest( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h );

   inline void releaseBuffer( buffer& buf ) { 
      releaseBuf( ((u32) buf.getData() - (u32) dmaBuffer), buf.getSize() );
   }
   inline void releasePhysicalBuffer( MM::physadr_t b, size_t blockcount ) {
      releaseBuf( b - dmaPhysical, blockcount * 512 );
   }

private:
   void processCurRq();
   void determineNext();
   void releaseBuf( u32 offset, size_t bytes );

   // Timer callback
   virtual void onTimer(  );
   
   // IInterruptContext
   virtual void onInterrupt( );
   
   Queue requests;   // atomic queue of pending (currently only read) requests

   FDC fdc;
   DMAController dma;
  
   // Max. 128KB = 1<<5 * 1<<12, in units of 1<<9 = 512 bytes
	typedef MM::BitBuddyAllocator< 17, 9 > MEMMAP;
	MEMMAP* bufmap;    // on stack, OK ? NO, for some reason need dynamically instead!

	u8* dmaBuffer;
	MM::physadr_t dmaPhysical;
	IORequest* curRq;

	Timer::timer_t timer;
   
	enum {
		S_INITIALIZING,   //< During FDC detection
		S_IDLE,           //< Waiting for requests
		S_WAIT_MOTOR,     //< Waiting for motor to spin up
		S_READY,          //< IDLE, but motor spinning
		S_SEEKING,        //< Waiting for seek to complete
		S_READING,        //< Waiting for read to finish
		S_DELAY_MOTOROFF, //< Wait for new requests before spinning down motor          
	} state : 8;    // valid FSM states
   
};

}}}   // namespace

#endif
