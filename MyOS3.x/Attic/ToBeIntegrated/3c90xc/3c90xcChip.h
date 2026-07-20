/***************************************************************************
                        3c90xcDevice.h  -  description
                           -------------------
  begin                : Thu Oct 18 2001
  copyright            : (C) 2001 by Jeroen van Bemmel
  email                : jeroen@thebem.localdomain

	@see http://knowledgebase.3com.com/3c905xc.pdf (search a bit, log in) and 3c59x.c (linux)

	@note On the 3COM chip

	- 3c90x is called the "boomerang" series, with full DMA busmaster capabilities
	- Similar to AMD LANCE and DEC Tulip	
	- It can do TCP/IP header checksumming !
	- enqueued Tx packets may be reorderd to implement multiple priorities !

	The chip has a 2-stage transmission scheme: "download" from host to FIFO buffers,
	"transmission" onto the cable. Both can be signalled separately by interrupts

***************************************************************************/
#ifndef _3c90xcCHIP_H
#define _3c90xcCHIP_H

namespace _3c90xc {

// Register offsets (p.23), enum so they can be used for both MMIO and PIO
enum REGISTERS {
	/// First 3.5 dwords are shared by Window 0-7
	WIN0_BIOSRomAddress 	= 0x4,	// 32bit
	WIN0_BIOSRomData			= 0x8,	// 8bit
	WIN0_EEPROMCommand		= 0xa,	// 16bit
	WIN0_EEPROMData				= 0xc,	// 16bit

	WIN2_PhysAdr01						= 0x0,
	WIN2_PhysAdr23						= 0x2,
	WIN2_PhysAdr45						= 0x4,
	WIN2_PhysAdrMask01				= 0x6,	// "dont care" bits for matching
	WIN2_PhysAdrMask23				= 0x8,
	WIN2_PhysAdrMask45				= 0xa,
	WIN2_ResetOptions					= 0xc,

	WIN3_InternalConfig		= 0x0,	// 32bit
	WIN3_MaxPacketSize		= 0x4,	// 16bit
	WIN3_MacControl				= 0x6,	// 16bit, @see MACCONTROLBITS
	WIN3_MediaOptions			= 0x8,	// 16bit
	WIN3_RxFree						= 0xa,	// 16bit
	WIN3_TxFree						= 0xc,	// 16bit
                   	
	IntStatusCmd					= 0xe,	// 16bit, present in all windows

	/// Non-window registers
	TxPktID 				= 0x18,		// 8bit
	Timer						= 0x1a,		// 8bit
	TxStatus  			= 0x1b,		// 8bit
	IntStatusAuto   = 0x1e,		// 16bit
	DMAControl			= 0x20,		// 32bit
	DnListPtr				= 0x24,		// 32bit, writes only accepted if == 0
	DnBurstTresh		= 0x2a,		// 8bit
	DnPriorityTresh = 0x2c,		// 8bit, FIFO treshold for priority over uploads (in dwords)
	DnPoll					= 0x2d,		// 8bit, 320ns time intervals (max. 40.64us) (= 4 byte times at 100 Mbps)
	UpPacketStatus	= 0x30,		// 32bit
	FreeTimer				= 0x34,		// 16bit
	CountDown				= 0x36,		// 16bit
	UpListPtr				= 0x38,		// 32bit
	UpPriorityTresh = 0x3c,		// 8bit
	UpPoll					= 0x3d,		// 8bit
	UpBurstTresh		= 0x3e,		// 8bit
	RealTimeCount		= 0x40,		// 32bit
	ConfigAddress		= 0x44,		// 8bit
	ConfigData			= 0x48,		// 8bit
	
	DebugData				= 0x70,		// 32bit
	DebugControl		= 0x74,		// 16bit
	DnMaxBurst			= 0x78,		// 16bit, diagnostic: reads max burst experienced
	UpMaxBurst			= 0x7A,		// 16bit
	PwrMgmtControl 	= 0x7C		// 16bit
};


/// Commands to IntStatusCmd : upper 5 bits, lower 11 bit are parameters (p.135)
enum COMMANDS {
	///  The "/*!*/" entries require wait-until-complete
	/*!*/ TotalReset 	= 0x00 << 11,		// 0b00000
	/*!*/ RxReset 		= 0x05 << 11,		// 0b00101
	/*!*/ TxReset 		= 0x0B << 11,		// 0b01011

