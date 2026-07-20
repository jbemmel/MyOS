#include "LPTPort.h"
#include "debug.h"
namespace MyOS { namespace Devices { namespace LPT {

LPTPort::LPTPort()
{
   control = (1<<3) | (1<<2);    // DSL, init  
}
 
bool   
LPTPort::write( char ch )
{
   for (u32 c=0; c<TIMEOUT_CNT; ++c) {
      if ((status & 0x80) == 0x80) {  // busy bit 1 == not busy
         data = ch;
         control = control | 0x01;  // strobe
         for (u32 s=0; s<STROBE_WAIT;++s);   // wait strobe
         control = control & ~0x01;  // strobe down
      
         for (u32 s=0; s<TIMEOUT_CNT; ++s) {
            if ((status & 0x40) == 0x00) return true; // ACK
         }
         DPRINTK( "\nno ACK" );
         return false;  // NO ACK
      }       
   }
   DPRINTK( "\nLPT:TIMEOUT" );
   return false;  // E_TIMEOUT
}   
   
}}}   // namespace
