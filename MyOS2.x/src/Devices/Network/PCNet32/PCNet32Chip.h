/***************************************************************************
                        PCNet32Device.h  -  description
                           -------------------
  begin                : Thu Oct 18 2001
  copyright            : (C) 2001 by Jeroen van Bemmel
  email                : jeroen@thebem.localdomain

    @see http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/20550.pdf
***************************************************************************/
#ifndef PCNET32CHIP_H
#define PCNET32CHIP_H

#include "MM/memtypes.h"            				// physadr_t
#include "Networking/Ethernet/EthernetPacket.h"  	// ethadr_t

namespace MyOS { namespace Devices { namespace Network { namespace PCNet32 {
	
using MM::physadr_t;
using Networking::Ethernet::ethadr_t;

// PCNet32 takes only 32 bytes of IO space, and uses an indexing scheme
enum IO32REGISTERS {
    /// 32bit access mode (p. 106)
    /// the chip automatically switches to 32bit mode when a 32bit write to RDP occurs (p.166)
    PCNET32_DWIO_RDP    =0x10,
    PCNET32_DWIO_RAP    =0x14, /// Register Address Pointer
    PCNET32_DWIO_RESET  =0x18,
    PCNET32_DWIO_BDP    =0x1C,

    /// for resetting only
    PCNET32_WIO_RAP     =0x12 /// Register Address Pointer
};

/// Chip status registers
enum CSR_REGISTERS {
    CONTROLLER_STATUS		= 0,
    INITBLOCK_LO        	= 1,
    INITBLOCK_HI            = 2,
    INTMASK_DEFERAL_CTRL	= 3,
    TEST_FEAT_CTRL         	= 4,
    EXT_CONTROL_INT1     	= 5,
    EXT_CONTROL_INT2     	= 7,

    PHYS_ADR0_1 = 12,
    PHYS_ADR2_3 = 13,
    PHYS_ADR4_5 = 14,

    MODE = 15,        // value from initblock, only accesible iff STOP==1 !

    // CSR_SOFTWARE_STYLE = 58, aliassed by BCR18

    // R/W only iff STOP==1
    TX_POLLING_INTERVAL = 47,
    RX_POLLING_INTERVAL = 49,
    RX_RING_LENGTH = 76,
    TX_RING_LENGTH = 78,

    DMATransferFIFOTreshold = 80,    // p. 132

    CHIPID_LO = 88,   /// the only 32bit register, see p. 106
    CHIPID_HI = 89,
    ADVANCED_FEATURE_CONTROL = 122,
    TESTREGISTER1    = 124
};

/// Burst / bus control
enum BCR_REGISTERS {
    MISC_CONFIG                  = 2,

    LED0_STATUSCONTROL    = 4,    // default: link state
    LED1_STATUSCONTROL    = 5,    // default: receive status
    LED2_STATUSCONTROL    = 6,    // default: rx poll
    LED3_STATUSCONTROL    = 7,    // default: transmit status

    FULLDUPLEX_CONTROL  = 9,
    BCR_BURST_CONTROL   = 18,   // default : 0x9001, alias for CSR58
    BCR_SOFTWARE_STYLE  = 20,   // default : 0x0200
    PCI_LATENCY         = 22,   // set to 0x1818
    SRAM_SIZE           = 25,	// upper 8 bytes => 512-byte units, chip has 12K = 24 pages (value=23)
    SRAM_RXBUFFER_START = 26,   // TX starts at 0, only valid if not in low-latency mode
    SRAM_CONTROL        = 27,
    SW_TIMER            = 31,   // 16 bit sw timer, units of 12.8us -> max 838ms
    MII_CONTROL_STATUS	= 32
};

/// access in 32bit, lower 16 bit are length
enum TXRXSTATUS {
    BROADCASTMATCH                 = (1<<20),
    LOGICALADDRESSFILTERMATCH     = (1<<21),
    PHYSICALADDRESSMATCH            = (1<<22),

