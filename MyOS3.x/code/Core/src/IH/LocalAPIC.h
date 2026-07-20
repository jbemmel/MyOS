#ifndef LOCALAPIC_H
#define LOCALAPIC_H

#include "memtypes.h"
#include "IContext.h"
#include "CPU/Processor.h"
#include "ihtypes.h"

namespace MyOS {
namespace InterruptHandling {

class LocalAPIC {

   // @return false if CPU does not support it
   friend class IHComponent;
   static bool initOnce( IInterruptHandling& ih ) INITSECTION;

   static void deinit();

public:
    static bool init() INITSECTION;


    enum E_DCR_VALUE
    {
        E_DIV_1 = 0xB, // 1011 binary, fastest
        E_DIV_2 = 0x0, // 0000 binary (initial value at startup)
        E_DIV_4 = 0x1, // 0001 binary
        E_DIV_8 = 0x2, // 0010 binary
        E_DIV_16 = 0x3, // 0011 binary
        E_DIV_32 = 0x8, // 1000 binary
        E_DIV_64 = 0x9, // 1001 binary
        E_DIV_128 = 0xA,
        // 1010 binary, slowest
    };

   static void initTimer( E_DCR_VALUE divisor ) INITSECTION;

   inline static void setTimer( u32 value ) {
       apic->initialCount = value;
   }

   inline static u32 getCurrentCount() {
       return apic->currentCount;
   }

   inline static bool isPresent() { return apic!=0; }

   /**
    * Returns the local APIC ID
    *
    * @return 4-bits (Pentium) or 8-bits (Pentium4) local APIC ID
    */
   inline static u32 getID() {
       return (apic->localId)>>24;
   }

   static void sendIPI( cpu_id_t cpumask, E_INTCODES vector );

   inline static void sendIPIToALL( E_INTCODES vector ) {
       sendIPI( 0xffffffff, vector );
   }

   static void startOtherCPU( u32 physicalAPIC_ID );

   inline static void ACK_IRQ() {
       apic->eoi = 0;
   }

private:
   // Some I/O registers, memory mapped
   enum {
       APIC_BASE           = 0xFEE00000,  // Memory mapped base, programmable
   };

   // some utility functions, adopted from linux
   static inline u8 getMaxLVT();
   static void clear();

   static bool waitICRDone();

   static void onSpurious();
   static void onError();

   // Static, addressed from static handler function
   struct LOCALAPIC
   {
       const u32 reserved1[8];
       struct
       {
           u32 localId;
           const u32 reserved2[3];
       };
       struct
       {
           const u32 version; // read-only, lower 8 bits are version, bits 16..23 = maxLVT entries -1
           const u32 reserved3[3];
       };
       const u32 reserved4[16];

       struct
       {
           u32 tpr; //< Task Priority Register, Only lowest 8 bits. 0x00 means accept all, 0xff block all
           const u32 reserved45[3];
       };

       struct
       {
           const u32 apr; //< Arbitration Priority Register, read-only. Only lowest 8 bits
           const u32 reserved46[3];
       };

       struct
       {
           const u32 ppr; //< Processor Priority Register, read-only. Only lowest 8 bits
           const u32 reserved47[3];
       };

       struct
       {
           u32 eoi; // write-only, offset 0x2C * 4
           const u32 reserved6[3];
       };
       const u32 reserved7[4];

       struct
       {
           u32 ldr; //< Logical Destination
           const u32 reserved8[3];
       };

       struct
       {
           u32 dfr; //< Destination Format Register
           const u32 reserved85[3];
       };

       struct
       {
           u32 sivr; // initialized to 0xFF at startup, which means timer OFF!
           const u32 reservedx[3];
       };

       struct {
           const u32 bits;
           const u32 _reserved[3];
       } isr[8];   // offset 0x100..0x170

       struct
       {
           const u32 bits;
           const u32 _reserved[3];
       } tmr[8];   // offset 0x180..

       struct
       {
           const u32 bits;
           const u32 _reserved[3];
       } irr[8];   // offset 0x200..

       struct
       {
           volatile u32 errorStatus; // read-only according to Intel table, but linux wr
           // Due to the Pentium erratum 3AP, see also Intel docs 8.5.3
           const u32 reserved9[3];
       };
       const u32 reserved10[28];
       struct
       {
           u32 icr_lo; // 0xc0
           const u32 reserved11[3];
       };
       struct
       {
           u32 icr_hi; // 0xc4
           const u32 reserved12[3];
       };

       /**
        * And finally, the interesting stuff...
        */
       struct
       {
           u32 timerLVT; // 0xc8, byte offset 0x320
           const u32 reserved13[3];
       };
       struct
       {
           u32 thermalSensor; // 0xcc
           const u32 reserved14[3];
       };
       struct
       {
           u32 perfMonitoring; // 0xd0
           const u32 reserved15[3];
       };
       struct
       {
           u32 lint0; // 0xd4
           const u32 reserved16[3];
       };
       struct
       {
           u32 lint1; // 0xd8
           const u32 reserved17[3];
       };
       struct
       {
           u32 lvtError; // 0xdc
           const u32 reserved18[3];
       };
       struct
       {
           u32 initialCount;
           const u32 reserved19[3];
       };
       struct
       {
           const u32 currentCount;
           const u32 reserved20[3];
       };
       const u32 reserved24[16];
       struct
       {
           u32 dcr; // one of the E_DCR_VALUES
           const u32 reserved25[3];
       };
       const u32 reserved26[4];

       /**
        * Reads the error status of the APIC
        */
       inline u32 getErrorStatus() volatile
       {
           this->errorStatus = 0; // need to write arbitrary value before reading
           return this->errorStatus;
       }

       inline u32 setTaskPriority( u32 priority ) volatile {
           u32 curTPR = this->tpr;
           this->tpr = priority;
           return curTPR;
       }

       inline void restoreTaskPriority( u32 prev ) volatile {
           this->tpr = prev;
       }
   };

   static volatile LOCALAPIC* apic ASMNAME( "apic" );
};

}} // namespace

#endif
