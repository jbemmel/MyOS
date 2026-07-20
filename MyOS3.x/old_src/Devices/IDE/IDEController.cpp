//#include "myosconfig.h"
//#ifdef CONFIG_IDE

/**
 * See http://www.stanford.edu/~csapuntz/specs/INF-8020.PDF
 */

#include "IDEController.h"
#include "ATAPIDevice.h"
#include "ATAPI.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace IDE {

// Bits of HD_STATUS
enum HD_STATUS {
   ERR_STAT                = 0x01,
   INDEX_STAT              = 0x02,
   ECC_STAT                = 0x04,    // Corrected error
   DRQ_STAT                = 0x08,
   SEEK_STAT               = 0x10,
   WRERR_STAT              = 0x20,
   READY_STAT              = 0x40,
   BUSY_STAT               = 0x80,
};   

// Values to be programmed for device selection, from ATAIO.H
enum DEV_SELECT {
   E_DEV_MASTER   = 0x00,
   E_DEV_SLAVE    = 0x10,
};

bool
IDEController::init() // INITSECTION
{
   DPRINTK( "\nIDEController::init base=%x", base );   

   state = S_DETECTING;
   
   IDEDriver::getInstance().iInterruptHandling->setIRQHandler( 
      base==PRIMARY ? E_IRQ14 : E_IRQ15, *this, E_NONE );
   writeControl(0);      // enable IRQ by clearing disable-bit, for 14 or 15
      
/*
   // Code below works under bochs, but not for *real* hardware...
   writeControl( 0 );   // enable IRQ, rest reserved
   writeDriveHead( E_DEV_MASTER );
   delay400ns();
   
   // Toggle the bits a bit, this seems random, ok?
   writeSectorCount( 0x55 );
   writeSector( 0xAA );
   writeSectorCount( 0xAA );
   writeSector( 0x55 );
   writeSectorCount( 0x55 );
   writeSector( 0xAA );

   if (readInterruptReason()==0x55 && readSector()==0xAA) {
      
   }

   // XX Should select before reading status?
*/
   reset();

   int c = 100;
   while (readAltStatus() & (BUSY_STAT|DRQ_STAT) )   // NOTE: Regardless of DRDY, p.62   
   {
      if ( --c == 0 ) return true;  // init success, no devices
   }
   writeDriveHead( 0xA0 | 0x40 | 0x00 | 0x00 );
   writeFeature( 0 );      // Disable DMA and overlap
   writeCylinder( 512 );   // bytecount: 256 words
   writeCommand( 0xA1 );   // 'IDENTIFY PACKET device' -> IRQ, 256 words of info
   DPRINTK( "\nIDENTIFY PACKET device sent..." );

   c = 10000;
   while (readAltStatus() & (BUSY_STAT|DRQ_STAT) )   // NOTE: Regardless of DRDY, p.62   
   {
      if ( --c == 0 ) {
         DPRINTK( "timeout, aborting" );
         break;             
      }        
   }
   DPRINTK( "\nTrying SLAVE..." );
   writeDriveHead( 0xA0 | 0x40 | 0x10 | 0x00 );
   writeFeature( 0 );      // Disable DMA and overlap   
   writeCylinder( 512 );   // bytecount: 256 words   
   writeCommand( 0xA1 );   // 'IDENTIFY device' -> IRQ, 256 words of info

   c = 10000;
   while (readAltStatus() & (BUSY_STAT|DRQ_STAT) )
   {
      if ( --c == 0 ) {
         DPRINTK( "timeout, aborting" );
         break;             
      }              
   }

   DPRINTK( "\nController ready" );
   state = S_READY;
   return true;
}

