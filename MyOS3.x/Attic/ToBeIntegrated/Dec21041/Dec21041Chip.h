/*
	Register definitions for the DEC21041 PCI Ethernet Controller
	@see ftp://tulip.sourceforge.net/pub/tulip/specs/21041/21041hm.pdf
*/
#ifndef DEC21041CHIP_H
#define DEC21041CHIP_H

namespace Dec21041 {

// IO Offsets of registers, table 3_1 p. 36 : implicitly used by PCI

/// CSR registers, Table 3-22 p. 49 : quadword aligned, long word access
enum CSRREGISTERS {
	BUS_MODE_REG				= 0x00,
	TX_POLL_DEMAND			= 0x08,
	RX_POLL_DEMAND			= 0x10,
	RX_BASE							= 0x18,
	TX_BASE							= 0x20,
	STATUS_REG					= 0x28,
	OPERATION_MODE_REG	= 0x30,
	INTERRUPT_MASK_REG	= 0x38,
	BOOT_AND_SERIAL_ROM = 0x48,
	SW_TIMER						= 0x58,

	SIA_STATUS					= 0x60,	/// CSR12
	SIA_CONNECTIVITY		= 0x68,
	SIA_TXRX						= 0x70,
	SIA_GENERAL					= 0x78
};

/* register offset and bits for CFDD PCI config reg, taken from tulip_core
enum pci_cfg_driver_reg {	
	CFDD = 0x40,
	CFDD_Sleep = (1 << 31),
	CFDD_Snooze = (1 << 30),
};
*/

/* The bits in the CSR5 status registers, mostly interrupt sources. */
enum status_bits {
	TimerInt = 0x800,
	SytemError = 0x2000,
	TPLnkFail = 0x1000,
	TPLnkPass = 0x10,
	NormalIntr = 0x10000,
	AbnormalIntr = 0x8000,
	RxJabber = 0x200,
	RxDied = 0x100,
	RxNoBuf = 0x80,
	RxIntr = 0x40,
	TxFIFOUnderflow = 0x20,
	TxJabber = 0x08,
	TxNoBuf = 0x04,
	TxDied = 0x02,
	TxIntr = 0x01,

	ALL_INTERRUPTS = 0x0001ebef
};


enum CSR6BITS {
	TxThreshold					= (1 << 22),
	FullDuplex					= (1 << 9),
	TxOn								= 0x2000,
	AcceptBroadcast			= 0x0100,
	AcceptAllMulticast	= 0x0080,
	AcceptAllPhys				= 0x0040,
	AcceptRunt					= 0x0008,
	RxOn								= 0x0002,
	RxTx								= (TxOn | RxOn),
};


enum CSR0BITS {
	MWI						= (1 << 24),
	MRL						= (1 << 23),
	MRM						= (1 << 21),
	CALShift			= 14,
	BurstLenShift	= 8,
};

struct RXTXDESCRIPTOR {
	s32 status;
	s32 length;
	u32 buffer1;
	u32 buffer2;
};

/// p. 100
enum desc_status_bits {
	DescOwned 			= (1<<31),
	FrameLengthMask	= 0x7FFF0000, // 16..30 = 15 bits
	ErrorSummary		= (1<<15),		// OR of bits { 0,1,6,7,11,14 }
	LengthError			= (1<<14),
	DataTypeMask		= (1<<13)|(1<<12), 	/// serial received frame / loopback frame
	RuntFrame				= (1<<11),
	MulticastFrame	= (1<<10),
	FirstDescriptor	= (1<<9),
	LastDescriptor	= (1<<8),
		RxWholePacket = FirstDescriptor | LastDescriptor,
	FrameTooLong		= (1<<7),
	CollisionSeen		= (1<<6),
	FrameType				= (1<<5),
	RxWatchdog			= (1<<4),
	DribblingBit		= (1<<2),
	CRCError				= (1<<1),
	Overflow				= (1<<0),

	DESC_RING_WRAP	= 0x02000000
};

enum t21041_csr13_bits {
	csr13_eng = (0xEF0<<4), // for eng. purposes only, hardcode at EF0h
	csr13_aui = (1<<3), 		// clear to force 10bT, set to force AUI/BNC
	csr13_cac = (1<<2), 		// CSR13/14/15 autoconfiguration
	csr13_srl = (1<<0), 		// When reset, resets all SIA functions, machines

