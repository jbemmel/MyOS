/**
 * Information used for this driver comes from:
 * - http://www.stanford.edu/~csapuntz/ide.html
 * 
 * Apparently, PCI IDE controllers can work in two modes: compatibility mode
 * and native-PCI mode
 * 
 * The former (compatibility) requires less code (needs general PCI code that I 
 * did not port yet) so I start off with the compatibility mode stuff
 * 
 * Most important reason for deveoping this IDE driver currently is, that I want
 * a working ATAPI CD-ROM driver
 */
#ifndef IDE_H
#define IDE_H

#include "Devices/IOPort.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "Devices/IORequest.h"

#include "IIDEDeviceImpl.h"

namespace MyOS { namespace Devices { namespace IDE {

using namespace InterruptHandling;

class ATAPICommandPacket;

/// Represents the (PCI) IDE controller
/**
 * - Could use template for primary/secondary
 * - IRQs are 14 (primary) and 15 (secondary), INTA in PCI mode
 */
class IDEController : public IInterruptContext
{
public:
   enum E_BASE { PRIMARY = 0x1F0, SECONDARY = 0x170 };

   IDEController( E_BASE b ) : base(b), curRq(0) {}

   bool init() INITSECTION;
   bool sendATAPICommand( const ATAPICommandPacket& cmd, u16 bytecount, u8 masterSlave );

   IIDEDevice* getMaster() const { return master; }
   IIDEDevice* getSlave() const { return slave; }

   inline void reissueRequest() { curRq->reissued(); }
   
private:
   // IInterruptContext
   virtual void onInterrupt();
   
   // Sets curRq to the next request to be processed, and executes that
   void doNext();

public:

   void writeData( u16* data, size_t bytes ) {
      outsw( base + 0, data, bytes );
   }

   u8 readError() const { return inb( base+1 ); }
   void writeFeature( u8 f ) { outb( base+1, f ); }

   u8 readInterruptReason() const { return inb(base+2); }
   void writeSectorCount( u8 c )  { outb(base+2, c); }

   u8 readSector() const { return inb(base+3); }
   void writeSector( u8 s )  { outb(base+3, s); }

   u16 readCylinder() const { return (inb(base+5)<<8) | inb(base+4); }
   void writeCylinder( u16 c ) { outb(base+4,c&0xff); outb(base+5,c>>8); }

   u8 readDriveHead() const { return inb(base+6); }
   void writeDriveHead( u8 d ) { outb(base+6,d); }

   u8 readStatus() const { return inb(base+7); }
   void writeCommand( u8 c ) { outb(base+7,c); }   

   // Use ALT status instead of status, to prevent IRQ to be deasserted
   u8 readAltStatus() const { return inb(base+0x206); }
   void writeControl( u8 c ) { outb(base+0x206,c); }
   
   // Note: This causes *both* devices to reset (ATA/ATAPI specs)
   void reset() {
      outb(base+0x206, 1<<2);             // set bit 2
      delay400ns();
      outb(base+0x206, 0);                // clear it
      delay400ns();      
      while (readStatus() & 0x80);        // wait until MASTER BUSY is cleared
      
      // Causes MASTER to be selected!
   }   
   
   /// @return Number of bytes read
   /// (only) newer controllers support 32-bit data transfers!   
   u16 readData( u16* data, size_t maxbytes ) {
      u16 toRead = min( maxbytes, readCylinder() );
      insw( base + 0, data, toRead );
      return toRead;
   }   

   void setSeeking( u8 masterSlave ) {
      state = masterSlave ? S_SLAVE_SEEKING : S_MASTER_SEEKING;
   }

   void setReading( u8 masterSlave ) {
      state = masterSlave ? S_SLAVE_READING : S_MASTER_READING;
   }

   bool handleNow( IORequest& rq ) { 
      if (state==S_READY) {
         curRq = &rq;
         return true;
      }
      return false;
   }

private:
   void delay400ns() {
      readAltStatus();
      readAltStatus();
      readAltStatus();
      readAltStatus();
   }

   /******
    * DATA
    ******/   
   E_BASE base;   // Base address of registers, 0x1F0(primary) or 0x170(secondary)
   
   enum E_STATE {
      S_UNDEFINED    = 0,
      S_DETECTING    = 1,
      S_READY        = 2,
      S_MASTER_SEEKING  = 3,
      S_MASTER_READING  = 4,

      S_SLAVE_SEEKING   = 5,
      S_SLAVE_READING   = 6,
   } state;

   IIDEDeviceImpl* master;
   IIDEDeviceImpl* slave;

   IORequest* curRq;
};

}}} // namespace

#endif
