#ifndef CMOS_H
#define CMOS_H

#include "../IOPort.h"

namespace MyOS {
namespace Devices {

/**
 * Utility class, not used yet
 * 
 * The CMOS (complementary metal oxide semiconductor) memory is actually
 * a 64 or 128 byte battery-backed RAM memory module that is a part of the
 * system clock chip. 
 * 
 * @see http://bochs.sourceforge.net/techspec/CMOS-reference.txt
 */
class CMOS 
{
public:
    enum E_CMOS_PORT {
        RTC_SECONDS         = 0x0,
        RTC_SECOND_ALARM    = 0x1,
        RTC_MINUTES         = 0x2,
        RTC_HOURS           = 0x4,
        RTC_DAY_OF_WEEK     = 0x6,
        RTC_DATE_OF_MONTH   = 0x7,
        RTC_MONTH           = 0x8,
        RTC_YEAR            = 0x9,
        
        STATUS_REG_A        = 0xA,  //< Read/write
        STATUS_REG_B        = 0xB,  //< Read/write
        STATUS_REG_C        = 0xC,  //< Read-only? (bit 7 triggers an IRQ8), rest are interrupt status flags
        STATUS_REG_D        = 0xD,  //< Read-only, bit 7 indicates CMOS battery good
    };

    inline u8 readData( E_CMOS_PORT port ) volatile {
        select = (u8) port;
        return data;
    }
    
    inline void writeData( E_CMOS_PORT port, u8 d ) volatile {
        select = (u8) port;
        data = d;
    }
    
    inline void enableNMI() volatile {
        select = 0x00;  // clear bit7
    }

    inline void disableNMI() volatile {
        select = 0x80;  // set bit7
    }
    
    /**
     * Checks whether non-maskable interrupt (NMI) is enabled
     */
    inline bool isNMIEnabled() const volatile {
        return (select & 0x80) == 0;
    }
    
private:
    volatile IOPort8<0x70> select;   //< bits 0-6 used for addressing (so 2^7=128 bytes addressable)
                                     //< Bit 7 is used to mask "non-maskable" interrupts (NMI)
    volatile IOPort8<0x71> data;  
};

}
}

#endif
