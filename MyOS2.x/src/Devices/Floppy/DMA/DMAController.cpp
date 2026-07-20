/***************************************************************************
                          DMAController.cpp  -  description
                             -------------------
    begin                : Fri Feb 15 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "DMAController.h"

namespace MyOS {
namespace Devices {  
namespace DMA {

// Mapping of channel->page register IO
const u8 C8237A::pageIOport[4] = {
	0x87,0x83,0x81,0x82  // DMA_2: +8 for each value
};

bool	// Note: This code might be better inlined when called directly with constants...
DMAController::startDMATransfer( u32 bits, size_t bytecount, physadr_t buf )
{
	// Program the DMA controller, typically like this: (PC HW p. 582)
	// DMAChip.deactivate( DMA1_SLAVE );

	// channel implies chip: 0..3 = DMA1, 4..7 = DMA2 (4 = cascade)
	MODE channel = (MODE) ((bits&CHANNELMASK)>>8);
	if (bits>=DMA_CHANNEL_5) {	// use DMA2 ? 16 bits DMA
		(u8&) channel &= ~4;
		bytecount >>= 1;	// is programmed in 16-bit amounts
		DMAChip.maskChannel( DMA2_MASTER, channel );
		DMAChip.setMode( DMA2_MASTER, (MODE) (bits|channel) );
		DMAChip.setAddress( DMA2_MASTER, channel, buf );
		DMAChip.setCount( DMA2_MASTER, channel, bytecount );
		DMAChip.unmaskChannel( DMA2_MASTER, channel );
	} else {
		// DMAChip.deactivate( DMA1_SLAVE );		Alternative: just mask channel
		DMAChip.maskChannel( DMA1_SLAVE, channel );
		DMAChip.setMode( DMA1_SLAVE, (MODE) (bits|channel) );
		DMAChip.setAddress( DMA1_SLAVE, channel, buf );
		DMAChip.setCount( DMA1_SLAVE, channel, bytecount );
		DMAChip.unmaskChannel( DMA1_SLAVE, channel );

		// perhaps not every time ?
		// DMAChip.activate( DMA1_SLAVE );
	}
	return true;
}

}}}	// namespace
