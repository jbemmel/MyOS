//
//	PCIRegister classes	(not multi-thread safe!!)
//
//	These registers are typically provided by a PCI Bridge, eg. the Intel 430TX MTXC
//	This device only supports type1 configuration (which is used here)
//
#ifndef PCIREGISTER_H
#define PCIREGISTER_H

#include "Devices/IOPort.h"

namespace MyOS {
namespace Drivers {   
namespace PCI {

class PCIDeviceHandle	// value used to select PCI registers
{
	/* layout:
		type : 2
		register : 6
		function : 3
		unit : 5
		bus : 8
		reserved : 7
		ECD : 1
	*/
	u32 	id;
public:
	inline PCIDeviceHandle( u8 b, u8 s ) : id( 0x80000000 | (b<<16) | (s<<8) ) {}	
	inline operator u32() const { return id; }
} PACKED;


class PCIRegister
{
protected:
	inline void selectRegister ( PCIDeviceHandle id, u8 reg_offset ) 
	{
		// from pci-pc.c, direct access to PCI registers (vs via PCI-BIOS):
      // send a command dword to port 0xCF8
		index = ( id | (reg_offset & ~3) );
	}

    enum { PCI_ADDRESSPORT=0xCF8, PCI_FORWARDPORT=0xCFA, PCI_DATAPORT=0xCFC };
	
    IOPort32< PCI_ADDRESSPORT > index;
};

template <u8 offset, typename T8 = u8 >
class PCI_BYTE : private PCIRegister
{
    // static IOPort8< (PCI_DATAPORT + (offset&3)) > port;
public:
	inline T8 read( PCIDeviceHandle id ) 
	{
		selectRegister( id, offset );
		// u8 v = (u8) port;
		// return (T8) v;
		return (T8) inb( PCI_DATAPORT + (offset&3) );
	}

	inline void write( PCIDeviceHandle id, T8 value ) 
	{
		selectRegister( id, offset );
		// port = value;
		outb( PCI_DATAPORT + (offset&3), value );
	}

	inline T8 readOffset( PCIDeviceHandle id, u8 var_offset )  
	{
		var_offset += offset;					
		selectRegister( id, var_offset );
		IOPort8_v vport( PCI_DATAPORT + (var_offset&3) );
		u8 data = vport;
		return (T8) data;
	}
	
	inline void writeOffset( PCIDeviceHandle id, u8 var_offset, T8 value ) 
	{
		var_offset += offset;
		selectRegister( id, var_offset );
		IOPort8_v vport( PCI_DATAPORT + (var_offset&3) );
		vport = value;
	}	
};

template <u8 offset, typename T16 = u16 >
class PCI_WORD : private PCIRegister
{
	 // static IOPort16 <(PCI_DATAPORT + (offset&2))> port;
public:
	inline T16 read( PCIDeviceHandle id ) 
	{
		selectRegister( id, offset );
		// u16 v = port;
		// return (T16) v;
		return (T16) inw( PCI_DATAPORT + (offset&2) );
	}

	inline void write( PCIDeviceHandle id, T16 value ) 
	{
		selectRegister( id, offset );
		// port = value;
		outw( PCI_DATAPORT + (offset&2), value );
	}
	
	inline T16 readOffset( PCIDeviceHandle id, u8 var_offset ) 
	{
		var_offset += offset;
		selectRegister( id, var_offset );
		IOPort16_v vport( PCI_DATAPORT + (var_offset&2) );
		u16 v = vport;
		return (T16) v;
	}	
	
	inline void writeOffset( PCIDeviceHandle id, u8 var_offset, T16 value ) 
	{
		var_offset += offset;	
		selectRegister( id, var_offset );
		IOPort16_v vport( PCI_DATAPORT + (var_offset&2) );
		vport = value;
	}
	
};
	
template <u8 offset, typename T32 = u32 >
class PCI_DWORD : private PCIRegister
{
	// static IOPort32< PCI_DATAPORT > data;		
public:
	inline T32 read( PCIDeviceHandle id )  
	{
		selectRegister( id, offset );
		// u32 v = data;
		// return (T32) v;
		return (T32) inl( PCI_DATAPORT );
	}

	inline void write( PCIDeviceHandle id, T32 value ) 
	{
		selectRegister( id, offset );
		// data = value;
		outl( PCI_DATAPORT, value );
	}
	
	inline T32 readOffset( PCIDeviceHandle id, u8 var_offset ) 
	{
		var_offset += offset;
		selectRegister( id, var_offset );
		// u32 v = data;
		// return (T32) v;
		return (T32) inl( PCI_DATAPORT );
	}
	
	inline void writeOffset( PCIDeviceHandle id, u8 var_offset, T32 value ) 
	{
		var_offset += offset;
		selectRegister( id, var_offset );
		// data = value;
		outl( PCI_DATAPORT, value );
	}	
};

}}} // namespace

#endif