	// Tx
  /*!*/ DownStall 	= (0x6<<11)+2,	// 0b00110 + 0b10
	DownUnstall 			= (0x6<<11)+3,	// 0b00110 + 0b11
	SetTxReclaimTresh = 0x18 << 11,		// 0b11000
	SetTxStartTresh 	= 0x13 << 11,		// 0b10011
	TxAgain						= 0x12 << 11,		// 0b10010	
	TxDisable 				= 0x0A << 11,		// 0b01010
	TxDone						= 0x07 << 11,		// 0b00111
	TxEnable 					= 0x09 << 11,		// 0b01001
	TxFIFOBisect			= 0x1B << 11,		// 0b11011

	// Rx
	RxDisable 				= 0x03 << 11,		// 0b00011
	RxDiscard 				= 0x08 << 11,		// 0b01000
	RxEnable 					= 0x04 << 11,		// 0b00100
	SetHashFilterBit	= 0x19 << 11,		// 0b11001
	SetRxEarlyTresh		= 0x11 << 11,		// 0b10001
	SetRxFilter 			= 0x10 << 11,		// 0b10000	
	/*!*/ UpStall 		= (0x6<<11)+0,	// 0b00110 + 0b00
	UpUnstall 			 	= (0x6<<11)+1,	// 0b00110 + 0b01

	/// Interrupt
	AckIntr 					= 0x0D << 11,		// 0b01101
	FakeIntr 					= 0x0C << 11,		// 0b01100
	SetIndicationEn		= 0x0F << 11,		// 0b01111
	SetIntrEnable			= 0x0E << 11,		// 0b01110
		
	/// Other
	EnableDCConverter		= 0x02 << 11,		// 0b00010
	DisableDCConverter  =	0x17 << 11,		// 0b10111
	SelectRegWindow			= 0x01 << 11,		// 0b00001 + 3 bits for register
	StatsEnable 				= 0x15 << 11,		// 0b10101
	StatsDisable 				= 0x16 << 11,		// 0b10110

	/// From linux, not in manual
	StartDMAUp 		= (0x14<<11)+0,				// 0b10100 + 0b0
	StartDMADown 	= (0x14<<11)+1,				// 0b10100 + 0b1	
};

// Bits in the general status register at offset 0xe (when read) p.121-123
enum STATUSBITS {
	IntLatch 			= 0x0001,		// set when INT is indeed from the 3c90xc chip (for sharing IRQs)

	/// Possible IRQ sources
	HostError 		= 0x0002,		// major error, must be cleared by global reset
	TxComplete 		= 0x0004,
	// TxAvailable 	= 0x0008,  ?? linux driver
	RxComplete 		= 0x0010,
	RxEarly 			= 0x0020,
	IntReq 				= 0x0040,		// Counter or user-generated interrupt
	StatsFull 		= 0x0080,		// counters (nearly) overflow, cleared by reading those registers
	// DMADone 			= 0x0100,  ?? linux driver
	LinkChange		= 0x0100,
	DownComplete 	= 0x0200,
	UpComplete 		= 0x0400,

	// Other bits
	DMAInProgress = 0x0800,			// DMA controller is still busy
	CmdInProgress = 0x1000,			// EL3_CMD is still busy
	CurWindowMask	= 0xE000,

	INVALID 			= 0xffff						// when hw is not there
};

/// RXDESCRIPTORS, p. 105
struct RXDESCRIPTOR
{
	PHYSICALADDRESS nextBuffer;
	u32 status;		// copied from UpPktStatus register by chip

	/// Then, there are up to 63 pairs of (adr,size) structures:
	// struct FRAGMENT {
	//		PHYSICALADDRESS address;
	//	size_t length;
	//} fragments[1];	

	// OR... you can set bit 28 "impliedBufferEnable" to make the chip assume there is a 1528 bytes
  // buffer directly available starting after 'status'
	u8 buffer[ 1528 ];
};

/// Statusbits in RX packet header, p. 105
enum RXSTATUSBITS {
	packetLengthMask	= 0x00001FFF,
	upError						= (1<<14),
	upComplete				= (1<<15),
	upOverrun					= (1<<16),
	runtFrame					= (1<<17),	// only received if RxEarlyTresh < 60
	alignError				= (1<<18),
	crcError					= (1<<19),
	oversized					= (1<<20),
	dribbleBits				= (1<<23),		// informational, *not* an error
	upOverflow				= (1<<24),		// insufficient buffers available
	ipChecksumError			= (1<<25),  // These are only valid if checked (see below)
	tcpChecksumError		= (1<<26),
	udpChecksumError		= (1<<27),
	impliedBuffer				= (1<<28),	// must be refreshed every time
	ipChecksumChecked		= (1<<29),
	tcpChecksumChecked	= (1<<30),
	udpChecksumChecked	= (1<<31)
};

/// TXDESCRIPTORS Type 0, p. 85 (could use type 1 for scheduled packets)
struct TXDESCRIPTOR
{
	PHYSICALADDRESS nextBuffer;
	u32 frameStartHeader;        	// @see FRAMESTARTHEADER bits
	PHYSICALADDRESS packet;
	size_t packetLength;					// bit31 == fragment_last indication
};

/// Frame start header format (p. 86)
enum FRAMESTARTHEADER
{
	/// set this to round packetsizes up to the next (d)word size
	ALIGN_MASK 	= 0x3,
	DWORD_ALIGN = 0x0,
	WORD_ALIGN 	= 0x2,
	NO_ALIGN		= 0x1,	// 0x3 also means no align

