/***************************************************************************
                          RTL8139_chip.h  -  description
                             -------------------
    begin                : Sun Aug 19 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem

    Memory layout for the RTL8139 chip, @see http://chip.ms.mff.cuni.cz/~pcguts/ds/RTL8139C.pdf

 ***************************************************************************/
#ifndef RTL8139_CHIP_H
#define RTL8139_CHIP_H

#include "types.h"

#define READ_ONLY	// const, but then no default constructors can be used !
#define RESERVED_BITS 0

/// Needed to handle volatile structures
#define ASSIGN_VOLATILE(C,t) \
	inline C( const volatile C& c ) { \
	    *((t*)this) = *((t*) &c); \
	} \
	inline volatile C& operator=( const C& c ) volatile { \
	    *((t*)this) = *((t*) &c); \
	    return *this; \
	}

namespace RTL8139 {

enum E_TXSTATUS
{
	BYTECOUNT_MASK 	= 0x00001FFF,
	TxDMAComplete		= 0x00002000,
	TxFIFOUnderrun	= 0x00004000,
	TransmitOK	    = 0x00008000,
	EARLYTX_MASK		= 0x000FC000,
	OutOfWindow	    = 0x20000000,
	TransmitAbort		= 0x40000000,
	TxCarrierLost		= 0x80000000,

	TxDone		= TransmitOK | TransmitAbort | TxFIFOUnderrun,
	TxError		= OutOfWindow | TransmitAbort,		// TxCarrier is always asserted at 100Mbps (8139too)
	FINISHED 	=   TxDMAComplete | TxFIFOUnderrun | TransmitOK |
	                OutOfWindow | TransmitAbort | TxCarrierLost
};

// also for programming the transmitter
// all consts are reset by chip when writing count + DMA bit
struct TXSTATUS		
{
    // Total number of bytes in this descriptor.
    // documentation: When >0x700, transmit of next descriptor will wait
    // !! This implies a maximum of 8K packetsize !! (but ethernet: maxframesize = 1536 (8139too))	
	u16 ByteCount : 13;		
						
    // driver must set to false when sending, becomes true when finished	
	bool TxDMAComplete : 1;			
	
	READ_ONLY bool FIFOUnderrun : 1;		// RTL8139C can handle this
	READ_ONLY bool TransmitOK : 1;
	
    // nBytes = 8 + 32 * value [ so max value = 8 + 32 * 63 = about 2K bytes ]	
	u8 EarlyTxTreshold : 6;
	
	READ_ONLY u8 __reserved0__ : 2;
	READ_ONLY u8 CollisionCount : 4;	// Number of collisions during transmit
	READ_ONLY bool CDHeartBeat : 1;		    // cleared in 100MBps mode
	READ_ONLY bool OutOfWindowCollision : 1;
	READ_ONLY bool TransmitAbort : 1;
	READ_ONLY bool CarrierSenseLost : 1;
	
    inline void operator =( u32 v ) volatile
    {
        *((u32*)this) = v;
    }
		
	inline void startTx(
	    u32 byteCount, u8 earlyTxTreshold = 8 /* from 8139too */ ) volatile
	{
		// this clears TxDMAComplete
		(*this) = ((earlyTxTreshold&0x3f)<<16) | (byteCount & 0x1fff);	
	}

//	inline operator E_TXSTATUS() volatile
//	{
//		return (E_TXSTATUS) (u32) (*this);
//	}
};

// bitmasks for TX status
enum E_TXALLSTATUS
{
	TXOWN_bits	    = 0x000F,			
	TXABORT_bits		= 0x00F0,
	TXUNDERRUN_bits = 0x0F00,
	TXOK_bits 	    = 0xF000
};


// all fields are marked read-only in docs, but "writing '1' will clear" ??
struct EarlyRxStatus
{
	bool EarlyRxOK : 1;		// chips sets this when Rx bytecount > treshold, clears when complete packet is received
					        // "setting this bit will cause an ROK interrupt" ???
	bool EarlyRXOverwrite : 1;	// set when local address pointer == CAPR (different from buffer overflow)
	bool EarlyRxBad : 1;
	bool EarlyRxGood : 1;		
	u8 __reserved__ : 4;
	
	inline operator u8() const volatile { return *((u8*)this); }
	
	/// writing '1' to (almost) all bits clears them...
	inline void clear() volatile {  *((u8 *)this) = 0xE; }
};

enum CommandRegister {
	RxBufferEmpty 	= 0x1,
	TxEnable 	= 0x4,
	RxEnable 	= 0x8,
	RxTxEnable	= TxEnable | RxEnable,
	ResetChip 	= 0x10
};

enum IRQSTATUS { 	
	RxOK=0x1, 	// normalMode: complete packet		earlyMode: RxByteCount exceeded treshold => receipt still in progress
	RxErr=0x2, 	// CRC or frame alignment error
			
	TxOK=0x4,
	TxErr=0x8, 	// transmit aborted, due to excessive collisions (limit settable in Tx.RetryCount, minimum = 16 times )
			