    ///         RX           |                TX
    BUSPARITYERROR     = (1<<23),
	ENDOFPACKET        = (1<<24),
    STARTOFPACKET      = (1<<25),
    BUFFERERROR        = (1<<26), DEF       = (1<<26),
    CRCERROR           = (1<<27), ONE       = (1<<27),    // one retry vs. more
    OVERFLOW           = (1<<28), MORE      = (1<<28),     TXDONE_INT = (1<<28),
    FRAMEERROR         = (1<<29), CHECKSUM	= (1<<29),
	ERROR              = (1<<30),  // RX: bitwise OR of FRAM | OFLOW | CRC | BUFF | BPE
    OWN                = (1<<31),  /// 1 => chip owns the buffer

    ANYERROR = BUFFERERROR | CRCERROR | OVERFLOW | FRAMEERROR | ERROR,

    /// Transmission starts by setting OWN bit, this is for packets that fit in a single buf!
    START_TX = OWN | STARTOFPACKET | ENDOFPACKET | CHECKSUM
};

/// Rx descriptor for swmode 3, Table 47
struct RXDESCRIPTOR {
	u16 packetSize;                 // 12 bits, 4 are 0
	u16 packetTag;              // 15 bits, 1 reserved
	union {
		s16 bufferLength;           // 2-complement (negative) length
		/*TXRXSTATUS*/ s32 status;  // access 32 bit address
	};
	physadr_t rxBuffer;
	u32 userspace;
} PACKED;

enum TXMISC {
    CLEAR         = 0,
    TRC_MASK      = 0x000F,

    RETRY         = (1<<26),
    LCAR          = (1<<27),
    LCOL          = (1<<28),
    EXDEF         = (1<<29),
    UNDERFLOW     = (1<<30),
    TXBUFFERERROR = (1<<31)
};

/// for swmode 0x3, p. 46 0f 2nd doc
struct TXDESCRIPTOR {
    TXMISC misc;
	union {
		s16 packetSize;
		/*TXRXSTATUS*/ s32 status;
	};
    physadr_t txBuffer;
    u32 userspace;
} PACKED;


/// Info structure (7 dwords), used to program the device
/// Documentation Am79C971 Table 43
struct CHIPCONFIG {
    u16 mode;					// value goes into CSR15, defined on p. 130
    u8 rxDescriptorCount;   	// upper 4 bits in log2, 9 and higher give 512
    u8 txDescriptorCount;		// upper 4 bits in log2, 9 and higher give 512
    ethadr_t physicalAddress;
    u16 _res2;
    u32 logicalAddressFilter[2];   /// 64 bit mask, indexed by CRC of incoming DA with bit0=1
    physadr_t rxDescriptorRing;    /// physical(!) address of RXDESC[]
    physadr_t txDescriptorRing;    /// physical(!) address of TXDESC[]
} PACKED;

/// CSR0 bits p. 196
enum IRQSTATUS {
    INIT     = 0x0001,
    START    = 0x0002,
    STOP     = 0x0004,
    TDMD     = 0x0008,
    TXON     = 0x0010,
    RXON     = 0x0020,
    IENA     = 0x0040,
    INTR     = 0x0080,
    INITDONE = 0x0100,
    TINT     = 0x0200,
    RINT     = 0x0400,
    MERR     = 0x0800,     // bus master arbitration failure
    MISS     = 0x1000,
    CERR     = 0x2000,     /// tranceiver test in 10Mbps mode only
    BABL     = 0x4000,
    ERR      = 0x8000,

    /// Acknowledge: all but a few bits
    ACKMASK = ~(INIT|START|STOP|TDMD|IENA),

    /// Command to clear all error bits (end of IRQ), 0x7940
    ACK_CLEAR = BABL|CERR|MISS|MERR|INITDONE|IENA
};

/// Interrupt mask & deferral control, p.109 of Am79C972_1.pdf
enum CSR3BITS {
    DisableMissedFrameInt = (1<<12),
    DisableMemoryErrorInt = (1<<11),
    DisableRxInt          = (1<<10),
    DisableTxInt          = (1<<9),
    DisableInitDoneInt    = (1<<8),

