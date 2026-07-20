/***************************************************************************
                          DP8390xd.h  -  description
                             -------------------
    begin                : Tue Apr 30x 20x0x2
    copyright            : (C) 20x0x2 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl

Implements the DP83290xD chip used eg in NE2000 cards. This one is supported by 
Bochs, and not PCI based

Linux driver(s): 8390.c, ne.c, ne2k.c, ...

***************************************************************************/

#ifndef DP8390xD_H
#define DP8390xD_H

//#include "MyOSConfig.h"
//#if defined(CONFIG_NE2000) || defined(COMPONENTCOMPILATION)

#include "Devices/Network/IEthernetDevice.h"
#include "Devices/IOPort.h"
#include "Atomic/atomic32.h"
#include "InterruptHandling/IInterruptHandling.h"

namespace MyOS {
namespace Devices {
namespace Network {
namespace NE2000 {

using namespace InterruptHandling;
using Networking::Ethernet::EthernetPacket;

/**
 *@author Jeroen van Bemmel
 */
class DP8390D : public IEthernetDevice, public IInterruptContext
{
public:
	DP8390D( MyOS::Core::IComponent& c ) INITSECTION;

	bool init() INITSECTION;
	void deinit();

	// IPacketDevice
	virtual myos_result_t open( IPacketReceiver& rx, u32 mode );
	virtual void close();
	virtual myos_result_t sendPacket( const buffer& buf, u32 flags, 
										IPacketSender* onDone, void* act );

	// IEthernetDevice
	// virtual EthernetPacket* createPacket( size_t maxPayload ) const;
	// virtual void releasePacket( EthernetPacket &p ) const;
	// virtual RESULT transmit( EthernetPacket &p, size_t octets, u32 flags, void* act );
	// virtual const char* getDeviceName() const { return "ne2000"; }
	virtual myos_result_t getAddress( IO::OStream& out ) const;
	
   // should be a variable, for now constant (bochs configured at 0x300)
   enum {
      BASEADDRESS                      = 0x300,		// see .bochsrc
      MAX_RETRY_COUNT_WHILE_BLOCKING   = 100000,
      E_TX_BUFFERS                     = 6,			//< Number of TX buffers
   };

private:
   /// Selects accessable page, must be called depending on which register you want to read/write
   inline void selectPage( u8 page );

   bool probe( /* u16 baseadr */ ) INITSECTION;
   void setPromiscuous();

   // IInterruptContext
   virtual void onInterrupt();

   void handleRX();
   void handleTX();
   void handleTXError();
   void blockInput( u16* buffer, size_t bytes, size_t offset );
   void blockOutput( u16* buffer, size_t bytes, u8 page );
   void triggerTx( size_t bytes, u8 startPage );

	u8* rxBuffer;     // DMA memory for rx
	u8* rxCurPacket;  // current pos in rx buffer

	IPacketReceiver* rxclient;

   // pingpong buffer for tx, checked&sent in IRQ handler
   struct JOB {
      EthernetPacket* 	packet;
      IPacketSender*  	notify;
      void*				act;
      volatile s16 bytes;
      u8 page;	// Could be calculated instead	
      u8 flags;	// could also be used for locking
   } PACKED pingpong[E_TX_BUFFERS];

   // statistics
   u32 rxCount, txTriggered, txIrq, frameErrors, crcErrors, missedPackets;

   // Flag indicating that chip is busy processing a triggered TX
   bool txing, inInterrupt;

   enum E_STATUS {
      E_UNDEFINED    = 0,
      E_NO_CHIP      = 1,
      E_INITIALIZED  = 2,
      E_OPENED       = 3,
   } status : 8;

   /*
    * Registers are page(0x,1,2)-mapped, all 8 bit. Base address may vary, read/write are different
    */
   volatile IOPort8< BASEADDRESS+0x0 > commandRegister;    // same for all pages

