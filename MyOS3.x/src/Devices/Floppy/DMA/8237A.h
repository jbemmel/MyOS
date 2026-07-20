/***************************************************************************
                          8237A.h  -  description
                             -------------------
    begin                : Thu Feb 14 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

Code for the DMA controller
See http://titan.etf.bg.ac.yu/~gvozden/mips/download/8237.pdf for manual
 ***************************************************************************/
#ifndef _8237A_H
#define _8237A_H

#include "Devices/IOPort.h"
#include "MM/memtypes.h"   // physadr_t

#include "debug.h"

namespace MyOS {
namespace Devices { 
namespace DMA {

using namespace MM;

/// Chooses between master- and slave DMA chip
enum DMACHIP { DMA1_SLAVE=0, DMA2_MASTER=0xc0 };

/// Options used for @see C8237A#maskChannel
enum MODE {
	// CHANNELMASK=0x3,
	CHANNEL0_4=0,
	CHANNEL1_5=1,
	CHANNEL2_6=2,
	CHANNEL3_7=3,

	// Transfer mode, invalid if MODE_CASCADE
	MODE_VERIFY = 0<<2,
	MODE_WRITE  = 1<<2,		// write means from device to memory
	MODE_READ   = 2<<2,

	AUTO_INITIALIZE    = 1<<4,
	AUTO_INCREMENT     = 0,
	AUTO_DECREMENT     = 1<<5,
	MODE_DEMAND        = 0<<6,
	MODE_SINGLE        = 1<<6,
	MODE_BLOCK         = 2<<6,
	MODE_CASCADE  	   = 3<<6
};

/// Represents the DMA controller chips
class C8237A
{
public:
   inline void init() {
      // reset both chips (random values), this resets command register too
      DMA1_masterClear = 0xff;
      DMA2_masterClear = 0xff;

      // enable both chips (MASTER needed by the slave in cascade...)
      // Linux does not seem to do this
      //
      // I found that newer PCI-ISA bridges supposedly support this register,
      // but very limited: http://www.intel.com/design/chipsets/datashts/290550.htm
      // '0' seems the best value to set, and reset above does that
      //
      // activate( DMA2_MASTER );
      // activate( DMA1_SLAVE );

      // unmask the cascade channel in the master (!)
      unmaskChannel( DMA2_MASTER, CHANNEL0_4 );
   }

	// Dectivate DMA chip
	void deactivate( DMACHIP dma )
	{
		if (dma==DMA1_SLAVE) DMA1_commandRegister = DEACTIVATE | FIXED_PRIORITY;
		else DMA2_commandRegister = DEACTIVATE | FIXED_PRIORITY;
	}

	/// Set mode for the chip
   /**
    * @param mode : OR of channel and other MODE bits
    */
	void setMode( DMACHIP dma, u8 mode )
	{
		if (dma==DMA1_SLAVE) DMA1_modeRegister = mode;
		else DMA2_modeRegister = mode;
	}

	void maskChannel( DMACHIP dma, MODE channel )
	{
		if (dma==DMA1_SLAVE)
		   DMA1_channelMaskRegister = channel | SET_BIT;
		else
		   DMA2_channelMaskRegister = channel | SET_BIT;
	}

	void unmaskChannel( DMACHIP dma, MODE channel )
	{
		if (dma==DMA1_SLAVE)
		   DMA1_channelMaskRegister = channel | CLEAR_BIT;
		else
		   DMA2_channelMaskRegister = channel | CLEAR_BIT;
	}

	/* Activate DMA chip, linux does not seem to touch the CMD register...
	void activate( DMACHIP dma )
	{
        // Note: Bochs complains about command byte not being 0x0 ???
     
		// compressed cycle
		if (dma==DMA1_SLAVE) {
		   DMA1_commandRegister = ACTIVATE | FIXED_PRIORITY | COMPRESSED_CYCLE;
		} else {
		   DMA2_commandRegister = ACTIVATE | FIXED_PRIORITY | COMPRESSED_CYCLE;
      }
	}
    */

