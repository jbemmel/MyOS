/***************************************************************************
                          FloppyController.h  -  description
                             -------------------
    begin                : Thu Feb 14 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef FLOPPYCONTROLLER_H
#define FLOPPYCONTROLLER_H

#include "Devices/IOPort.h"

namespace MyOS { namespace Devices { namespace Floppy {

enum FDC { FDCPRIMARY=0x3f0, FDCSECUNDARY=0x370 };

/// @see FloppyController::setDataRate
enum E_RATE {
   E_500Kbps   = 0,
   E_300Kbps   = 1,
   E_250Kbps   = 2,
   E_1Mbps     = 3,
};

// Controller command, must be written sequentially to dataregister
struct ControllerCommand {
   enum COMMAND {
      // upper 3 bits, only used for some commands
      SKIP_ERASED          = 1<<5,
      USE_MFM              = 1<<6,
      REPEAT_OTHER_HEAD    = 1<<7,

      NONE=0,
      // ??
      READ_TRACK           = 0x2 | USE_MFM,     // read 1 head, 18 sectors = 18*512 bytes
      SET_DRIVE_DATA       = 0x3,      // 3 byte command, no result
      DRIVE_STATUS         = 0x4,      // 2 byte command, 1 byte result
      WRITE_SECTOR         = 0x5 | USE_MFM | REPEAT_OTHER_HEAD,
      READ_SECTOR          = 0x6 | SKIP_ERASED | USE_MFM /*| REPEAT_OTHER_HEAD*/,
      RECALIBRATE_DRIVE    = 0x7,      // move to track 0
      CHECK_IRQSTATUS      = 0x8,      // 1 byte command, 2 byte result
      WRITE_ERASED_SECTOR  = 0x9 | USE_MFM,
      READ_SECTOR_ID       = 0xA | SKIP_ERASED | USE_MFM | REPEAT_OTHER_HEAD,    // 2 byte command
      // ??
      READ_ERASED_SECTOR   = 0xC,
      FORMAT_TRACK         = 0xD | USE_MFM,     // 6 byte command
      DUMP_REGISTERS       = 0xE,
      SEEK_TRACK           = 0xF,      // 3 byte command, no result -> use CHECK_IRQSTATUS

      // extended commands
      CONTROLLER_VERSION   = 0x10,                       // 1 byte command
      SET_PERPENDICULAR    = 0x12,                       // 2 bytes, only supported on >= FDC_82077
      FIFO_CONFIG          = 0x13,
      FIFO_CONFIG_UNLOCK   = 0x14,
      PARTID               = 0x18,
      FIFO_CONFIG_LOCK     = 0x94,
      VERIFY               = 0x15 | USE_MFM,       // 9 byte command
      SEEK_OUTWARD         = 0x8F,  // bit 6 = DIR
      SEEK_INWARD          = 0xCF,  // to higher track numbers

      _COMMANDMASK_        = 0x1F,
   } command : 8;

   static const u8 commandsizes[_COMMANDMASK_+1];

   // Invalid opcodes return a 1-byte result
   enum { INVALID_RESULT=0x80 };
   static const u8 resultsizes[_COMMANDMASK_+1];

   enum DRIVESELECT { DA=0, DB=1, DC=2, DD=3 } select : 2;
   enum HEAD { HEAD0=0, HEAD1=1 } headx : 1;
   /* const */ u8 __reserved__ : 5;
   u8 track;   // 0..79
   u8 head;    // 0..1
   u8 sector;  // 1..18
   enum E_SECTORSIZE { BPS_256=1, BPS_512=2 } sectorsize : 8;
   enum E_TRACKLENGTH { KB_360=9, KB_720=9, MB_1_2=15, MB_1_44=18 } tracklength : 8;
   enum E_GAP { DEFAULTGAP=27, INCH5_25_FORMAT=80, INCH3_5_FORMAT=84 } gap3 : 8;
   u8 datalength;

   // no constructor, use setCommand(s)
   void setCommand( COMMAND type, DRIVESELECT drive, u32 logicalblock );
   void setCommand( COMMAND type ) {
      command = type;
   }
   
   // specific for seek
   inline void setSeek( DRIVESELECT drive, u32 lb ) {
      command = SEEK_TRACK;
      select = drive;

      // Seek only needs head & track
      track  = lb/36;               // 0..79
      headx = (HEAD) ((lb/18)&1);
   }
   
} PACKED;

// After issueing a command, all status bytes must be read
// Number and type depends on the command
struct ControllerStatus {
   u8 st0, st1, st2;
   u8 track, head, sector, bytespersector;

   // Interpretation (masks) of the status bytes
   enum ST0 { UNIT_SELECT=0x3, CUR_HEAD=0x4, NOT_READY=0x8, EC=0x10, SEEK_COMPLETE=0x20,
      RESULT_CODE          = 0xC0,
      ST0_OK               = 0<<6,
      ST0_INTERRUPTED      = 1<<6,
      ST0_INVALIDCOMMAND   = 2<<6,
      ST0_ABORTBYPOLLING   = 3<<6
   };

   enum ST1 {
      E_OK=0,
      E_NO_ID              = 1<<0,  // no address markings
      E_NOT_WRITABLE       = 1<<1,
      E_NO_DATA            = 1<<2,
      E_TIMEOUT            = 1<<4,  // DMA or CPU was too late with signalling
      E_CRC_ERROR          = 1<<5,  // in ID field or data field
      E_CYLINDER_END       = 1<<7
   };