    GracefulUnderflowRecovery = (1<<6),
    LookAheadPacketProcessing = (1<<5),     // causes interrupt when first packet buffer comes in
    DisableTx2PartDeferal     = (1<<4),        // media access control
    EnableModifiedBackoff     = (1<<3),
    BigEndian                 = (1<<2)
};

/// Test&features control p. 112 of Am79C972_1.pdf
enum CSR4BITS {
    // Selection of events for generated IRQs
    TX_START_MASK             = (1<<2), // disabled IRQ for TX_START
    TX_START                  = (1<<3), // set by controller when TX started
    RX_COLL_COUNT_OF_MASK     = (1<<4), // disable IRQ for RCC Overflow
    RX_COLL_COUNT_OVERFLOW    = (1<<5),
    USERIRQ                   = (1<<6), // set when user irq was generated
    GENERATE_IRQ              = (1<<7), // forced generation of a user IRQ (if IENA=1)
    MISS_FRAME_COUNT_OF_MASK  = (1<<8), // disable IRQ for MFC Overflow
    MISS_FRAME_COUNT_OVERFLOW = (1<<9),

    /// Mask all of the above
    MASK_ALL = TX_START_MASK|RX_COLL_COUNT_OF_MASK|MISS_FRAME_COUNT_OF_MASK,

    // features
    AUTOSTRIPPADDING  = (1<<10),  // auto-strip padding from <64 IEEE Ethernet packets, including FCS
    AUTOPADDING       = (1<<11),  // automatically pad TX packets to 64 bytes if smaller
    DISABLE_TXPOLLING = (1<<12)
};

/// Ext. control & interrupt 1, p. 113
enum CSR5BITS {
    DisableTxOKInt       = (1<<15), // only effective when TxIntCheck==0
    EnableLastTxIntCheck = (1<<14), // when 1, chip reads bit28 of TXD to determine if INT is asked
    SystemInt            = (1<<11),
    EnableSystemInt      = (1<<10),
    ExcessiveDeferalInt  = (1<<7),
    EnableExcessiveDef   = (1<<6),
    RxMagicBroadcast     = (1<<5), // when 1, also respond to Magic Packets to broadcast addresses
    MagicPacketInt       = (1<<4),
    EnableMagicInt       = (1<<3),
    EnableMagicMode      = (1<<2),
    MagicPacketMode      = (1<<1),
    Suspend              = (1<<0), // set to 1, then poll and wait till it reads 1

    CSR5_ENABLE_ALL = EnableSystemInt | EnableExcessiveDef
};

/// Mode value for init block
enum CSR15BITS {

    DISABLE_RX = (1<<0),
    DISABLE_TX = (1<<1),

    ENABLE_LOOPBACK   = (1<<2),
    DISABLE_TX_CRC    = (1<<3),
    FORCE_COLLISION   = (1<<4), // for tesing collision logic, requires internal_loopback mode
    DISABLE_RETRY     = (1<<5), // disable 16x retransmission
    INTERNAL_LOOPBACK = (1<<6),

    /// Port selection, only when autoselect (BCR2[1]) is not used ??
    SELECT_AUI     = (0<<7),
    SELECT_10BASET = (1<<7),
    SELECT_GPSI    = (2<<7),
    SELECT_MII     = (3<<7),

    /// Only for AUI
    TRANSIT_MODE_SELECT = (1<<9),
    /// Only for 10BaseT intreface
    LOW_RX_TRESHOLD = (1<<9),      // 180 to 312 mV instead of standard 300 to 520 mV

    MENDEC_LOOPBACK = (1<<10),

    /// Only valid for 10Mbps, 10BaseT interface
    NO_AUTO_POLARITY      = (1<<11),
    NO_MONITOR_LINKSTATUS = (1<<12),

