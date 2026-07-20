/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
#include "Init/MyOSCoreContainerDefs.h"

//#ifndef BIOS_IN_CORE
//#error This test needs the BIOS driver
//#endif 
  
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
   
   IBIOS &bios = context.lookup( myos_name_t(IBIOS::ID()) ).castToExcept<IBIOS>();

  // MOV   AH, 0x00    ; Set video mode
  // MOV   AL, 0x12    ; 80x30  8x16  640x480   16/256K  1   A000  VGA,  + 0x80 for no clear screen
  // INT   0x10
   
   VM86REGS regs;
   // regs.ax = 0x12 | 0x80;  // don't clear screen
   regs.ax = 0x0F00;          // get video mode
   if (bios.vm86int( 0x10, regs ) == E_MYOS_SUCCESS) {
     PRINTK( "\nResult (AX) = %x", regs.ax );
     
     // 2nd test: get video modes info (VESA VBE)
     struct vbeControllerInfo {
        char signature[4];             // == "VESA"
        short version;                 // == 0x0300 for VBE 3.0
        short oemString[2];            // isa vbeFarPtr
        unsigned char capabilities[4];
        short videomodes[2];           // isa vbeFarPtr
        short totalMemory;             // as # of 64KB blocks
        
        char reserved[ 512 - 20 ];      // must be 512 bytes in size
     } PACKED info;
     memset( &info, 0, sizeof(info) );
     
     // get extended info
     memcpy( info.signature, "VBE2", 4 );     
     
     regs.clear();
     regs.ax = 0x4F00;
     MM::linadr_t vmbuf = bios.vm86map( &info, sizeof(info) );
     regs.es = ((u32) vmbuf) >> 4;
     regs.di = ((u32) vmbuf) & 0xf;
     bios.vm86int( 0x10, regs );
     PRINTK( "\nResult (AX) = %x VBE.version=%x mem=%d signature=%c%c%c%c", 
             regs.ax, info.version, info.totalMemory, 
             info.signature[0],info.signature[1],info.signature[2],info.signature[3] );
     bios.vm86unmap( vmbuf, sizeof(info) );
     
     return true;
  }
  return false;
}

}}
