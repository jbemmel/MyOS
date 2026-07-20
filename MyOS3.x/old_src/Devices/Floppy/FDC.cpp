#include "FDC.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace Floppy {

// Policy values
enum {
   MAXTIMEOUT=10000,    // max. for-loop repeat to wait for status, linux
   NOMINAL_DTR=500,

   // parameters for a 3_1/2  inch HD
   HLT = 16,      // head load time (ms)
   HUT = 16,      // head unload time (ms)
   SRT = 4000     // max. step rate interval (usec)
};


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
};

enum MAINSTATUS {
   MOVING_A=1<<0, MOVING_B=1<<1, MOVING_C=1<<2, MOVING_D=1<<3,
   BUSY=1<<4, DMA_DEACTIVATED=1<<5, IO_TO_CPU=1<<6, DR_READY=1<<7
};

enum E_SECTORSIZE { BPS_256=1, BPS_512=2 };
enum E_TRACKLENGTH { KB_360=9, KB_720=9, MB_1_2=15, MB_1_44=18 };
enum E_GAP { DEFAULTGAP=27, INCH5_25_FORMAT=80, INCH3_5_FORMAT=84 };

u8
FDC::waitUntilReady() const
{
   u32 timeout = MAXTIMEOUT;
   u8 status;
   do {
      status = mainStatusRegister;
   } while ( (!(status&DR_READY)) && --timeout );
   return (timeout!=0) ? status : 0xFF;
}

// sendbyte() routine from intel manual? NO, linux variant
bool
FDC::sendByte(u8 byte)
{
   u8 status = waitUntilReady();
   if ((status&(DR_READY|IO_TO_CPU|DMA_DEACTIVATED))==DR_READY) {
      dataRegister = byte;
      return true;
   }
   DPRINTK( "###sendByte timeout### status=%X", status );
   return false;
}

bool
FDC::detect() // INITSECTION
{
   if (!sendByte(DUMP_REGISTERS)) return false;
   FDCStatus status;
   u8 length = this->readStatus( status );
   if (status.st0 == FDCStatus::ST0_INVALIDCOMMAND) {
      DPRINTK( "\nFDC is a FDC_8272A" );  // does not support dump 
      version = FDC_8272A;
      return true;  // could remember type
   }
   // Try to send 'configure' command
   if (!this->configure()) {
      // does not know configure command
      DPRINTK( "\nFDC is a FDC_82072" );
      version = FDC_82072;
      return true;
   }
   length = this->partId();
   if (length!=0xff) {
      if (length == 0x80) {
         DPRINTK( "\nFDC is a post-1991 FDC_82077" ); 
         version = FDC_82077;
         return true;
      } else switch (length >> 5) {
   case 0x0:
       // Either a 82078-1 or a 82078SL running at 5Volt
      DPRINTK( "\nFDC is an 82078-1");
      version = FDC_82078_1;
      return true;

   case 0x1:
      DPRINTK( "\nFDC is a 44pin 82078");
      version = FDC_82078;
      return true;

   case 0x2:
      DPRINTK( "\nFDC is a S82078B\n");
      version = FDC_S82078B;
      return true;
      
   case 0x3:
      DPRINTK( "\nFDC is a National Semiconductor PC87306\n");
      version = FDC_87306;
      return true;
      }
   }      

   DPRINTK( "\nUnknown floppy or error! Returning false" );
   return false;      
}

bool
FDC::configure()  // INITSECTION
{
   if (sendByte(FIFO_CONFIG)) {
      if ((waitUntilReady()&(DMA_DEACTIVATED|IO_TO_CPU|DR_READY))==DR_READY) {
         sendByte( 0x0 );
         sendByte( 0x10 | 0xa );    // use FIFO, length=10
         sendByte( 0x0 );           // precompensate for track >= 0
         return true;
      } else DPRINTK( "\nFDC:configure wait failed" );
   } else DPRINTK( "\nFDC:configure send failed" );
   return false;
}

u8
FDC::partId()  // INITSECTION
{
   if (sendByte(PARTID)) {
      u8 status = waitUntilReady() & (BUSY|DMA_DEACTIVATED|IO_TO_CPU|DR_READY);

      // expect 1 byte
      if ( status == (BUSY|IO_TO_CPU|DR_READY) ) {
         return dataRegister;
      } else DPRINTK( "\nFDC:partid failed status=%X", status );
   }   
   return 0xff;      
}

void
FDC::initialize() // !NOT! INITSECTION
{
   reset();
   digitalOutputRegister = ACTIVATE_CONTROLLER | ACTIVATE_DMAIRQ;

   // TO DO: check !
   // rateSelectRegister = 0; // 500Kb/s, default precompensation delay
   // specifyRate( 0 );

   // set drive parameters to defaults
   enum {
    // E_STEPTIME  = 0xA,   // 6ms at 500Kbps, as suggested by "STEPRATE.doc"
      E_STEPTIME  = 0xD,   // 3ms at 500Kbps, as suggested by fdc.c
      E_UPTIME    = 0xF,   // 240ms at 500Kbps, as suggested by linux constant
      E_DOWNTIME  = 0x8,   // 16ms at 500Kbps, ""
   };
   
   sendByte( SET_DRIVE_DATA );
   sendByte( (E_STEPTIME<<4) | E_UPTIME );
   sendByte( (E_DOWNTIME<<1) );
}

bool
FDC::seek( u32 lb )
{
   u8 head = (lb/18)&1;   // 0 or 1   
   return 
      sendByte( SEEK_TRACK )  &&    // command   
      sendByte( head << 2 )   &&    // drive select (A:), head, reserved
      sendByte( lb / 36 );
}

bool
FDC::senseIrq()
{
   if (sendByte(CHECK_IRQSTATUS)) {
      waitUntilReady();                // result ignored...   
      u8 st0 = dataRegister;           // read code
      waitUntilReady();                // result ignored...
      u8 d UNUSED = dataRegister;      // read cylinder, currently ignored
      
      // also current head is available...
      return (st0&FDCStatus::SEEK_COMPLETE)==FDCStatus::SEEK_COMPLETE;
   }
   DPRINTK( "\nsenseIrq sendByte failed!" );
   return false;
}

bool 
FDC::readBlock( u32 lb )
{
   u8 head = (lb/18)&1;   // 0 or 1
   return
      sendByte( READ_SECTOR ) &&
      sendByte( head<<2 )     &&    // drive A:, headX
      sendByte( lb/36 )       &&    // track
      sendByte( head )        &&
      sendByte( lb%18 + 1 )   &&    // sector
      sendByte( BPS_512 )     &&    // sector size, fixed
      sendByte( MB_1_44 )     &&    // track length
      sendByte( DEFAULTGAP )  &&    // gap3
      sendByte( 0xff );             // data length, not used for whole sectors
}

u8
FDC::readStatus( FDCStatus &result )
{
   u8 *res = (u8*) &result;

   // 16 == MAX_REPLIES, XX would overwrite stack!
   for (u8 c=0; c<sizeof(FDCStatus); ++c)    
   {
      u8 status = waitUntilReady() & (BUSY|DMA_DEACTIVATED|IO_TO_CPU|DR_READY);
      if ( (status & ~BUSY) == DR_READY ) {
         return c;
      } else if (status == (BUSY|IO_TO_CPU|DR_READY)) {
         *res++ = dataRegister;
      } else return 0;
   }
   return 0;
}

   
}}}   // namespace