	csr13_mask_auibnc = (csr13_eng | csr13_aui | csr13_srl),
	csr13_mask_10bt = (csr13_eng | csr13_srl),
};

enum t21143_csr6_bits {
	csr6_sc = (1<<31),
	csr6_ra = (1<<30),
	csr6_ign_dest_msb = (1<<26),
	csr6_mbo = (1<<25),
	csr6_scr = (1<<24),  /* scramble mode flag: can't be set */
	csr6_pcs = (1<<23),  /* Enables PCS functions (symbol mode requires csr6_ps be set) default is set */
	csr6_ttm = (1<<22),  /* Transmit Threshold Mode, set for 10baseT, 0 for 100BaseTX */
	csr6_sf = (1<<21),   /* Store and forward. If set ignores TR bits */
	csr6_hbd = (1<<19),  /* Heart beat disable. Disables SQE function in 10baseT */
	csr6_ps = (1<<18),   /* Port Select. 0 (defualt) = 10baseT, 1 = 100baseTX: can't be set */
	csr6_ca = (1<<17),   /* Collision Offset Enable. If set uses special algorithm in low collision situations */
	csr6_trh = (1<<15),  /* Transmit Threshold high bit */
	csr6_trl = (1<<14),  /* Transmit Threshold low bit */

	/***************************************************************
	 * This table shows transmit threshold values based on media   *
	 * and these two registers (from PNIC1 & 2 docs) Note: this is *
	 * all meaningless if sf is set.                               *
	 ***************************************************************/

	/***********************************
	 * (trh,trl) * 100BaseTX * 10BaseT *
	 ***********************************
	 *   (0,0)   *     128   *    72   *
	 *   (0,1)   *     256   *    96   *
	 *   (1,0)   *     512   *   128   *
	 *   (1,1)   *    1024   *   160   *
	 ***********************************/

	csr6_fc = (1<<12),   /* Forces a collision in next transmission (for testing in loopback mode) */
	csr6_om_int_loop = (1<<10), /* internal (FIFO) loopback flag */
	csr6_om_ext_loop = (1<<11), /* external (PMD) loopback flag */
	/* set both and you get (PHY) loopback */
	csr6_fd = (1<<9),    /* Full duplex mode, disables hearbeat, no loopback */
	csr6_pm = (1<<7),    /* Pass All Multicast */
	csr6_pr = (1<<6),    /* Promiscuous mode */
	csr6_sb = (1<<5),    /* Start(1)/Stop(0) backoff counter */
	csr6_if = (1<<4),    /* Inverse Filtering, rejects only addresses in address table: can't be set */
	csr6_pb = (1<<3),    /* Pass Bad Frames, (1) causes even bad frames to be passed on */
	csr6_ho = (1<<2),    /* Hash-only filtering mode: can't be set */
	csr6_hp = (1<<0),    /* Hash/Perfect Receive Filtering Mode: can't be set */

	csr6_mask_capture = (csr6_sc | csr6_ca),
	csr6_mask_defstate = (csr6_mask_capture | csr6_mbo),
	csr6_mask_hdcap = (csr6_mask_defstate | csr6_hbd | csr6_ps),
	csr6_mask_hdcaptt = (csr6_mask_hdcap  | csr6_trh | csr6_trl),
	csr6_mask_fullcap = (csr6_mask_hdcaptt | csr6_fd),
	csr6_mask_fullpromisc = (csr6_pr | csr6_pm),
	csr6_mask_filters = (csr6_hp | csr6_ho | csr6_if),
	csr6_mask_100bt = (csr6_scr | csr6_pcs | csr6_hbd),
};

enum EEPROMControlBits {
	EE_CS					= 0x01,	// EEPROM chip select
	EE_SHIFT_CLK 	= 0x02,	// EEPROM shift clock

	EE_DATA_WRITE	= 0x04,	// Data from the Tulip to EEPROM
	EE_DATA_READ	= 0x08,	// Data from the EEPROM chip

	EE_WRITE_0		= 0x01,
	EE_WRITE_1		= 0x05,
	EE_READ_CMD		= 0x06,	// includes always-set leading bit

	EE_ENB				= (0x4800 | EE_CS)
};

};	// namespace

#endif
