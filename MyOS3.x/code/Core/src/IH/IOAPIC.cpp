/*
 * IOAPIC.h
 *
 *  Created on: 14-sep-2008
 *      Author: Jeroen
 */

#include "types.h"
#include "ihtypes.h"    // E_IRQ0
#include "debug.h"

#include "IVirtualMemory.h"
#include "IContext.h"

namespace MyOS {
namespace InterruptHandling {

/**
 * @see http://developer.intel.com/design/chipsets/datashts/29056601.pdf
 *
 * Register select dword, can be relocated using PIIX3 APIC Base Address Relocation Register:
 * 0xFEC0xy00, where x=0-F and y=0,4,8,C
 *
 * Indirect addressing scheme: write index to REGSELECT bit 0..7, then access via IOWIN
 */
struct IOAPIC {
    static const u32 BASE = 0xFEC00000;

    enum E_IOAPIC_REGS {
        E_IOAPIC_ID = 0x0,              // R/W
        E_IOAPIC_Version = 0x1,         // RO
        E_IOAPIC_ArbitrationID = 0x2,   // RO

        E_IOAPIC_IOREDTBL0  = 0x10,     // 24 entries of 64 bits each, R/W
    };

    mutable volatile u32 regSelect;
    u32 _r0, _r1, _r2;
    mutable volatile u32 ioWin;

    /**
     * Reads 4-bit IOApic ID, must be unique within the system.
     * OS is responsible for checking this
     *
     * ID space is shared with local APICs, IOAPIC should get next ID after all local APICs are assigned
     */
    inline u8 readIOAPIC_ID() const {
        regSelect = E_IOAPIC_ID;
        u32 id = ioWin;

        return (id>>24) & 0xF;  // 4 bits
    }

    inline void setIOAPIC_ID( u8 id ) {
        regSelect = E_IOAPIC_ID;
        ioWin = ((id&0xf)<<24);
    }

    inline u8 readIOAPIC_Version() const {
        regSelect = E_IOAPIC_Version;
        u32 version = ioWin;
        return version & 0xff; // default: 00170011h -> 0x11h version
    }

    inline u8 readIOAPIC_maxRedirectionPins() const {
        regSelect = E_IOAPIC_Version;
        u32 version = ioWin;
        return (version>>16) & 0xff; // default: 00170011h -> 0x17h = 24 pins
    }

    inline u8 readIOAPIC_ArbitrationId() const {
        regSelect = E_IOAPIC_ArbitrationID;
        u32 aid = ioWin;
        return (aid >> 24) & 0xf;
    }



    inline void redirectIRQ( u8 entry, u32 bits, u8 destAPICs ) {
        // Write highest bits first(!), IRQ is unmasked when writing to lower bits
        regSelect = E_IOAPIC_IOREDTBL0 + 2*entry + 1;
        ioWin = (destAPICs << 24);
        regSelect = E_IOAPIC_IOREDTBL0 + 2*entry;
        ioWin = bits;
    }

    inline void clearVector( u8 entry ) {
            // Write lowest bits first(!), IRQ is masked when writing to lower bits
            regSelect = E_IOAPIC_IOREDTBL0 + 2*entry;
            ioWin = (1 << 16);
            regSelect = E_IOAPIC_IOREDTBL0 + 2*entry + 1;
            ioWin = 0;
    }

};

enum E_REDIRECT_BITS
{

    /// Delivery modes, also for ICR register of local APIC
    E_FIXED = (0x0 << 8),           //< 000b
    E_LOWEST_PRIORITY = (0x1 << 8),
    E_SMI = (0x2 << 8),             //< 010b, vector must be 0
    E_NMI = (0x4 << 8),             //< 100b, must be edge triggered
    E_INIT = (0x5 << 8),            //< 101b, vector must be set to 0
    E_EXTINT = (0x7 << 8),          //< 111b, compatibility mode

    E_PHYSICAL_MODE = 0,
    E_LOGICAL_MODE = (1 << 11),

    E_DELIVERY_STATUS = (1 << 12), // read-only

    E_HIGH_ACTIVE = 0,
    E_LOW_ACTIVE = (1 << 13),

    E_REMOTE_IRR = (1 << 14), // read-only, indicates local APIC(s) accepted the interrupt

    E_EDGE_TRIGGERED = 0x0,
    E_LEVEL_TRIGGERED = (1 << 15),

    E_MASKED = (1 << 16),
};

// extern
void initIOAPIC( Context::IContext &c )
{
    MM::IVirtualMemory *vm = c.lookup( myos_name_t(MM::IVirtualMemory::ID()) ).castTo<MM::IVirtualMemory>();
    IOAPIC *ioapic = (IOAPIC*) vm->mapMMIO( IOAPIC::BASE, sizeof(IOAPIC) );  // 1 page is enough

    DPRINTK( "\nIOAPIC id=%X version:%X #pins=%d arb-id=%X", ioapic->readIOAPIC_ID(),
            ioapic->readIOAPIC_Version(), ioapic->readIOAPIC_maxRedirectionPins()+1,
            ioapic->readIOAPIC_ArbitrationId() );

    /* Assumption: BIOS setup the redirection tables? No, all 0's with bit 16 set
    for (int n=0; n<5; ++n) {
        ioapic->regSelect = IOAPIC::E_IOAPIC_IOREDTBL0 + 2*n;
        u32 dw_lo = ioapic->ioWin;
        ioapic->regSelect = IOAPIC::E_IOAPIC_IOREDTBL0 + 2*n + 1;
        u32 dw_hi = ioapic->ioWin;
        DPRINTK( "\nBIOS set REDIR[%u] to %x:%x", n, dw_hi, dw_lo );
    }*/

    // Enable IRQ0 (timer), compatibility mode to BSP? Doing this gives 'bogus 0xf vector' in Bochs
    // after reset
    //ioapic->redirectIRQ( 0,
    //        E_IRQ0 | E_EXTINT | E_EDGE_TRIGGERED | E_PHYSICAL_MODE | E_LOW_ACTIVE,
    //        0 );
}

}}