   // Page 0x (normal operation)
   volatile IOPort8_R< BASEADDRESS+0x1 > localDMA0;       volatile IOPort8_W< BASEADDRESS+0x1 > pageStart;
   volatile IOPort8_R< BASEADDRESS+0x2 > localDMA1;       volatile IOPort8_W< BASEADDRESS+0x2 > pageStop;
   volatile IOPort8< BASEADDRESS+0x3 > boundaryPtr;
   volatile IOPort8_R< BASEADDRESS+0x4 > txStatus;        volatile IOPort8_W< BASEADDRESS+0x4 > txPageStart;
   volatile IOPort8_R< BASEADDRESS+0x5 > colissionCount;  volatile IOPort8_W< BASEADDRESS+0x5 > txCount0;
   volatile IOPort8_R< BASEADDRESS+0x6 > fifo;            volatile IOPort8_W< BASEADDRESS+0x6 > txCount1;
   volatile IOPort8< BASEADDRESS+0x7 > isr;
   volatile IOPort8_R< BASEADDRESS+0x8 > remoteDMA0;      volatile IOPort8_W< BASEADDRESS+0x8 > remoteStart0;
   volatile IOPort8_R< BASEADDRESS+0x9 > remoteDMA1;      volatile IOPort8_W< BASEADDRESS+0x9 > remoteStart1;
   volatile IOPort8_W< BASEADDRESS+0xA > remoteCount0;
   volatile IOPort8_W< BASEADDRESS+0xB > remoteCount1;
   volatile IOPort8_R< BASEADDRESS+0xC > rxStatus;        volatile IOPort8_W< BASEADDRESS+0xC > rxConfig;
   volatile IOPort8_R< BASEADDRESS+0xD > faeCount;        volatile IOPort8_W< BASEADDRESS+0xD > txConfig;
   volatile IOPort8_R< BASEADDRESS+0xE > crceCount;       volatile IOPort8_W< BASEADDRESS+0xE > dataConfig;
   volatile IOPort8_R< BASEADDRESS+0xF > missCount;       volatile IOPort8_W< BASEADDRESS+0xF > imr;

   // counters are 8bit and have max. value of 192, then overflow occurs. Reading clears

   // Page 1 (init)
   volatile IOPort8< BASEADDRESS+0x1 > physAdr0;
   volatile IOPort8< BASEADDRESS+0x2 > physAdr1;
   volatile IOPort8< BASEADDRESS+0x3 > physAdr2;
   volatile IOPort8< BASEADDRESS+0x4 > physAdr3;
   volatile IOPort8< BASEADDRESS+0x5 > physAdr4;
   volatile IOPort8< BASEADDRESS+0x6 > physAdr5;
   volatile IOPort8< BASEADDRESS+0x7 > curPage;       // updated by device, only write during init
   volatile IOPort8< BASEADDRESS+0x8 > multicastAdr0;
   volatile IOPort8< BASEADDRESS+0x9 > multicastAdr1;
   volatile IOPort8< BASEADDRESS+0xA > multicastAdr2;
   volatile IOPort8< BASEADDRESS+0xB > multicastAdr3;
   volatile IOPort8< BASEADDRESS+0xC > multicastAdr4;
   volatile IOPort8< BASEADDRESS+0xD > multicastAdr5;
   volatile IOPort8< BASEADDRESS+0xE > multicastAdr6;
   volatile IOPort8< BASEADDRESS+0xF > multicastAdr7;

   // Page 2 (diagnostic)
   volatile IOPort8_R< BASEADDRESS+0x1 > DIAG_pageStart;       volatile IOPort8_W< BASEADDRESS+0x1 > DIAG_localDMA0;
   volatile IOPort8_R< BASEADDRESS+0x2 > DIAG_pageStop;        volatile IOPort8_W< BASEADDRESS+0x2 > DIAG_localDMA1;
   volatile IOPort8< BASEADDRESS+0x3 >   DIAG_remoteNextPacket;
   volatile IOPort8_R< BASEADDRESS+0x4 > DIAG_txPageStart;
   volatile IOPort8< BASEADDRESS+0x5 >   DIAG_localNextPacket;
   volatile IOPort8< BASEADDRESS+0x6 >   DIAG_addressHi;
   volatile IOPort8< BASEADDRESS+0x7 >   DIAG_addressLo;
   // .. a few reserved
   volatile IOPort8_R< BASEADDRESS+0xC > DIAG_rxConfig;
   volatile IOPort8_R< BASEADDRESS+0xD > DIAG_txConfig;
   volatile IOPort8_R< BASEADDRESS+0xE > DIAG_dataConfig;
   volatile IOPort8_R< BASEADDRESS+0xF > DIAG_imr;

   // ASIC data port in high 16 bytes of IO memory, only 2 used
   volatile IOPort16< BASEADDRESS + 0x10 > dataport;  	// also byte-addressable
   volatile IOPort8< BASEADDRESS + 0x10 > dataport_u8;
   volatile IOPort8< BASEADDRESS + 0x1f > resetport; // ?? gambling, based on ne2.c
   
   // see http://bochs.sourceforge.net/cgi-bin/lxr/source/iodev/ne2k.cc?v=bochs-2000-03-25
   // Bochs assumes offset 0x1f, and lets both reads and writes reset the device
};

}}}} // namespace

// #endif   // CONFIG_NE2000

#endif   // DP8390xD_H