   enum ST2 {
      E_NO_ADDR_MARK    = 1<<0,
      E_BAD_CYLINDER    = 1<<1,  // ID address marks dont match controller (0xff)
      E_SEEK_ERROR      = 1<<2,
      N_SEEK_OK         = 1<<3,
      E_WRONG_CYLINDER  = 1<<4,
      E_CRC_DATA_ERROR  = 1<<5,
      E_ERASED_MARKING  = 1<<6
   };

   // Constructor
   ControllerStatus()
   {
      // clear some of the bytes
      *((u32*)this) = 0;
   }

   bool isOK() const { return (st0 & RESULT_CODE) == ST0_OK; }
};


// GCC 3.0.3 doesnt implement "non-trivial labels"
static const ControllerCommand DEFAULTVALUES = {
   /* command        : */ ControllerCommand::NONE,
   /* driveSelect    : */ ControllerCommand::DA,
   /* headSelect     : */ ControllerCommand::HEAD0,   // must be equal to head byte
   /* __reserved__   : */ 0,
   /* cyl_hd_sec     : */ 0,0,0,
   /* sectorsize     : */ ControllerCommand::BPS_512,
   /* tracklength    : */ ControllerCommand::MB_1_44,
   /* gap            : */ ControllerCommand::DEFAULTGAP,
   /* datalength     : */ 0xff   // not used for whole sectors
};

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


/**
 *    This template defines FC registers, and methods to read/write them
 */
template <FDC base>
class FloppyController
{

   enum {
      MAXTIMEOUT=10000,    // max. for-loop repeat to wait for status, linux
      NOMINAL_DTR=500,

      // parameters for a 3_1/2  inch HD
      HLT = 16,      // head load time (ms)
      HUT = 16,      // head unload time (ms)
      SRT = 4000     // max. step rate interval (usec)
   };

public:
   // Version should be detected, for now fixed
   FloppyController() : version(FDC_8272A) {}

   ~FloppyController() {
      // stop all motors & controller forcefully
      digitalOutputRegister = 0x0;
   }

   void initialize();
   void specifyRate( u8 rate );

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
   inline void stopMotor()
   {
     digitalOutputRegister = ACTIVATE_CONTROLLER | ACTIVATE_DMAIRQ;
   }

   /**
    *    Issues a command to the controller
    */
   bool issueCommand( const ControllerCommand &command ) {
      size_t csize = ControllerCommand::commandsizes[command.command & ControllerCommand::_COMMANDMASK_];
      return issueCommand( (u8*) &command, csize );
   }

   /**
    *    Use when an IRQ is detected, polling mode (can be disabled)
    *
   u16 checkirq() {
      // u32 timeout = MAXTIMEOUT;
      // while ( (mainStatusRegister&(DR_READY|IO_TO_CPU) != (DR_READY)) && --timeout );
      // if (timeout==0) return 0xFF;
      dataRegister = ControllerCommand::CHECK_IRQSTATUS;
      u8 st0 = mainStatusRegister;     // read code
      u8 d = mainStatusRegister;       // size==2 bytes
      return (d<<8) | st0;
   }
   */
   
   // shortcut for sendCommand(CHECK_IRQ), could return current cyl
   bool senseIrq();

   // shortcut for sendCommand(CONFIGURE), this aborts if unknown by FDC
   bool configure();

   // sends PARTID command
   u8 partid();

   /**
    * Read status bytes after a command. Their number depends on the type of command
    * This should be used in the interrupt handler
    */
   u8 readStatus( ControllerStatus &status );

   /**
    * Reads the status register (independent of command result)
    */
   u8 getStatus() const {
      return mainStatusRegister;
   }

   bool isBusy() const {
      return getStatus() & BUSY;
   }

   void setDataRate( E_RATE rate ) {
      configControlRegister = rate;
   }

   bool diskChanged() const {
      return digitalInputRegister & DISK_CHANGE;
   }

private:
   u8 waitUntilReady() const;
   bool issueCommand( const u8 *command, size_t csize );
   bool sendByte( u8 byte );

   // Controller version, should be determined dynamically
   FDCVERSION version;

   // Cache of last SPECIFY command issued
   u8 lastspecifycommand[4];

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

   // Register commands
   enum DIOR {
      DRIVE_A=0, DRIVE_B=1, DRIVE_C=2, DRIVE_D=3,
      ACTIVATE_CONTROLLER=4,  ACTIVATE_DMAIRQ=8,
      MOTOR_A=1<<4,MOTOR_B=1<<5,MOTOR_C=1<<6,MOTOR_D=1<<7
   };

   enum MAINSTATUS {
      MOVING_A=1<<0, MOVING_B=1<<1, MOVING_C=1<<2, MOVING_D=1<<3,
      BUSY=1<<4, DMA_DEACTIVATED=1<<5, IO_TO_CPU=1<<6, DR_READY=1<<7
   };

   enum DIR {
      LOWDENSITY_RATE=1,
      KB_500=2, KB_300=3, KB_250=4, KB_1MB=5,
      DISK_CHANGE=1<<7
   };

   enum STATUSA {
      HEAD_TO_INNER=1, NOT_WRITE_PROTECTED=2, INDEXMARKING_OBTAINED=4,
      HEAD0=0<<3, HEAD1=1<<3, HEAD_ON_TRACK0=1<<4, STEP_IMPULS=1<<5,
      TWO_DRIVES=1<<6, IRQPENDING=1<<7
   };

   enum STATUSB {
      MOTOR_0=1<<1, MOTOR_1=1<<2, WRITE_ENABLE=1<<2, READING=1<<3, WRITING=1<<4, DRIVE0=1<<5
   };
};

}}}   // namespace

#include "FloppyController.hpp"

#endif
