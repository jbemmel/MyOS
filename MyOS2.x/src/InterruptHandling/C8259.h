#ifndef C8259_H
#define C8259_H

#include "defs.h"
#include "Devices/IOPort.h"
#include "InterruptHandling/IInterruptHandling.h"  // E_IRQVector

namespace MyOS {
namespace InterruptHandling {

/// Programmable Interrupt Controller(PIC) chip, master&slave
class C8259
{
public:
	C8259() INITSECTION;

	void enableIRQ(E_IRQ irqv)
	{
		if (irqv >= E_IRQ8)
		{
			// Threading issue: read-modify-write may result in faulty behaviour. this forces "and [mem], mask"
			
			// SlaveMask &= ~(1 << (irqv-IRQ8));	// clear bit to enable
			mem_write_and( SlaveMask, ~(1 << (irqv-E_IRQ8)));
			SLAVE_DATA = SlaveMask;
		}
		else
		{
			// MasterMask &= ~(1 << (irqv-IRQ0));
			mem_write_and( MasterMask, ~(1 << (irqv-E_IRQ0)));
			MASTER_DATA = MasterMask;
		}
	}

	void disableIRQ(E_IRQ irqv)
	{
		if (irqv >= E_IRQ8)
		{
			// SlaveMask |= (1 << (irqv-IRQ8));	// set bit to disable
			mem_write_or( SlaveMask, (1 << (irqv-E_IRQ8)));
			SLAVE_DATA = SlaveMask;
		}
		else
		{
			// MasterMask |= (1 << (irqv-IRQ0));
			mem_write_or( MasterMask, (1 << (irqv-E_IRQ0)));
			MASTER_DATA = MasterMask;
		}
	}

    inline u16 getMasks() const {
        // in order of bits, so bit0 == IRQ0
        return (SlaveMask<<8) | MasterMask;
    }	
	
private:
	/// Interrupt masks, '1' means *disabled*
	unsigned char MasterMask, SlaveMask;

	// Control port addresses: IO = IRR, ISR, vector	DATA = IMR, ICW3, ICW4, OCW1
	IOPort8< 0x20 > MASTER_IO;
	IOPort8< 0x21 > MASTER_DATA;
	IOPort8< 0xa0 > SLAVE_IO;
	IOPort8< 0xa1 > SLAVE_DATA;	

	enum ICW1 { ICW4_FOLLOWS = (1<<0), NO_CASCADE = (1<<1), LEVEL_TRIGGERING = (1<<3), BIT_ON = (1<<4) };
	enum ICW4 { X86MODE = (1<<0), AutoEOI = (1<<1), IS_MASTER_BUFFER = (1<<2), BUFFERED = (1<<3), FULLY_NESTED = (1<<4) };
};

}}   // namespace

#endif //C8259_H
