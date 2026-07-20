/**
 * A cleaned up version of the FDC code, not using templates
 */
#ifndef FDC_H
#define FDC_H

#include "Devices/IOPort.h"

namespace MyOS { namespace Devices { namespace Floppy {

struct FDCStatus;

// FDC version return types (from linux/fdreg.h), sorted on increasing features
enum FDCVERSION {
   FDC_NONE          =  0x00,
   FDC_UNKNOWN       =  0x10, /* DO NOT USE THIS TYPE EXCEPT IF IDENTIFICATION FAILS EARLY */
   FDC_8272A         =  0x20, /* Intel 8272a, NEC 765 */
   FDC_765ED         =  0x30, /* Non-Intel 1MB-compatible FDC, can't detect */
   FDC_82072         =  0x40, /* Intel 82072; 8272a + FIFO + DUMPREGS */
   FDC_82072A        =  0x45, /* 82072A (on Sparcs) */
   FDC_82077_ORIG    =  0x51, /* Original version of 82077AA, sans LOCK */
   FDC_82077         =  0x52, /* 82077AA-1 */
   FDC_82078_UNKN    =  0x5f, /* Unknown 82078 variant */
   FDC_82078         =  0x60, /* 44pin 82078 or 64pin 82078SL */
   FDC_82078_1       =  0x61, /* 82078-1 (2Mbps fdc) */
   FDC_S82078B       =  0x62, /* S82078B (first seen on Adaptec AVA-2825 VLB SCSI/EIDE/Floppy controller) */
   FDC_87306         =  0x63, /* National Semiconductor PC 87306 */
};


/// Represents the floppy controller
class FDC
{
   u8 waitUntilReady() const;
   bool sendByte( u8 byte );   
   bool configure() INITSECTION;
   u8 partId() INITSECTION;

   // Register commands, for inline methods
   enum DIOR {
      DRIVE_A=0, DRIVE_B=1, DRIVE_C=2, DRIVE_D=3,
      ACTIVATE_CONTROLLER=4,  ACTIVATE_DMAIRQ=8,
      MOTOR_A=1<<4,MOTOR_B=1<<5,MOTOR_C=1<<6,MOTOR_D=1<<7
   };

   // Controller version detected
   FDCVERSION version;
   
public:
   inline FDC() : version(FDC_UNKNOWN) {}

   /// Detects the version of the controller, and initializes it
   bool detect() INITSECTION;

   // Called from worker thread, not in INITSECTION
   void initialize();

   void reset() {
      if (version>=FDC_82072A) {
         rateSelectRegister = 0x80 | 0;   // 0 == rate == 500 kbps (?) (fixed for now)
      } else {
         // set   dataOutputRegister = old & ~0x4;
         // delay 20us
         // set   dataOutputRegister = old

         // digitalOutputRegister = 0; ??
      }
   }


   /**
    * Prepare drive for reading or writinr
    * @param drive: A=0, B=1, C=2, D=3
    */
   inline void startMotor( u8 drive )
   {
      // Select drive, enable motor
      digitalOutputRegister = drive | ACTIVATE_CONTROLLER | ACTIVATE_DMAIRQ | (1<<(drive+4));
   }

   // Disable all
   inline void stopMotors()
   {
      digitalOutputRegister = ACTIVATE_CONTROLLER | ACTIVATE_DMAIRQ;
   }

   /// Performs a seek for the given block (on drive A:)
   bool seek( u32 block );

   // shortcut for sendCommand(CHECK_IRQ), could return current cyl
   bool senseIrq();

   /// Reads a 512-byte logical block (assumes seek has been done)
   bool readBlock( u32 block );
   
   /// Reads status, returns number of valid status bytes
   u8 readStatus( FDCStatus& status );

private:
   enum FDCBASE { FDCPRIMARY=0x3f0, FDCSECUNDARY=0x370, base=FDCPRIMARY };

   // Registers
   const IOPort8<base+0> statusRegisterA;
   IOPort8<base+1> statusRegisterB;
   IOPort8<base+2> digitalOutputRegister;
   // base+3 TapeDrive register
   const IOPort8<base+4> mainStatusRegister;
   IOPort8_W<base+4> rateSelectRegister;             // write only
   IOPort8<base+5> dataRegister;
   // base+6 reserved
   const IOPort8<base+7> digitalInputRegister;
   IOPort8_W<base+7> configControlRegister;    // write only
   
};

// After issueing a command, all status bytes must be read
// Number and type depends on the command
struct FDCStatus {
   u8 st0, st1, st2;
   u8 track, head, sector, bytespersector;

   // Note: could have more bytes, max. 16 statusbytes are returned!
   
   // Constructor
   inline FDCStatus()
   {
      // clear some of the bytes
      *((u32*)this) = 0;
   }

   // Interpretation (masks) of the status bytes
   enum ST0 { UNIT_SELECT=0x3, CUR_HEAD=0x4, NOT_READY=0x8, EC=0x10, SEEK_COMPLETE=0x20,
      RESULT_CODE          = 0xC0,
      ST0_OK               = 0<<6,
      ST0_INTERRUPTED      = 1<<6,
      ST0_INVALIDCOMMAND   = 2<<6,
      ST0_ABORTBYPOLLING   = 3<<6
   };

   bool isOK() const { return (st0 & RESULT_CODE) == ST0_OK; }   
};
   
}}}   // namespace

#endif