// virtual 
void
IDEController::onInterrupt( )
{
   // Check status bits
   u8 status = readStatus();

   DPRINTK( "\nIDE:IRQ base=%x bytecount=%u IRQreason=%X status=%X err=%X state=%X", 
      base, readCylinder(), readInterruptReason(), status, readError(), state );

   // Filter out command-done IRQs, TODO other ways for this? use it somehow?
   //if (( readInterruptReason() & 3 ) != 3 ) {
   //   // status not valid, ignore
   //   return 0;  
   //}

switch (state) {

case S_DETECTING:
{
   // added checking for more bits, non-present devices return e.g. 0xff
   // See ATA specs http://www.t13.org/docs2003/d1532v1r3g.pdf 6.17.5
   // if ((status & (BUSY_STAT|READY_STAT|WRERR_STAT|DRQ_STAT|ERR_STAT))==DRQ_STAT) {
   
   // However, in practice this does not work: real device returns s==0x08, bochs s==0x58
   // Non-present device returns s==0xff, catch that case with ERR bit
   if ((status & (DRQ_STAT|ERR_STAT)) == DRQ_STAT) {
   
      u16 data[256];    // need to read all??
      u16 l = readData( data, sizeof(data) );
      
      // Perform in-memory swapping here, instead of in subclasses
      // see linux: ide-probe.c, around lines 92
      // XX: Linux also swaps serial number!
      for (u32 w=23; w<=46; ++w) {
         u16 n = data[w];
         data[w] = ((n & 0xFF)<<8) | (n>>8);
      }
            
      DPRINTK( "\nRead %u bytes [0]=%x DRQ_type=%X firmware=%s caps=%x PIO=%X", 
         l, data[0], (data[0]>>5) & 3, &data[23], data[49], data[64] );
      
      // Could also set some flag instead
      if (readDriveHead() & 0x10) {
         slave  = (data[0] & 0x8000) ? new ATAPIDevice( *this, E_DEV_SLAVE, data ) : 0;
      } else {
         master = (data[0] & 0x8000) ? new ATAPIDevice( *this, E_DEV_MASTER, data ) : 0;
      }
   }      
   break;
}

case S_MASTER_SEEKING:

      // XX not sure how to interpret the standard here. SEEK returns completion status
      // immediately (READY bit set, others cleared) but command still running
      // DSC bit (SEEK_STAT bit) will be set when command completes. Does this mean
      // I need to spin here until that bit is set?
      // YES! Section 10.4 explains that no further IRQ is sent, although new
      // commands can now be sent to the other ATA device if required

      // Section 5.6: Device can accept a new command (and executes when able)
      if (( status & (SEEK_STAT) ) || ((readInterruptReason() & 3) == 3)) {
         master->onSeekDone( *curRq );    // assumed to issue 'read' next
      } else {
         DPRINTK( "\nERROR, seek not done -> skip request" );
         // TODO: master->onError()
         doNext();         
      }
      break;
   
case S_MASTER_READING:

      // Cannot put <sectorsize> bytes on the stack, so delegate to device
      if ( status & (1<<3)) {
         master->onReadDone( *curRq );
      } else if ((readInterruptReason() & 3) == 3) {     // completion status
         // CoD and IO bits both set
         doNext();   // *only* after valid data is read         
      }
      break;

case S_SLAVE_SEEKING:
      // Section 5.6: Device can accept a new command (and executes when able)
      if (( status & (SEEK_STAT) ) || ((readInterruptReason() & 3) == 3)) {
         slave->onSeekDone( *curRq );
      } else {
         DPRINTK( "\nERROR, seek not done -> skip request" );
         // TODO: slave->onError()
         doNext(); 
      }      
      break;

case S_SLAVE_READING:
      if ( status & (1<<3)) {
         slave->onReadDone( *curRq );
      } else if ((readInterruptReason() & 3) == 3) {     // completion status
         // CoD and IO bits both set
         doNext();   // *only* after valid data is read         
      }
      break;

   default: 
      DPRINTK( "\nUnknown state: %X", state );
      break;
}

}


bool
IDEController::sendATAPICommand( const ATAPICommandPacket& cmd, u16 bytecount, u8 masterSlave )
{
   DPRINTK( "\nsendATAPICommand opcode=%X", cmd.opcode );   

   // 1. Wait until ready (XX Linux writes 'select' before this ?? )
   int c = 10000;
   
   // Crucial to poll altStatus here, real devices won't send IRQ otherwise
   while (readAltStatus() & (BUSY_STAT) )   // NOTE: Regardless of DRDY, p.62
   {
      if (--c == 0 ) break;   // timeout, bochs does not support altStatus  
   }

   // 0. Select drive & head LBA=1 (optimize ? if already selected...)
   writeDriveHead( 0xA0 | 0x40 | masterSlave | 0x00 /* head, 0..15 */);

   // Wait again, this should return soon enough
   // For ATA devices, SEEK complete would be set, but ATAPI...
   c = 10000;
   while (readAltStatus() & (BUSY_STAT) )   // NOTE: Regardless of DRDY, p.62
   {
      if (--c == 0 ) break;   // timeout, bochs does not support altStatus       
   }
         
   // 2. Initialize task file (i.e. IO registers)
   
   // Set Byte count (cyl lo/hi), only for sector read ?
   writeCylinder( bytecount );
   
   // Set feature register? bit0 == DMA, bit1 == overlap, rest reserved
   // hd_feature = (1<<1) | (1<<0);
   
   // 3. Start ATAPI command, state already updated
   writeCommand( 0xA0 );    // ATAPI packet command, this may trigger IRQ

   // XX There should be some delay here, to allow device to set BUSY

   // Could use IRQ instead? This happens within <=3ms according to ATAPI std
   // More precisely, bits 5-6 of 'identify drive' command indicate 3 options:
   // 00: Within 3ms
   // 01: Uses IRQ
   // 10: Within 50us <- Bochs + laptop reports this
   c = 10000;
   while ( readAltStatus() & (BUSY_STAT) )
   {
      if (--c == 0 ) break;   
   }
   
   // TODO: in IRQ handler instead ?
   writeData( cmd.getBytes(), 12 );    
   
   return true;   
}
 
void
IDEController::doNext()
{
   DPRINTK( "\nIDEController::doNext() state=%u", state );
   
   // Policy is to do round-robin scheduling between master&slave, simplest form
   switch (state)
   {
      case S_MASTER_SEEKING:
      case S_MASTER_READING:
         if (slave && slave->hasWork()) {
            curRq = slave->nextRq();
            if (curRq) return slave->process( *curRq );
         }
         // no break

      case S_SLAVE_SEEKING:
      case S_SLAVE_READING:
         if (master && master->hasWork()) {
            curRq = master->nextRq();
            if (curRq) return master->process( *curRq );
         } 
         
         if (slave && slave->hasWork()) {   // repeat code above
            curRq = slave->nextRq();
            if (curRq) return slave->process( *curRq );
         } 
         
         // nothing more to do! curRq == 0
         DPRINTK( "-> no more requests, going to READY" );
         state = S_READY;
         return;
      
      default:
         DPRINTK( "\nUnhandled state %u in doNext", state );
   }   
} 
   
}}}   // namespace

//#endif
