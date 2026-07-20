//
//	Configuration structure as defined by the PCI standard(s)
//

#ifndef PCICONFIG_H
#define PCICONFIG_H

// type definitions for interpreting PCI information, in order of appearance
#include "pci_ids.h"
#include "types.h"
#include "Devices/PCI/PCIRegister.h"

// Treat type <T> as (basic) type b
#define TREAT_AS(b, T) \
   inline T( b v=0 )                { *((b*)this)=v; }      \
   inline operator b() const        { return *((b*)this); } \
   inline void operator =( b t )    { *((b*)this) = t; }

namespace MyOS {
namespace Drivers {   
namespace PCI {

struct PCI_HEADER_TYPE {
	enum { TYPE0=0, PCI_PCI_BRIDGE=1, CARDBUS=2 } type : 2;
	u8 __undefined : 5;
	bool multi_function : 1;
	
	PCI_HEADER_TYPE( u8 v = 0) { *((u8 *)this) = v; }
} PACKED;

/// Built-in Self Test
enum PCI_BIST { BIST_RESULT_MASK=0xF, BIST_START=0x40, BIST_CAPABLE=0x80 } ;

struct PCI_SLOT_FUNC {
	u8 function : 3;	// for multi-function devices
	u8 slot     : 5;

	TREAT_AS( u8, PCI_SLOT_FUNC );	
} PACKED;

// PCI command definitions, see also AoPC HW p. 496
struct PCI_COMMAND {
	bool enableIO			: 1;	// Enable response in I/O space
	bool enableMEM			: 1;	// Enable response in Memory space
	bool MASTER				: 1;	// Enable bus mastering
	bool SPECIAL			: 1;	// Enable response to special cycles
	bool MWI					: 1;	// Use memory write and invalidate
	bool VGA_PALETTE 		: 1;	// Enable palette snooping
	bool PARITY				: 1;	// Enable parity checking
	bool WAIT 				: 1;	// Enable address/data stepping
	bool SERR				: 1;	// Enable SERR
	bool FAST_BACK2BACK  : 1;	// Enable back-to-back writes
	u8 __reserved__      : 6;

	TREAT_AS( u16, PCI_COMMAND );
} PACKED;

struct PCI_STATUS {
	u32 __reserved__                           : 4;
	bool PCI_STATUS_CAP_LIST 			       : 1;	///< Supports Capability List
	bool PCI_STATUS_66MHZ					   : 1;	///< Supports 66 Mhz PCI 2.1 bus
	bool PCI_STATUS_UDF						   : 1;	///< Support User Definable Features [obsolete]
	bool PCI_STATUS_FAST_BACK			       : 1;	///< Accept fast-back to back
	bool PCI_STATUS_PARITY				       : 1;	///< Detected parity error
	enum { FAST=0, MEDIUM=1, SLOW=2 } devsel   : 2; ///< Selects speed
	bool PCI_STATUS_SIG_TARGET_ABORT 		   : 1;	///< is set on target abort      const ?
	bool PCI_STATUS_REC_TARGET_ABORT 		   : 1;	///< Master ack of "
	bool PCI_STATUS_REC_MASTER_ABORT 		   : 1;	///< Set on master abort
	bool PCI_STATUS_SIG_SYSTEM_ERROR 	       : 1;	///< Set when we drive SERR      const ?
	bool PCI_STATUS_DETECTED_PARITY 		   : 1;	///< Set on parity error

	TREAT_AS( u16, PCI_STATUS );
} PACKED;

enum PCI_INTERRUPT { PCI_NONE=0, PCI_INTA=1, PCI_INTB=2, PCI_INTC=3, PCI_INTD=4 };

// PCI capabilities
enum PCI_CAPABILITY {
	INVALID			       = 0
	,  PCI_CAP_ID_PM       = 0x01	/* Power Management */
	,  PCI_CAP_ID_AGP	   = 0x02	/* Accelerated Graphics Port */
	,  PCI_CAP_ID_VPD	   = 0x03	/* Vital Product Data */
	,  PCI_CAP_ID_SLOTID   = 0x04	/* Slot Identification */
	,  PCI_CAP_ID_MSI	   = 0x05	/* Message Signalled Interrupts */
	,  PCI_CAP_ID_CHSWP    = 0x06	/* CompactPCI HotSwap */
	,  PCI_END_OF_LIST     = 0x0F
};

struct PCI_MEM_INFO {
	bool isIO         : 1;	// 0==memory space, 1==IO space
	bool below1M      : 1;	// must be below 1M, only for memory, reserved when IO
	bool bit64        : 1;	// 0==32bit, 1==64bit, only valid for memory
	bool prefetchable : 1;	// only valid for memory
	u32 address       : 28;	// the rest

	inline u32 getAddress() const 	{ return isIO ? (*((int*) this) & ~0x3):(*((int*) this) & ~0xF); }

	TREAT_AS( u32, PCI_MEM_INFO );	
};
	
struct PCI_ROM_INFO {
	bool enable : 1;
	u32 _reserved : 9;
	u32 address : 22;
};

// Power management: offset to registers
enum PCI_POWER_MANAGEMENT 	{ PMCapRegister=2, PMControlRegister=4, PMPPBExtensions=6, PMData=7 };

struct PCI_PM_CONTROL {
	enum PCI_POWERSTATE { D0=0, D1, D2, D3 } powerstate : 2;
	u8 __reserved__   : 6;
	bool PME_enable   : 1;
	u8 dataselect     : 4;		// ??
	u8 datascale      : 2;		// ??
	bool PME_status   : 1;
	
