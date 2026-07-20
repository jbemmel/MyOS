#include "IBIOSImpl.h"
#include "BIOS/BIOSComponent.h"

#include "MM/new.h"
#include "MM/memtypes.h"
#include "InterruptHandling/ihtypes.h"

#include "debug.h"

namespace MyOS { namespace BIOS {

// asm routines
extern "C" void VM86Code();
extern "C" void afterCall();
extern "C" void INT3Handler();
extern "C" void VM86TrapHandler();

IBIOSImpl::IBIOSImpl( MyOS::Core::IComponent& c )
: IBIOS( c, VERSION(1,0) )
{

}

bool 
IBIOSImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   DPRINTK( "\nBIOS init" );
 
   IPhysicalMemory* mem = BIOSComponent::getInstance().iPhysicalMemory; 
 
   // make sure BIOS page 0 is mapped at physical 0, writable since the BIOS modifies parameters
   mem->setAttributes( 0, 1, E_PAGE_PRESENT | E_PAGE_READWRITE | E_PAGE_USER );

   // (E)IP must also be < _1MB, and it should be 16bit code
   u32 realCode = ((u32)&VM86Code) & (_1MB-1);
   mem->copyMapping( (linadr_t) realCode, (linadr_t) &VM86Code, 1, E_PAGE_PRESENT | E_PAGE_READONLY | E_PAGE_USER );  // too much...

   // (since it's only 3 bytes of code, I might as well replace e.g. INT3 handler entry in BIOS[0]!)

   IInterruptHandling* ih = BIOSComponent::getInstance().iInterruptHandling;

   // as a quick hack, install a global trap handler for int 0x10
   ih->setTrapHandler( (E_INTCODES) 0x10, VM86TrapHandler, E_ENABLE_INTS | E_DPL3 );

   // for some reason, VGA BIOS uses 0x6d as 0x10
   ih->setTrapHandler( (E_INTCODES) 0x6D, VM86TrapHandler, E_ENABLE_INTS | E_DPL3 );

   // set return handler too
   ih->setTrapHandler( (E_INTCODES) 0x03, INT3Handler, E_ENABLE_INTS | E_DPL3 );
 
