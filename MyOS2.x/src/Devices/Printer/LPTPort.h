#ifndef LPTPORT_H
#define LPTPORT_H

#include "Devices/IOPort.h"

namespace MyOS { namespace Devices { namespace LPT {

// Simple class, useful for debugging   
class LPTPort {

enum { 
   BASE_ADR = 0x378,    /* LPT1, should come from BIOS */ 

// Note: These values work under Bochs, but are not valid in general!
   TIMEOUT_CNT = 2,     /* Max. retries before aborting write */
   STROBE_WAIT = 1,     /* wait for strobe, value ok? */
};

// XX these need not have memory allocated to them...
IOPort8<BASE_ADR+0x0>   data;
IOPort8_R<BASE_ADR+0x1> status;
IOPort8<BASE_ADR+0x2>   control;
u8 __padding;

public:
   LPTPort();  // XX called every time when LPTStream is stack-allocated...
   bool write( char c ); 
};   
   
}}}   // namespace

#endif