	TREAT_AS( u16, PCI_PM_CONTROL );
} PACKED;

enum BRIDGE_CONTROL {
   PARITY         =  0x01,
   SERR           =  0x02,
   NO_ISA         =  0x04,
   VGA            =  0x08,
   MASTER_ABORT   =  0x20,
   SEC_BUS_RESET  =  0x40,
   FAST_BACK2BACK =  0x80
};
	
//
//	PCI device information is a 256-byte block per device, identified by (bus,slot)
//
		
extern struct PCI_DEFINITION_STATIC
{
	PCI_WORD<0x00, E_PCI_ID>           vendor;
	PCI_WORD<0x02, E_PCI_ID >          device;
	PCI_WORD<0x04, PCI_COMMAND>        command;
	PCI_WORD<0x06, PCI_STATUS>         status;
	PCI_BYTE<0x08>                     revision;
	PCI_BYTE<0x09>                     reg_prog;
	PCI_WORD<0x0A, E_PCI_CLASS >       device_class;
	PCI_BYTE<0x0C>                     cache_line_size;
	PCI_BYTE<0x0D>                     latency_timer;
	PCI_BYTE<0x0E, PCI_HEADER_TYPE>    header_type;
	PCI_BYTE<0x0F, PCI_BIST>           built_in_self_test;
	PCI_DWORD<0x10, PCI_MEM_INFO>      base_address0;
	
	struct HEADER_TYPE_0 {
		PCI_DWORD<0x14, PCI_MEM_INFO> base_address1;
		PCI_DWORD<0x18, PCI_MEM_INFO> base_address2;
		PCI_DWORD<0x1C, PCI_MEM_INFO> base_address3;
		PCI_DWORD<0x20, PCI_MEM_INFO> base_address4;
		PCI_DWORD<0x24, PCI_MEM_INFO> base_address5;
		PCI_DWORD<0x28>               cardbus_CIS;
		PCI_WORD<0x2C, E_PCI_ID>      subsystem_vendor;
		PCI_WORD<0x2E, E_PCI_ID>      subsystem_device;
		PCI_DWORD<0x30, PCI_MEM_INFO> ROM_address;
		PCI_BYTE<0x34>			      first_capability;
		PCI_BYTE<0x3C>			      interrupt_irq;
		PCI_BYTE<0x3D, PCI_INTERRUPT> interrupt_pin;
		/*const*/ PCI_BYTE<0x3E> minimumGrantTimer;	  // read-only : indicates minimum burst length in units of 1/4 us (@33Mhz)
		/*const*/ PCI_BYTE<0x3F> max_latency;		  		// read-only, indicates how often PCI access is needed (in 1/4 us units)

        // not standard, VIA uses these
        PCI_BYTE<0x40> misc_control_1;
        PCI_BYTE<0x41> misc_control_2;        
        // For Via 686A/B, to solve a quirk (see linux pci/quirks.c)
        PCI_BYTE<0x42> sci_irq_line;
	} type0;

	struct HEADER_TYPE_1 {		// PCI-PCI bridges
		PCI_DWORD<0x14, PCI_MEM_INFO>	base_address1;
		PCI_BYTE<0x18>		primary_bus;
		PCI_BYTE<0x19>		secondary_bus;
		PCI_BYTE<0x1A>		subordinate_bus;
		PCI_BYTE<0x1B>		sec_latency_timer;
		PCI_DWORD<0x1C>		IOBaseLimit32;		// linux reads this in 1 time
		PCI_BYTE<0x1C>		IOBase;			
		PCI_BYTE<0x1D>		IOLimit;
		PCI_WORD<0x1E>		secondary_status;
		PCI_DWORD<0x20>		MEMBaseLimit32;		// idem
		PCI_WORD<0x20>		MEMBase;
		PCI_WORD<0x22>		MEMLimit;
		PCI_WORD<0x24>		MEMPrefetchableBase;
		PCI_WORD<0x26>		MEMPrefetchableLimit;
		PCI_DWORD<0x28>		MEMPrefetchableBase_upper32;
		PCI_DWORD<0x2C>		MEMPrefetchableLimit_upper32;
		PCI_WORD<0x30>		IOBase_upper16;
		PCI_WORD<0x32>		IOLimit_upper16;		
		PCI_BYTE<0x34>		first_capability;		
	} pcibridge;
	
	struct HEADER_CB {
		PCI_BYTE<0x14>			first_capability;			
	} cardbus;
	
} PCICONFIG_FIXED;

// read/write this data using readOffset / writeOffset
extern struct PCI_DEFINITION_VARIABLE	
{
	PCI_BYTE<0,PCI_CAPABILITY>			capabilities; // [ 48*4 ]  read/write using readOffset / writeOffset
	PCI_WORD<PMCapRegister> 			PMCapabilities;	
	PCI_WORD<PMControlRegister, PCI_PM_CONTROL> 	PMControl;	

} PCICONFIG_VAR;

}}}	// namespace

#endif	// PCICONFIG_H
