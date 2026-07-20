/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
#include "Init/MyOSCoreContainerDefs.h"
#ifndef BIOS_IN_CORE
#error This test needs the BIOS driver
#endif 
  
#include "Context/IContext.h"
#include "debug.h"
#include "Tests/TestComponent.h"

#include "BIOS/IBIOS.h"

#include "MultiThreading/IThread.h"

namespace MyOS { namespace Tests {

using namespace BIOS;

using namespace MultiThreading;

/**
 * A test for the BIOS VM86 calling mechanism
 * 
 */
bool doTest( IContext& context, NVPAIR params[] )
{
   DPRINTK( "\nBIOS test running" );
   
   IInterface *b;
   if (context.lookup( myos_name_t("2ebf3820-423e-47cc-b13c-07eff2a2a6a5"), b )==E_MYOS_SUCCESS) {
      IBIOS* bios = b->castTo<IBIOS>();

      // MOV   AH, 0x00    ; Set video mode
      // MOV   AL, 0x12    ; 80x30  8x16  640x480   16/256K  1   A000  VGA,  + 0x80 for no clear screen
      // INT   0x10
   
      VM86REGS regs;
      // regs.ax = 0x12 | 0x80;  // don't clear screen
      regs.ax = 0x0F00;          // get video mode
      if (bios->vm86int( 0x10, regs ) == E_MYOS_SUCCESS) {
         PRINTK( "\nResult (AX) = %x", regs.ax );
         return true;
      }
   }
   return false;
}

}}
