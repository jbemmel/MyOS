//#ifdef DEBUG

#include "Assertion.h"
#include "debug.h"      // DPRINTK
#include "Processor/Processor.h"

#include "Devices/Printer/LPTStream.h"
#include "Devices/Display/ScreenBuffer.h"    // SCREENWIDTH

namespace MyOS { namespace Debugging {

using namespace Devices::Display;

void
assertionFailed( E_SEVERITY s, const char* fileline, const char* func, int debugvalue )
{
   // index register to reset screen to top (could perhaps just call Display::clear)
   IOPort16<0x3d4> initialRowAddress;
   initialRowAddress = (0 & 0xFF00) | 0xc;
   initialRowAddress = ((0 & 0x00FF) << 8) | 0xd;

   DPRINTK( "#4f#\nASSERTION FAILED function %s : %s (severity %u)", func, fileline, s );
   if (debugvalue) DPRINTK( "#4f# (debug value=%x)", debugvalue );

   // Duplicate on the printer
   // Devices::LPT::LPTStream printer;
   // printer.printf( "\nASSERTION FAILED function %s : %s (severity %u)(debug %x)", func, fileline, s, debugvalue );

   if (s<=E_CRITICAL) {
      Processor::allowInterrupts( false );
      while( 1 );
   }
}

void checkpoint( char c )
{
   static s32 curColom=0;

   if (++curColom >= ScreenBuffer::E_SCREENWIDTH-1) {
      // leave timer heartbeat on screen
      // statusbar << GotoRC( 24,0 ) << Repeat( '-', 79 ) << GotoRC( 24,0 );
      ScreenBuffer::getInstance().fillrow( 
         ScreenBuffer::E_SCREENHEIGHT-1, 0x072D072D );  // '-''-'
      curColom = 0;
   }

   ScreenBuffer::getInstance().put( ScreenBuffer::E_SCREENHEIGHT-1, curColom, c, E_WHITE );
   ScreenBuffer::getInstance().put( ScreenBuffer::E_SCREENHEIGHT-1, ++curColom, '.', E_WHITE );
}

  
}} // namespace

//#endif   // DEBUG