	/**
	 * 	Set the number of bytes to transfer
	 *	@param dma: DMA1_SLAVE or DMA2_MASTER
	 *	@param channel: 0..3 as defined by MODE
	 *	@param count: number of bytes to transfer (16 bit + 1, max. 0x10000 )
	 */
	void setCount( DMACHIP dma, MODE channel, u32 count )
	{
		ASSERTION( count <= 0x10000, E_ERROR );

		// erase flipflop
		if (dma==DMA1_SLAVE) {
		   DMA1_flipflopClear=0xff;		// random value
		} else {
		   DMA2_flipflopClear=0xff;
		}

		// rather than using IOPort, calculate the port (1,3,5,7 or c1,c3,c5,c7)
		_out2( dma+2*channel+1, count-1 );	// low, then high byte (1 less than count)
	}

	/**
	 * Set the physical(!) address where to start the transfer
	 *	@param dma: DMA1_SLAVE or DMA2_MASTER
	 *	@param channel: 0..7
	 *	@param address: physical start address (24 bit)
	 */
	void setAddress( DMACHIP dma, MODE channel, physadr_t address )
	{
		ASSERTION( address <= 0x00FFFFFF, E_ERROR );

      // pageIO offset for DMA2
      u8 pageIOOffset;

		// erase flipflop
		if (dma==DMA1_SLAVE) {
		   DMA1_flipflopClear=0xff;		// random value
		   pageIOOffset = 0;
		} else {
		   DMA2_flipflopClear=0xff;
		   pageIOOffset = 8;
      }

		// rather than using IOPort, calculate the port (0,2,4,6 or c0,c2,c4,c6)
		_out2( dma+2*channel, address & 0xFFFF );		// low, then high byte (1 less than count)

		outb( pageIOport[channel] + pageIOOffset, address >> 16 );
	}

private:
	// SLAVE chip (AT)
	const IOPort8_R<0x08> DMA1_statusRegister;
         IOPort8_W<0x08> DMA1_commandRegister;
         IOPort8_W<0x09> DMA1_requestRegister;
         IOPort8_W<0x0a> DMA1_channelMaskRegister;    // for single channel at a time
         IOPort8_W<0x0b> DMA1_modeRegister;
         IOPort8_W<0x0c> DMA1_flipflopClear;
	const	IOPort8_R<0x0d> DMA1_tempRegister;
         IOPort8_W<0x0d> DMA1_masterClear;
         IOPort8_W<0x0e> DMA1_maskClear;
         IOPort8_W<0x0f> DMA1_maskRegister;           // to mask/clear many channels at once

	// MASTER chip, different addresses
	const IOPort8_R<0xd0> DMA2_statusRegister;
         IOPort8_W<0xd0> DMA2_commandRegister;
         IOPort8_W<0xd2> DMA2_requestRegister;
         IOPort8_W<0xd4> DMA2_channelMaskRegister;
         IOPort8_W<0xd6> DMA2_modeRegister;
         IOPort8_W<0xd8> DMA2_flipflopClear;
	const	IOPort8_R<0xda> DMA2_tempRegister;
         IOPort8_W<0xda> DMA2_masterClear;
         IOPort8_W<0xdc> DMA2_maskClear;
         IOPort8_W<0xde> DMA2_maskRegister;

	// Mapping of channel->page register IO
	static const u8 pageIOport[4];

	// commandRegister
	enum COMMAND {
		MEM2MEM           = 1<<0,
		ADR_HOLD	         = 1<<1,	// channel0,4, only iff MEM2MEM==0
		ACTIVATE	         = 0,
		DEACTIVATE        = 1<<2,
		COMPRESSED_CYCLE  = 1<<3,
		ROTATE_PRIORITY   = 0,
		FIXED_PRIORITY		= 1<<4,
		LATE_WRITE	      = 0,
		EXTENDED_WRITE		= 1<<5,	// only matters iff COMPRESSED_CYCLE==0 (vs LATE_WRITE)
		DRQ               = 1<<6,	// DMA request, active lo (default=0=hi)
		DACK              = 1<<7,  // DMA ack, active hi (default=0=lo)
	};

	// mask register
	enum MASK {
		// channels as in MODE
		CLEAR_BIT 	= 0,
		SET_BIT 		= 1<<2
	};
};

}}}	// namespace

#endif