	return true;
}

void 
IBIOSImpl::deinit( IContext& context )
{
   IInterruptHandling* ih = BIOSComponent::getInstance().iInterruptHandling;
   // cleanup, could remember old handlers...
   ih->removeTrapHandler( (E_INTCODES) 0x03 );
   ih->removeTrapHandler( (E_INTCODES) 0x6D );
   ih->removeTrapHandler( (E_INTCODES) 0x10 );

   IPhysicalMemory* mem = BIOSComponent::getInstance().iPhysicalMemory;
   // u32 realCode = ((u32)&VM86Code) & (_1MB-1);
   // VM86Component::getInstance().memory()->unmapRegion( (linadr_t) realCode, 1 );
   mem->setAttributes( 0, _4KB, 0 );
}


// virtual 
myos_result_t
IBIOSImpl::vm86int( u8 i, VM86REGS& regs )  
{
   DPRINTK( "\nvm86int[%X] regs.ax=%x", i, regs.ax );
  
   u32* v86stack = new (THREAD) u32[ 256 ];    // allocate v86 stack
   DPRINTK( "\nAllocated V86 stack at %x", v86stack );
   
   // alias stack to under 1Mb, need only be done once per thread...
   u32 realStackSeg = ((u32) v86stack & 0xFF000);    // strip page offset too

   IPhysicalMemory* mem = BIOSComponent::getInstance().iPhysicalMemory;

   // at the moment, the first 4Mb are identity mapped for all processes (shared)
   mem->copyMapping( 
      (linadr_t) realStackSeg, (linadr_t) v86stack, 1,
         E_PAGE_PRESENT | E_PAGE_READWRITE | E_PAGE_USER );
      
   ASMVOLATILE( 
      "call VM86MakeCall" 
      :  
      : "a"(i), "d"(&regs), "c"(v86stack) 
      : "memory"  // use ASM to indicate mem clobber  
   );

   regs.ax = (u16) *((u32*) v86stack[ 255 ]);
   
   // TODO: unmap region below 1Mb. not really 'releaseRegion' since it was
   // never allocated explicitly
   
   ::operator delete( v86stack, 256 * sizeof(u32), THREAD );
   return E_MYOS_SUCCESS;
}

void  // static
IBIOSImpl::makeCall( u8 intNumber, VM86REGS& regs, u32* v86stack /* in/out */ )
{
   // save address of regs structure on stack (force it somehow!)
   VM86REGS* volatile result = &regs;

   // calculate proper regs.SP ??
   // regs->sp = (Processor::ESP() + 0x0) & (_1MB-1);

   // instead of pushing onto the PM stack, I define a separate VM86 stack page
   // VBE BIOS can push 512 bytes, and this way VM86 task cannot corrupt PM code/stack
   u32* vm86sp = &v86stack[ 256 ];

   // current ESP, to be restored in INT3Handler
   *--vm86sp = (u32) ( Processor::ESP() - 0x4 );

   // CS of after call, followed by IP of aftercall, and EFLAGS, for IRET in VM86 code
   *--vm86sp = ( Processor::FLAG_IOPL3 | Processor::FLAG_INT );           // FLAGS with upper 16bit=0
   *--vm86sp = ((((u32) &afterCall) << 12) & 0xFFFF0000 | ((u32) &afterCall) & 0xF );   // CS:IP

   // CS:IP of INT handler #intNumber, for RETF in VM86 code
   *--vm86sp = ( ((u32*) 0)[ intNumber ] );

   // input registers for POPA in VM86 code
   *--vm86sp = ( *((u32*) &regs.cx) );   // CX, AX
   *--vm86sp = ( *((u32*) &regs.bx) );   // BX, DX
   *--vm86sp = ( *((u32*) &regs.bp) );   // BP, SP
   *--vm86sp = ( *((u32*) &regs.di) );   // DI, SI

#define PUSH(x) ASMVOLATILE( "pushl %0" :  : "r"(x) : "%esp" );

   // return address for 'ret' in INT3Handler (may not work in release version!)
   PUSH( &&afterIret );

   // stackframe for iret below
   PUSH( 0 );  // GS
   PUSH( 0 );  // FS
   PUSH( (u32) regs.ds );  // DS
   PUSH( (u32) regs.es );  // ES

   register u32 esp = ((u32)vm86sp + 0x0) & (_1MB-1);
   PUSH( (esp & 0xF0000) >> 4 );    // SS
   PUSH( esp & 0xFFFF );            // ESP, before all this pushing (but not too small...)
   PUSH( Processor::FLAG_V8086 | Processor::FLAG_IOPL3 | Processor::FLAG_INT );   // EFLAGS
   PUSH( ( (u32) (&VM86Code) & (_1MB-1)) >> 4 );   // CS
   PUSH( (u32) (&VM86Code) & 0xF );                // EIP

#undef PUSH

   // go do it!
   ASMVOLATILE( "iret" : : "D"(0),"S"(0) : "memory" );   // clear some 32bit registers
afterIret:
   ASMVOLATILE(
      "movw    %%di, 0x0(%%eax) \n" // save results
      "movw    %%si, 0x2(%%eax) \n" // save results
      "movw    %%bp, 0x4(%%eax) \n" // save results
      // not sp
      "movw    %%bx, 0x8(%%eax) \n" // save results
      "movw    %%cx, 0xA(%%eax) \n" // save results
      "movw    %%dx, 0xC(%%eax) \n" // save results
   :
   : "a"(result)
   : /* "ebx", "ecx", "edx", "esi", "edi", "ebp", */ "memory"
   );

   // lastly, restore original ax (done in caller now, params not saved!)
   // result->ax = (u16) *((u32*) v86stack[ 255 ]);
}

// PM trap handler for BIOS (int 0x10) calls, and perhaps others
// Only SS and CS are loaded properly on entry(!)

/**
   Upon entry the stack looks like this: 
   (see http://x86.ddj.com/articles/vme1/vme_overview.htm)

   v86EIP         dd ?        <- ESP
   v86CS          dd ?        +4
   v86EFLAGS      dd ?        +8
   v86ESP         dd ?        +12
   v86SS          dd ?        +16
   v86ES          dd ?
   v86DS          dd ?
   v86FS          dd ?
   v86GS          dd ?
   v86_Context ENDS
*/
__asm__ (
"VM86TrapHandler:                \n"
"_VM86TrapHandler:               \n"
"TESTB   $0x02, 10(%esp)         \n"   // test EFLAGS image for VM86 flag
"JZ      no_vm86                 \n"   // no, PM mode, something wrong here...

"PUSH %eax \n PUSH %ecx \n PUSH %edx  \n"   // save changed registers
"MOVL    16(%esp), %eax          \n"   // load CS (PUSHes adds 3*4 to esp)
"SHL     $4, %eax                \n"   // times 16
"ADDL    12(%esp), %eax          \n"   // add EIP
"MOVL    %ss:-2(%eax), %eax      \n"   // load executed opcode(s) (32bit) using SS as selector(!)
"CMPB    $0xCD, %al              \n"   // only interested in 'cd', not into or int3
"JNE     not_int_opcode          \n"

"MOVZX   %ah, %eax               \n"   // keep only #INT
"MOVL    %ss:(,%eax,4), %eax     \n"   // get CS:IP of real mode handler

// Save original CS:EIP, before it gets changed
"MOVL    12(%esp), %edx          \n"   // get EIP
"MOVL    16(%esp), %ecx          \n"   // get CS

// Adjust return point on PL0 stack
"MOV     %ax, 12(%esp)           \n"   // update IP (16 bits)
"SHR     $16, %eax               \n"   // get CS in lower 16 bits
"MOV     %eax, 16(%esp)          \n"   // CS

// Modify PL3 state, such that real mode iret will end up correctly
"SUBL    $6, 24(%esp)            \n"   // make some room on the PL3 stack
"MOVL    28(%esp), %eax          \n"   // load SS (PUSHAL adds 7*4 to esp)
"SHL     $4, %eax                \n"   // times 16
"ADDL    24(%esp), %eax          \n"   // add ESP
// "ADDL    $2, %edx                \n"   // compensate for 'int n' instruction?
"MOVW    %dx, %ss:(%eax)         \n"   // put IP on real mode stack
"MOVW    %cx, %ss:2(%eax)        \n"   // put CS on real mode stack
"MOVL    20(%esp), %edx          \n"   // get EFLAGS
"MOVW    %dx, %ss:4(%eax)        \n"   // put EFLAGS on real mode stack

"not_int_opcode:                 \n"
"POP %edx \n POP %ecx \n POP %eax  \n"   // restore changed registers

"no_vm86:"
"IRET    \n"
);

// VM86 code, 16 bit (!)
__asm__ (
".section VM86       \n"   // Must be handled in linker script!
"VM86Code:           \n"
"_VM86Code:          \n"
"POPA                \n"   // load register values
"LRET                \n"   // make INT call based on CS:IP on stack
"afterCall:          \n"   // return address to setup
"_afterCall:         \n"   // return address to setup
"INT $3              \n"   // back to PMode
".section .text      \n"
);

/**
 * ESP is restored to ESP0, which is _not_ set at 'iret' in makeVM86Call (but in TSS.cpp)
 */
__asm__ (
"INT3Handler:                 \n"
"_INT3Handler:                \n"
"PUSH    %ss                  \n"   // restore DS, ES
"POP     %ds                  \n"
"PUSH    %ss                  \n"
"POP     %es                  \n"
"PUSH    %eax                 \n"   // save value
"MOVL    20(%esp), %eax       \n"   // load SS (PUSH adds 4 to esp)
"SHL     $4, %eax             \n"   // times 16
"ADDL    16(%esp), %eax       \n"   // add ESP
"XCHGL   2(%eax), %esp        \n"   // read original ESP (PM mode), keep ESP0 at end of VM86 stack
"RET                          \n"   // return to "afterIret"
);

}}  // namespaces
