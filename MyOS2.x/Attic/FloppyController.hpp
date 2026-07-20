/***************************************************************************
                          FloppyController.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "FloppyController.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace Floppy {


template <FDC base>
void
FloppyController<base>::initialize() {
	reset();
	digitalOutputRegister = ACTIVATE_CONTROLLER | ACTIVATE_DMAIRQ;

	// TO DO: check !
	// rateSelectRegister = 0;	// 500Kb/s, default precompensation delay
	// specifyRate( 0 );

	// set drive parameters to defaults
	enum {
	   // E_STEPTIME  = 0xA,   // 6ms at 500Kbps, as suggested by "STEPRATE.doc"
	   E_STEPTIME  = 0xD,   // 3ms at 500Kbps, as suggested by fdc.c
	   E_UPTIME    = 0xF,   // 240ms at 500Kbps, as suggested by linux constant
	   E_DOWNTIME  = 0x8,   // 16ms at 500Kbps, ""
	};
	u8 setParametersCommand[4] = {
	   ControllerCommand::SET_DRIVE_DATA,
	   (E_STEPTIME<<4) | E_UPTIME,
	   (E_DOWNTIME<<1),
	   0  // filler
	};
	issueCommand( setParametersCommand, 3 );
}


template <FDC base>
u8
FloppyController<base>::waitUntilReady() const
{
	u32 timeout = MAXTIMEOUT;
	u8 status;
	do {
		status = mainStatusRegister;
	} while ( (!(status&DR_READY)) && --timeout );
	return (timeout!=0) ? status : 0xFF;
}

/* sendbyte() routine from intel manual? NO, linux variant */
template <FDC base>
bool
FloppyController<base>::sendByte(u8 byte)
{
   u8 status = waitUntilReady();
   if ((status&(DR_READY|IO_TO_CPU|DMA_DEACTIVATED))==DR_READY) {
      dataRegister = byte;
      return true;
   }
   DPRINTK( "###sendByte timeout### status=%X", status );
   return false;
}

template <FDC base>
bool
FloppyController<base>::senseIrq()
{
   if (sendByte(ControllerCommand::CHECK_IRQSTATUS)) {
      waitUntilReady();                // result ignored...   
      u8 st0 = dataRegister;           // read code
      waitUntilReady();                // result ignored...
      u8 d UNUSED = dataRegister;      // read cylinder, currently ignored
      
      // also current head is available...
      return (st0&ControllerStatus::SEEK_COMPLETE)==ControllerStatus::SEEK_COMPLETE;
   }
   DPRINTK( "\nsenseIrq sendByte failed!" );
   return false;
}

template <FDC base>
bool
FloppyController<base>::configure()
{
   if (sendByte(ControllerCommand::FIFO_CONFIG)) {
      if ((waitUntilReady()&(DMA_DEACTIVATED|IO_TO_CPU|DR_READY))==DR_READY) {
         sendByte( 0x0 );
         sendByte( 0x10 | 0xa );    // use FIFO, length=10
         sendByte( 0x0 );           // precompensate for track >= 0
         return true;
      } else DPRINTK( "\nFDC:configure wait failed" );
   } else DPRINTK( "\nFDC:configure send failed" );
   return false;
}

template <FDC base>
u8
FloppyController<base>::partid()
{
   if (sendByte(ControllerCommand::PARTID)) {
      u8 status = waitUntilReady() & (BUSY|DMA_DEACTIVATED|IO_TO_CPU|DR_READY);

      // expect 1 byte
      if ( status == (BUSY|IO_TO_CPU|DR_READY) ) {
         return dataRegister;
      } else DPRINTK( "\nFDC:partid failed status=%X", status );
   }   
   return 0xff;      
}

template <FDC base>
bool
FloppyController<base>::issueCommand( const u8 *command, size_t csize )
{
   // DPRINTK( "\nFloppy:issueCommand len=%u", csize );
	for (; csize; --csize) {
	   if (!sendByte( *command++ )) {
			return false;	// linux just ignores this...
		}
	}
	return true;
}

template <FDC base>
u8
FloppyController<base>::readStatus( ControllerStatus &result )
{
	u8 *res = (u8*) &result;

	for (u8 c=0; c<16; ++c)		// 16 == MAX_REPLIES
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

// helper function
inline u32 limit (u32 x, u32 min, u32 max) {
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
}

/* Issue a "SPECIFY" command to set the step rate time, head unload time,
 * head load time, and DMA disable flag to values needed by floppy.
 *
 * The value "dtr" is the data transfer rate in Kbps.  It is needed
 * to account for the data rate-based scaling done by the 82072 and 82077
 * FDC types.  This parameter is ignored for other types of FDCs (i.e.
 * 8272a).
 *
 * Note that changing the data transfer rate has a (probably deleterious)
 * effect on the parameters subject to scaling for 82072/82077 FDCs, so
 * fdc_specify is called again after each data transfer rate
 * change.
 *
 * srt: 1000 to 16000 in microseconds
 * hut: 16 to 240 milliseconds
 * hlt: 2 to 254 milliseconds
 *
 * These values are rounded up to the next highest available delay time.
 */
template <FDC base>
void
FloppyController<base>::specifyRate( u8 rate )
{
	unsigned long scale_dtr = NOMINAL_DTR;

/*	TBD: configure FIFO
	if (FDCS->need_configure && FDCS->version >= FDC_82072A) {
		fdc_configure();
		FDCS->need_configure = 0;
	}
*/

	if (version >= FDC_82072) {
		switch (rate & 0x03)
		{
			case 1: scale_dtr = 250; break;
			case 3: scale_dtr = 1000; break;
			case 2:
				scale_dtr = 300;
				if (version >= FDC_82078) {		// select default rate table, not with 2Mbps
/*
					output_byte(FD_DRIVESPEC);
					if (need_more_output() == MORE_OUTPUT) {
						output_byte(UNIT(current_drive));
						output_byte(0xc0);
					}
*/
				}
				break;
		}
	}

	// Convert step rate from microseconds to milliseconds and 4 bits
	u32 srt = limit( 16 - (SRT*scale_dtr/1000 + NOMINAL_DTR - 1)/NOMINAL_DTR, 0, 0xf );

	u32 hlt = limit( (HLT*scale_dtr/2 + NOMINAL_DTR - 1)/NOMINAL_DTR, 0x01, 0x7f );
	if ((version >= FDC_82072) && hlt==0x7f) hlt=0;

	u32 hut = limit( (HUT*scale_dtr/16 + NOMINAL_DTR - 1)/NOMINAL_DTR, 0x01, 0xf );
	if ((version >= FDC_82072) && hut==0xf) hut=0;

	// Specify command (3 bytes), uses DMA (byte 2, bit 0 == 0)
	const u8 specifycommand[4] = { ControllerCommand::SET_DRIVE_DATA, (srt << 4) | hut, (hlt << 1), 0 };

	// check if this is different from last time
	if ( *((u32*) specifycommand) != *((u32*) lastspecifycommand) ) {
		*((u32*) lastspecifycommand) = *((u32*) specifycommand);
		issueCommand( lastspecifycommand, 3 );
	}

}

}}}	// namespace