	RxBufferOverflow=0x10,
	RxUnderrun_LinkChange=0x20,
	RxFIFOOverflow=0x40,
	CableLengthChange=0x2000,
	TimerInterrupt=0x4000,
	SystemError=0x8000,
	
	RxAck = RxOK | RxBufferOverflow | RxFIFOOverflow,

	DISABLE_ALL = 0x0,
	ALL_INTR		= 0xE07F,
	CLEAR_INTR 	= 0xFFFF		// ACK all interrupts (used in 8139too.c at too much work limit)
};

struct TXCONFIG
{
	bool ClearAbort : 1;			// In transmit abort state, force chip to retransmit last descriptor
	READ_ONLY u8 __reserved0__ : 3;
	u8 RetryCount : 4;			// count = 16 + (value*16), TER bit in ISR will be set only after n retries
	u8 MaxDMABurst : 3;		// nBytes = 2^(value+4), so max = 2K bytes per burst
	READ_ONLY u8 __reserved1__ : 5;	
	bool disableCRC : 1;
	enum { NORMAL=0, LOOPBACK=3 } loopback : 2;
	READ_ONLY u8 __reserved2__ : 4;
	READ_ONLY bool isRTL8139A_G : 1;	
	enum { IFG_840ns=0, IFG_880ns=1, IFG_920ns=2, IFG_960ns_default=3 } IFGTime : 2;	// for 100Mb operation, docs contradict
	
	enum {  NONE=0, RTL8139=0x10, RTL8139A=0x18,
	        RTL8139B=0x1C, RTL8130=0x1E, RTL8139C=0x1D } chipID : 5;
	READ_ONLY u8 __reserved3__ : 1;

	inline void operator=( const TXCONFIG &c ) volatile
	{
        *((u32*) this) = *((u32*) &c);	
	}
	
	inline void clearAbort( TXCONFIG settings ) volatile
	{
        // force chip to retransmit, 8139too.c does flushed write (W->R)
        settings.ClearAbort = true;
    	*this = settings;
	}

    /// Should not have a constructor	
	
};				
		
struct RXCONFIG
{
	bool AcceptAllPackets : 1;		// promiscuous mode I assume
	bool AcceptMatchingPhysical : 1;
	bool AcceptMulticast : 1;
	bool AcceptBroadcast : 1;
	bool AcceptSmallPackets : 1;		// when true, also received packets >= 8 bytes (normally >= 64 bytes)
	bool AcceptErrorPacket : 1;		// when false, packets will be rejected
	READ_ONLY enum { EEPROM9346=0, EEPROM9356=1 } Eeprom : 1;
	
    // when set, chip will continu writing data after end of rx buffer (=> allocate more!)	
	// invalid when RxBufferLength == 64K
	bool NoRingWrap : 1;			
						
	u8 maxRxDMABurst : 3;		// nBytes = 2^(value+4), 0x7 = no limit
	
    // 8K + 16 bytes, etc	
	enum { _8K_16=0, _16K_16=1, _32K_16=2, _64K_16=3 } RxBufferLength : 2;		
	inline u32 buffersize() const
	{
        // log2(8K) == 13
        return ((1<<(RxBufferLength+13)) + 16);
	}
	
    // nBytes = 2^(value+4), limit to start FIFO_2_hostmemory by PCI busmaster, 0x7 = no tresh	
	u8 RxFIFOTreshold : 3;		
	bool ErrorReceiveLimit_8byte : 1;	// normally, only erropackets larger than 64bytes are received	[default=0]
	
    /// when set, packet of any protocol will cause earlyRxIRQ when MultiInterruptSelect
    /// bytes have been received. If cleared, EarlyRxTreshold will be used for known protoc.
	bool MultiEarlyIRQ : 1;			
	READ_ONLY u8 __reserved0__ : 6;
	
    // 0 = no early transmit, value is percentage of total frame (1/16 .. 15/16)	
	u8 EarlyRxTresholdFactor : 4;	
	READ_ONLY u8 __reserved1__ : 4;
	
	// inline void operator =( u32 v ) 	{ *((u32*)this) = v; }	
	
	inline operator u32() const	        { return *((u32*)this); }	
	inline void operator=( const RXCONFIG &c ) volatile
	{
        *((u32*) this) = *((u32*) &c);	
	}			
};

/// EEPROM commands			
enum C9346Command
{
	NORMAL_MODE		    = 0x00,
	AUTO_LOAD		    = 0x40,
	PROGRAMMING_MODE	= 0x80,
	CONFIG_WRITE_ENABLE	= 0xC0
};

enum CONFIG1
{
	ENABLE_PM 				= 0x01,
	ENABLE_VPD				= 0x02,
	MAPPING_PIO				= 0x04,	/* read only */
	MAPPING_MMIO			= 0x08,	/* read only */
	LWAKE_ACTIVE_LOW	= 0x10,
	DRIVER_LOADED			= 0x20,
	LED0							= 0x40,
	LED1							= 0x80
};

enum MEDIASTATUS
{
	// read-only bits
	BackingOff					= 0x01,
  SendingPausePacket  = 0x02,
	LinkFail						= 0x04,
	Mode_10mbps					= 0x08,
	AuxPowerPresent			= 0x10,