    NO_RX_PHYSMATCH       = (1<<13),
    NO_RX_BROADCAST       = (1<<14),
    PROMISCUOUS           = (1<<15),  // Receive all packets regardless of DA
};

enum CSR7BITS {
    /// Media Independent Interface(MII) interrupts
    EnableMIIPhysTransitionInt     = (1<<0),
    MIIPhysTransitionInt           = (1<<1),
    EnableMIIInternalCommandComp   = (1<<2),
    MIIInternalCommandCompleteInt  = (1<<3),
    EnableMIICommandComplete       = (1<<4),
    MIICommandCompleteInt          = (1<<5),
    EnableMIIAutoPollInt           = (1<<6),
    MIIAutoPollInt                 = (1<<7),
    EnableMIIReadErrorInt          = (1<<8),
    MIIReadErrorInt                = (1<<9),

    ENABLE_MII_INTS = EnableMIIPhysTransitionInt|EnableMIIInternalCommandComp|
          EnableMIICommandComplete|EnableMIIAutoPollInt|EnableMIIReadErrorInt,

    ANY_MII_INT = MIIPhysTransitionInt|MIIInternalCommandCompleteInt|MIICommandCompleteInt|
            MIIAutoPollInt|MIIReadErrorInt,

    SW_TIMER_ENABLE = (1<<10),
    SW_TIMER_INT    = (1<<11),    // set when sw timer fired if not masked
    RX_POLL_DISABLE = (1<<12),
    RX_DEMAND       = (1<<13),
    TAG_RX_FRAME    = (1<<14),    // tag rx packets, only valid if BCR2[3]==1
    FAST_SUSPEND    = (1<<15)     // used in combination with suspend
};

/// Advanced feature control (p. 8 of 2nd document)
enum CSR122BITS {
    RX_ALIGN_DWORD = (1<<0)                    // cleared by SW-RESET
};

/// Test register 1
enum CSR124BITS {
    RUNT_PACKET_ACCEPT = (1<<3)        // force chip to accept <64 packets, needed for low-latency rx mode
};

/// Burst&bus control,same as CSR58 (p.20 of Am79C972_2.pdf)
enum BCR18BITS {
    NoTxUnderflow                    = (1<<11),        // 1 -> chip will make sure no Tx underflow occurs, by waiting
    PCIBurstReadMultiple    = (1<<9),
    __EXTENDED_REQUEST__    = (1<<8),            // for non-PCI applications, it should be set to 0 !
    DWIO_MODE                            = (1<<7),            // read-only
    PCIBurstReadEnable        = (1<<6),
    PCIBurstWriteEnable        = (1<<5),
    PCI_BURSTMODE = PCIBurstReadEnable|PCIBurstWriteEnable
};

/// p.30 of Am79C972_2.pdf, SRAM control
enum BCR27BITS {
    LOW_LATENCY_RXMODE = (1<<14),        // special SRAM mode for low-latency reception, CSR124[3] must be 1
    EB_CLOCK_PCI             = (000<<3),        // default, clock should run @ 33 Mhz
    // EB_CLOCK_TIMEBASE    = (001<<3),    will not work at 100Mbps
    // EB_CLOCK_EBCLKPIN = (002<<3),    // ?? connected ??
    CLOCK_DIVISOR_1        = 0,
    CLOCK_DIVISOR_2        = 1,
    CLOCK_DIVISOR_3        = 3,
};

/// Can be used for monitoring and programming leds (latter iff BCR2[12]==1), multiple OR'd signals
enum LEDBITS {
    LEDOUT         = (1<<15),
    INVERT         = (1<<14),
    DISABLE        = (1<<13),
    MBPS100        = (1<<12),
    MAGICPACKET_RX = (1<<9),
    FULL_DUPLEX    = (1<<8),
    STRETCH_PULSE  = (1<<7),
    LINKSTATUS     = (1<<6),
    RXMATCH        = (1<<5),
    TXSTATUS       = (1<<4),
    RXSTATUS       = (1<<2),
    COLLISION      = (1<<0)
};

}}}}    // namespace

#endif
