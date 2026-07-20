#include "FloppyFSM.h"
#include "Drivers/Floppy/Default/FloppyDriver.h"

#include "TimerFacility/ITimer.h"
#include "../IORequest.h"

#include "debug.h"

namespace MyOS { namespace Devices { 

/*
inline void*
IORequest::operator new( size_t s )
{
	return MyOS::Drivers::Floppy::Default::allocateNoThrow( s );   
}

inline void
IORequest::operator delete( void* r )
{
   MyOS::Drivers::Floppy::Default::deallocate( r, sizeof(IORequest) );   
}
*/
  
namespace Floppy {

using MyOS::Drivers::Floppy::Default::FloppyDriver;

using namespace DMA;

FloppyFSM::FloppyFSM()
: timer( FloppyDriver::getInstance().iTiming->createTimer(*this) )
{
    
}

bool
FloppyFSM::init() // INITSECTION
{
   state = S_INITIALIZING;
   
   dma.init();

   // Allocate 1<<7 * 4 = 512KB of DMA buffer, max (this class must obey 64K lim)
   // ...but impl limit is 32 bits = 1<<5 :(
   dmaBuffer = (u8*) FloppyDriver::getInstance().iVirtualMemory->allocISADMA( 5, dmaPhysical );  
   DPRINTK( "\nDMA buffer @ %x dmaPhysical=%x", dmaBuffer, dmaPhysical );

   // For some reason, allocation as a direct member does not work
   void* mem = MyOS::Drivers::Floppy::Default::allocate( sizeof(MEMMAP) );
   bufmap = new ( mem ) MEMMAP();
   bufmap->init();
   bufmap->free( 1, 8 );    // numbered 0..128KB/512, in 512 byte offsets
   bufmap->allocate(3);     // Save BIOS page 0
   bufmap->initDone();      // need to clear the 'present' bit on use!

   // Dont overwrite BIOS page 0 (first page of dmaBuffer...)
   if (fdc.detect()) {
      DPRINTK( "\nFloppy detected, FDC.init and FSM to IDLE state..." );
      fdc.initialize();
      state = S_IDLE;
      return true;         
   }
   return false;
}

void
FloppyFSM::deinit()
{
   // XX Wait for / abort any pending requests, set state == S_ENDING
   
    timer.free();

    FloppyDriver::getInstance().iVirtualMemory->freeISADMA( dmaBuffer, 5 );

    MyOS::Drivers::Floppy::Default::deallocate( bufmap, sizeof(MEMMAP) );
    bufmap = 0;
}

/**
 * Will only work in *synchronous* contexts when allocating thread memory...
 */
void 
FloppyFSM::addRequest( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h ) 
{
   DPRINTK( "\nFloppyFSM::addRq lbn=%u", lbnStart );
   // NOT_IN_IRQ;    // if we use THREAD memory here

   IORequest* rq = new IORequest( lbnStart, count, seriesCount, h );
   switch (state)
   {
   case S_DELAY_MOTOROFF:
      DPRINTK( "\nCancel timer" );
      timer.cancel();
      // goto S_READY, no break
      
   case S_READY:      
      curRq = rq;
      processCurRq();
      break;

   case S_IDLE:
      state = S_WAIT_MOTOR;
      fdc.startMotor( 0 );
      curRq = rq;	
      // wait 500 ms
      timer.start( 500 * 1000 );
      break;

   default:
      requests.enqueue( *rq );   
   }
}

// virtual
void
FloppyFSM::onTimer( ITimer &  )
{
	switch (this->state)
	{
	case S_WAIT_MOTOR:
      DPRINTK( "\nMotor running" );
      this->state = S_READY;
      this->processCurRq();
      break;
      
   case S_DELAY_MOTOROFF:
      DPRINTK( "\nMotor timer fired, stopping floppy motor..." );
      this->state = S_IDLE;
      this->fdc.stopMotors();
      break;
      
   default:
      DPRINTK( "\nUnexpected state in MotorTimer::onTimer: %u", this->state );      
   }      
}

void
FloppyFSM::processCurRq()
{
   ASSERTION( curRq, E_ERROR );
   
   // TODO: Check if SEEK is really needed, for now always assume YES
   if (true) {
      state = S_SEEKING;
      fdc.seek( curRq->getStartBlock() );     
   }
}

// virtual
void
FloppyFSM::onInterrupt( )
{
   ASSERTION( curRq, E_ERROR );
   // DPRINTK( "\nonIRQ state=%u", state );
   
   u32 id;
   MM::physadr_t dmaAdr;
      
   // TODO: Error checking
   switch (state)
   {
   case S_SEEKING:
      if (fdc.senseIrq()) {
        // DPRINTK( "\nFloppy:seekResult len=%X ST0=%X cyl=%X", len, status.st0, status.st1 );
        // TODO: Verify cylinder now OK?
      } else {
         DPRINTK( "\nFloppy: Failed to issue sense command" );
      }         

      // Could allocate earlier...
      if (!curRq->isContinuation()) {
         id = bufmap->allocate( curRq->getLog2Count() ) & ~1;
         // DPRINTK( "\nGot offset %x order=%d blocks=%d", id, curRequest->getLog2Count(), curRequest->getCount() );
         dmaAdr = dmaPhysical + id;
         curRq->setDMAAddress( dmaAdr );
         // DPRINTK( "\nDMA address is %x", dmaAdr ); while (1);
      } else {
         dmaAdr = curRq->getDMAOffset(512);  
      }         
      id = curRq->getStartBlock();
      state = S_READING;
      break;
   
   case S_READING:
   {
      // process result of previous request
      FDCStatus status;
      /* u8 length = */ fdc.readStatus( status );
      if (status.isOK()) {
         //DPRINTK( "\n..read %x rq#=%u cyl=%X head=%X sec=%X resultlength=%X", 
         //   curRequest, curRequest->getStartBlock(), status.track, status.head, status.sector, length );
         CHECKPOINT('r');
      } else {
         DPRINTK( "\nRead Failed" );

         // Free request, signal onError
         curRq->onError();  // XX Retry first
         id = 0;
         dmaAdr = curRq->getDMAAddress() - dmaPhysical;
         goto release;         
      }  

      if (curRq->done()) {
         // reuse 'id' and 'dmaAdr' variables
         dmaAdr = curRq->getDMAAddress() - dmaPhysical;
         id = curRq->onRead( buffer( &dmaBuffer[ dmaAdr + curRq->getPartIndex() * 512 ], curRq->getCount() * 512 ) );
         if ((id&IOCompletionHandler::E_KEEPBUFFER)==0) {
release:
            bufmap->free( dmaAdr | 1, curRq->getLog2Count() );
         }
         if ((id&IOCompletionHandler::E_REISSUE)==0) {
            delete curRq;
            determineNext();   // try next
         } else {
            // not done yet, reissue (and dont delete...)
            curRq->reissued();

            // To ensure fairness and prevent starvation, put it back in line            
            if (!requests.isEmpty()) {
               requests.enqueue( *curRq );   // reissue, modified
               determineNext();   // try next, guaranteed != 0
            } // else continue processing (could need SEEK!)
         }
       
         // SEEK here if needed
         if (curRq) {
            processCurRq();
            return;   // TODO: Not needed if not always seeking
         } else {
			state = S_DELAY_MOTOROFF;
			DPRINTK( "\nScheduling motor_off..." );   
			timer.start( 3 * 1000 * 1000 ); // 3 seconds
			return;
         }           
      }         

      // prepare for next if not done      
      id = curRq->nextBlock();
      dmaAdr = curRq->getDMAOffset(512);   
      break;
   }   
   
   default:
      DPRINTK( "\nUnexpected state in onInterrupt: %u", state );
      return;
   }

   // When code comes here, continue reading

   // For now, read in 512-byte quantities only
   dma.startDMATransfer(
      DMA_CHANNEL_2 | DMA_TYPE_SINGLE | DMA_TRANSFERMODE_WRITE,
      /* curRequest->getCount()* */ 512,
      // Should be below 1Mb physical, not crossing 64KB mark
      dmaAdr
   );

   // Program data rate(!) Found this in example fdc code
   // controller.setDataRate(E_500Kbps);

   // For now, assume 1.44M
   if (fdc.readBlock( id ) == false) {
      DPRINTK( "\nreadCommand failed!" );
   }
}

/// Sets 'curRq' to the next request to process, 0 if none
void
FloppyFSM::determineNext()
{
   // TODO: Can optimize order of multiple pending requests here if needed...
   // For now, simply always choose next one
   
   curRq = (IORequest*) requests.dequeue();   
}

void 
FloppyFSM::releaseBuf( u32 offset, size_t bytes )
{
   // DPRINTK( "\nFloppy::releaseBuf o=%x bytes=%d", offset, bytes ); 
 
   // XX duplicate calculation
   for (u32 o=0, b=1<<9; o<(32-9); ++o, b<<=1) {
      if (b>=bytes) {
         bufmap->free( offset | 1, o );
         return;
      }
   }    
   BUG( "\nCould not determine order of buffer" ); 
}

   
}}}   // namespace