	PACKETID_MASK = 0xFF << 2,		// 8bit packetid, placed in PktID register by chip
	
	DONT_APPEND_CRC 		= 1 << 13,
	TX_COMPLETE_INT 		= 1 << 14,		// generate TxComplete INT when packet has been sent
	DOWNLOAD_COMPLETE 	= 1 << 15,		// set by chip when transmission is done
	DISCARD_ON_MAX_COL  = 1 << 23,		// only for SMBus & keep-alive packets
	IS_LAST_KEEPALIVE		= 1 << 24,

	/// Chip does these, it first checks if the packet is an IP packet
	ADD_IP_CHECKSUM			= 1 << 25,
	ADD_TCP_CHECKSUM		= 1 << 26,
	ADD_UDP_CHECKSUM		= 1 << 27,

	DONT_ROUND_UP				= 1 << 28,		// disables auto-alignment by bits 0..1
	BUFFER_EMPTY				= 1 << 29,		// indicates an empty buffer
	
	DN_COMPLETE_INT			= 1 << 31,		// generate DnComplete INT on completion	
};

/// TX status bits in TXStatus register (8bit), p. 103
enum TXSTATUS {
	TxReclaimError		= 1 << 1,		// collision after ReclaimTresh bytes were freed from FIFO => no retry
	TxStatusOverflow	= 1 << 2,		// overflow on 32 entry status queue (chip stops Tx unit when set)
	maxCollisions			= 1 << 3,		// packet is discarded, Tx unit should be enabled again
	TxUnderrun				= 1 << 4,		// requires TxReset and TxEnable
	TxJabber					= 1 << 5,		// requires TxReset
	TxIndicateReq			= 1 << 6,		// bit was set in FRAMESTARTHEADER
	TX_TxComplete			= 1 << 7		// can be used for polling, rest are only valid if this is set		
};

/// p.179 : window 3, offset 6
enum MACCONTROLBITS {
	enableExtendedDefer					= (1<<0),
	deferTimer									= (1<<1),  	// 4 bits, standard IFS + 32 * bittime * <value>
	enableFullDuplex						= (1<<5),
	allowLargePackets						= (1<<6),
	extendAfterCollision				= (1<<7),
	enableFlowControl						= (1<<8),		// react to PAUSE frames	
	enableVLANtagging						= (1<<9),		// 3Com proprietary VLAN tagging scheme
	enableVLANoversizeHandling	= (1<<10)	
};

/// p.95, not all bits yet
enum DMACONTROLBITS {
	counterSpeed 		= (1<<8),
	countdownMode	  = (1<<9)
};

enum MIICONTROLBITS {
	inFullDuplexMode 				= (1<<8),
	restartAutoNegotiation 	= (1<<9),	
	isolate									= (1<<10),
	powerdown								= (1<<11),
	enableAutoNegotiate			= (1<<12),
	runningAt100Mbps				= (1<<13),
	loopback								= (1<<14),
	MIIreset								= (1<<15)
};

/// Bits that determine the kinds of packets to accept, p. 112 (default=none(!))
enum RXFILTERBITS {
	physicalMatch				= (1<<0),
	multicastMatch  		= (1<<1),		// this includes broadcast
	broadcastMatch			= (1<<2),
	promiscuous					= (1<<3),
	multicastHashMatch	= (1<<4)
};

/// (Global,RX)Reset option bits, p.137
enum RESETBITS {
	dontResetAUI			= (1<<0),		// 10-Base-T and AUI
	dontResetEndec		= (1<<1),		// internal encoder/decoder
	dontResetReceiver = (1<<2),		// if not set, rxFilter is cleared and receiver is disabled
	dontResetFIFO			= (1<<3),		// if not set, FIFO is flushed and RxEarlyTresh is defaulted

	// GLOBAL
	dontReloadEEPROM	= (1<<4),

	dontResetlogic	= (1<<8)		// not set 0-> UpListPtr, UpPacketStatus, UpEarlyRxEnable, upComplete
};


};	// namespace

#endif