	// settable bits
	EnableRxFlowControl	= 0x20,     // only for full-duplex, default from EEPROM	
  EnableTxFlowControl = 0x40
};

struct CONFIG3
{
	bool EnableFastBack2Back : 1;
	bool EnableCardbus_Registers : 1;
	bool EnableClock : 1;
	bool EnableCardbus : 1;
	bool LinkUp : 1;			// valid when Config1.EnablePM is set	
	bool enableMagicPacketWakeup : 1;	// read the manual for this one, complex !
	bool disableTweaking : 1;		// set to 0 to enable SW tweaking, also C9346Command.mode==CONFIG_WRITE_ENABLE must be set!
	READ_ONLY bool delayPCIGrant : 1;
	
	TREAT_AS( u8, CONFIG3 );
    ASSIGN_VOLATILE( CONFIG3, u8 );		
};

enum CONFIG4
{
	DisablePreBootWakeup	= 0x01,
	LANWakePattern				= 0x04,
	__disable_MS_wf__			= 0x08,		// readonly, reads 1 (functionality now in config5 + masks)
  LWAKE_DependsonPMEB		= 0x10,
  supportMSWakeupFrame	= 0x20,		// Microsoft-specific WakeUp functionality, yuk!
	TurnOffAnalogPower		= 0x40,		// default = 0 :)
  AutoClearFIFOOverflow	= 0x80
};

// Line tweaking
enum BASICMODECONTROL
{
	/// some reserved bits: 0..7, 14
	fullDuplex					= 0x0100,
	resetAutoNegotiate  = 0x0200,
  autoNegotiate				= 0x1000,
  enable100Mbps				= 0x2000,
	SWReset							= 0x4000
};

// could be a structure...
enum CSCRBits {
        CSCR_LinkOKBit=0x0400,
        CSCR_LinkChangeBit=0x0800,
        CSCR_LinkStatusBits=0x0f000,
        CSCR_LinkDownOffCmd=0x003c0,
        CSCR_LinkDownCmd=0x0f3c0,
};

// lots of register definitions for the chip
struct MMIO
{
	u32 IDR[2];			        // ID registers, 6 bytes are valid
	u32 MAR[2];			        // Multicast registers
	TXSTATUS TxStatus[4];			    // transmit status,  for 4 descriptors
	u32 TransmitStartAddress[4];	// transmit start address, for 4 descriptors, must be 32bit aligned !	
	u32 RxPhysBufferStart;		    // receive buffer start address
	const u16 EarlyReceiveBufferCount;	// "Early Receive Buffer Count Register"
	EarlyRxStatus EarlyReceiveStatus;	// Early receive status, RO but write '1' clears ?
	CommandRegister Command : 8;		// Command register
	u16 RxPacketRead;			    // current address of packet read (Rx) by handler
	const u16 RxWriteAddress;		// current buffer address, reflects total received bytecount (index into RxBuffer, READ_ONLY ??)
	IRQSTATUS IMR : 16;
	IRQSTATUS ISR : 16;
	TXCONFIG TxConfig;		// transmit configuration
	RXCONFIG RxConfig;		// receive configuration
	u32 Timer;				// general purpose timer, write any value to start counting from zero upwards
	u32 MissedPacketCounter;		// missed due to Rx FIFO overflow, 24bit valid, writing clears it
	C9346Command Config9346 : 8;		// 93c46 config register
	const u8 config0;			// contains BOOTROM config, some mode config
	u8 config1;					// @see enum CONFIG1
	u8 __reserved0__;
	u32 TimerMark;			// write value at which TIMEOUT status should be set, when Timer reaches this value
	u8 mediastatus;
	CONFIG3 config3;
	u8 config4;
	u8 HaltClock;
	u16 earlyRxsize;               // bytes received before earlyRx is asserted
	const u8 PCIrevisionID;		// should be 0x10
	u8 __reserved2__;
	const u16 TransmitStatusAll;	// status for all descriptors: OWN[4], TAB[4], TUN[4], TOK[4]
	u16 BasicModeControl;
	const u16 BasicModeStatus;
	
	// auto-negotiation
	u16 AN_Advertisement;
	const u16 AN_LinkPartner, AN_Expansion;
	
	const u16 DisconnectCounter;		    // reading clears
	const u16 FalseCarrierSenseCounter;	// reading clears
	u16 NWAY_TestRegister;
	const u16 PacketsReceived;			    // reading clears

	// Physical medium tuning
	u16 ChipStatusConfig;
	u16 __reserved3__;
	u16 PhyscalParam1;
	u32 Twister;
	u8  PhysicalParam2;
	u32 __reserved4__ : 24;
	u8  PM_CRC[8];			// PowerManagement CRC registers for frames 0..7
	u64 PM_Wakeupframe[8];
	u8  LSBMaskByte[8];
	u32 FlashMemory;
	u8  config5;
	u8 __reserved5__[23];
	
	// Cardbus only
	u32 CardBus_FunctionEvent;
	u32 CardBus_FunctionEventMask;
	const u32 CardBus_PresentState;
	u32 CardBus_ForceEvent;		// write_only
} PACKED;

};  // namespace

#endif
