/**
 * Defines USB UHCI 1.1 compliant IO port offsets
 * 
 * @see REF: ftp://download.intel.com/technology/usb/UHCI11D.PDF
 *
 * Summary
 * -------
 * 
 * USB defines four types of transfer modes:
 * - isochronous	: real-time streaming, no retries (all others are retried)
 * - interrupt		: one way - from device - only
 * - control		: status, setup, control
 * - bulk			: guaranteed
 *
 * Within each 1-ms USB frame, the host controller (HC) will try to send data in the above order
 * (i.e. ISO, INT, CTRL, BULK). The Start-of-frame (SOF) can be tweaked
 *
 * ISO transfers are immediate, all others are queued
 */

#ifndef USBIO_H
#define USBIO_H

namespace MyOS { namespace Devices { namespace USB {

enum E_UHCI_COMMAND {
    E_ZERO      = 0,
	E_RUN		= (1<<0),
	E_HCRESET	= (1<<1),
	E_GRESET	= (1<<2),

	E_CFG_DONE	= (1<<6),
	E_MAXPKT64	= (1<<7),
};

enum E_UHCI_STATUS {
	E_USBINT	= (1<<0),	// USB transaction completed, IOC bit was set
	E_USBERROR	= (1<<1),
	E_RESUMED	= (1<<2),
	E_HOSTERR	= (1<<3),	// serious error, eg PCI aborts
	E_PROCERR	= (1<<4),	// error processing packet list, illegal value
	E_HALTED	= (1<<5),	// controller has halted, Linux: R/O not RW/C
};

enum E_UHCI_PORTSTATUS {
    E_DEV_PRESENT   = (1<<0) ,
    E_CONN_CHANGED  = (1<<1) ,
    E_ENABLED       = (1<<2) ,
    E_ENA_CHANGED   = (1<<3) ,
    E_RESUME        = (1<<6) ,  // can be set by both software and HC
    E_IS_LOW_SPEED  = (1<<8) ,
    E_PORT_RESET    = (1<<9) ,
    E_SUSPENDED     = (1<<12),
};


/**
 * I/O register offsets from USB I/O base (read from PCI config)
 * Not using IOPort16 etc since that would use needless objects... 
 *
 * @see page 16 of REF
 */
enum E_UHCI_REGOFFSETS
{
	E_CMD		= 0x00,		// word registers
	E_STATUS	= 0x02,		// write 0xffff resets
	E_INTENA	= 0x04,
	E_FRAMENO	= 0x06,

	// 32 bit
	E_FRAMELISTBASE	= 0x08,

	// 8 bit
	E_SOFMODIFY		= 0x0C,	// 0..127 for 11936..12063 12Mhz steps

	// 16 bit, write 0xffff resets
	E_PORT1SC		= 0x10,     // IIUC, this is the upstream facing port
	E_PORT2SC		= 0x12,     // IIUC, this is the downstream port; device attaches here
};

// Frame list pointer format, HC won't change these
struct USB_FLP {
	int isInvalid  : 1;
	int isQH	   : 1;	// if 0, this item points to a Transfer Decriptor (TD)
	int _res_      : 2;
	int flp        : 28;
} PACKED;

enum E_PACKET_ID {
	E_IN	= 0x69,
	E_OUT	= 0xe1,
	E_SETUP	= 0x2d,
};

// USB Transfer descriptor. Note: This may not be the most convenient way of programming
struct USB_TD {
    union {    
       u32 next;
       struct {
        	int isLast		: 1;
        	int isQH		: 1;	// if 0, this item points to a Transfer Decriptor (TD)
        	int depthFirst	: 1;
        	int _res_		: 1;
        	int linkptr		: 28;
       } PACKED;
    };

	// This data is modified by the HC
    union {
        volatile u32 result;       
        struct {       
        	volatile int actualLen	: 11;
        	volatile int _res2_		: 5;
        	volatile int	status	: 8;	// various bits
        	volatile int	control	: 8;	// various bits
        } PACKED;
    };                    

	// packet header fields
    union {
        u32 ph;
        struct {             
        	E_PACKET_ID packetId	: 8;
        	int device		: 7;
        	int endpoint	: 4;
        	int dataToggle	: 1;	// 1-bit sequence number
        	int _res3_		: 1;
        	int maximumLen	: 11;	// n-1, so 0 means max. 1 byte packet
        } PACKED;           
    };
	u32 /* MM::physadr_t */ dataBuffer;	// phys. address of buffer, must be >= maxLen
  
    // OS specific fields

} __attribute__(( aligned(16), packed ));

#define PH(i,d,e,t,m) ( (i) | ((d)<<8) | ((e)<<15) | ((t)<<19) | ((m)<<21) )

enum E_SPECIAL_VALUES {
	E_NULL_DATA = 0x7FF,	// Value for the maxLen field for NULL-data packets
};

struct USB_QH {
	u32 nextQueue;
	u32 nextElement;
} __attribute__(( aligned(16), packed ));

enum E_RQTYPE_BITS {
    E_HOST_TO_DEV   = 0x00,
	E_DEV_TO_HOST	= 0x80,
};


// see Table 9-4 USB 1.1 specs
enum E_USB_REQUEST {
	E_GET_STATUS		= 0x00,	
	E_CLEAR_FEATURE		= 0x01,

	E_SET_FEATURE		= 0x03,

	E_SET_ADDRESS		= 0x05,
	E_GET_DESC			= 0x06,
	E_SET_DESC			= 0x07,
	E_GET_CONFIG		= 0x08,  // returns a 1-byte index
	E_SET_CONFIG		= 0x09,
	E_GET_INTF			= 0x0A,
	E_SET_INTF			= 0x0B,
	E_SYNC_FRAME		= 0x0C,
};

// 8-byte structure
struct USB_CONTROL_PACKET {
	E_RQTYPE_BITS rqTypeBits	: 8;
	E_USB_REQUEST request		: 8;
	u16 value;
	u16 index;
	u16 bytecount;
} PACKED;

// see USB2.0 Table 9-8: Standard device descriptor
struct USB_DEV_DESCRIPTOR {
    u8  length;         // size of this descriptor, in bytes
    u8  type;
    u16 usbVersion;     // 0x110 for 1.1
    union {
       u16 classId;
       struct {
            u8  deviceClass;    
            u8  deviceSubclass;
       } PACKED;
    };                   
    u8  deviceProtocol;
    u8  maxControlPacketSize;   // 8,16,32 or 64
    
    u16 vendorId;
    u16 productId;  // see http://www.linux-usb.org/usb.ids
    u16 release;
    u8  manufacturerStringIndex;
    u8  productStringIndex;
    u8  serialStringIndex;
    u8  configCount;
} PACKED;


}}} // namespace

#endif
