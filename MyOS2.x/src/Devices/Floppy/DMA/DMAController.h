/***************************************************************************
                          DMAController.h  -  description
                             -------------------
    begin                : Fri Feb 15 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef DMACONTROLLER_H
#define DMACONTROLLER_H

#include "8237A.h"

namespace MyOS {
namespace Devices { 
namespace DMA {

/// Arguments to startDMATransfer(), these match the 8237A's bits as closely as possible
enum DMABITS {
 DMA_TRANSFERMODE_VERIFY = 0,
 DMA_TRANSFERMODE_WRITE  = 1<<2,     // write means from device to memory
 DMA_TRANSFERMODE_READ   = 2<<2,
 TRANSFERMODEMASK        = (DMA_TRANSFERMODE_READ|DMA_TRANSFERMODE_WRITE),

 DMA_AUTO_INITIALIZE  = 1<<4,
 DMA_AUTO_INCREMENT   = 0,
 DMA_AUTO_DECREMENT   = 1<<5,
 DMA_TYPE_DEMAND      = 0,
 DMA_TYPE_SINGLE      = 1<<6,
 DMA_TYPE_BLOCK       = 2<<6,
 TYPEMASK             = (DMA_TYPE_SINGLE|DMA_TYPE_BLOCK),

 // These have to be shifted
 DMA_CHANNEL_0=0<<8,
 DMA_CHANNEL_1=1<<8,
 DMA_CHANNEL_2=2<<8,
 DMA_CHANNEL_3=3<<8,
 // DMACHANNEL_4=4, cascade, do not use
 DMA_CHANNEL_5=5<<8,
 DMA_CHANNEL_6=6<<8,
 DMA_CHANNEL_7=7<<8,

 CHANNELMASK = 7<<8,
};

/// Controls access to the DMA chips
class DMAController
{
public:
   inline void init() { DMAChip.init(); }
	bool startDMATransfer( u32 bits, size_t bytecount, physadr_t toBuf );

private:
	C8237A DMAChip;
};

}}}	// namespace

#endif
