/**
 * These initialization routines clean up a bit of the mess the bootloader left,
 * in terms of memory layout
 *
 * - Free all physical pages below 1Mb, as these may need to be used for DMA
 * - Unzip the compressed kernel images to pages above 1Mb
 *
 * Some things to note about the current implementation:
 * +) Uncompress code is discarded (not remapped), so Zip classes won't work
 */

/// Constants that determine the outcome
#include "memlayout.h"
#include "defs.h"
#include "types.h"
#include "string.h"

#include "memtypes.h"            // paging bits
#include "MT/Thread.h"  // sizeof(Thread)

#include "Zip/ZipHeader.h"
#include "Zip/ZipContext.h"

using namespace MyOS::Zip;
using namespace MyOS::MM;

extern "C" void MyOSmain() NORETURN;

namespace MyOS { namespace Init {

/**
 * The initial stack (becomes the idle thread for CPU0)
 *
 * Size = 2 pages: 1 guard page (unmapped later), like other stacks (does not work yet)
 */
static u32 initialStack[/*2**/1024] __attribute__ ((section(".stack")));

// prototypes
static inline void printMsg( const char* msg, int errorcode ) UNCOMPRESSEDSECTION;

static s32 decompress( u8* bs, u8* fs ) UNCOMPRESSEDSECTION;

static s32 decompress()
{
   ZipContext::initialize();

   ZipHeader header;
   buffer zipped( (u8*) &_START_OF_COMPRESSED_BINARY_, (size_t) &_SIZE_OF_COMPRESSED_BINARY_ );
   E_RESULT r = header.parse(zipped);
   if ( r == E_OK ) {

      // max. kernel size then 512Kb (unzipped!)
      buffer result( (u8*) KERNEL_PHYSICAL, _512KB ); // linear == physical
      buffer win32k( (u8*) &_FREE_KERNEL_SPACE_START_, _32KB ); // scratch buffer

      ZipContext ctxt( header.getData(), win32k, win32k.getEnd(), win32k.getEnd()+_256KB );
      r = ctxt.inflate(result);
   }
   return r;
}

extern "C" void* stackcode;

#ifdef MYOS_ELF
#define STACKCODE_ASM_LABEL "stackcode:"
#else
#define STACKCODE_ASM_LABEL "_stackcode:"
#endif

static inline void printMsg( const char* msg, int errorcode )
{
    u32* screen = (u32*) 0xB8000;
    u32* src = (u32*) msg;

    while ( *src & 0xff )
    {
        *screen++ = *src++;
    }
   *screen = 0x4F304F30 | ((errorcode/10)<<0) | ((errorcode%10)<<16);
}

// linker must put this at the (uncompressed) beginning of file
extern "C" void doDecompress() STARTSECTION;

// calculate PTE byte index for a given linear address
#define PT(l) ((((u32)l)&0x3FF000)>>10)

/**
 * This routine gets control after the bootloader has finished loading the
 * kernel image from floppy or USB into memory
 *
 * Machine state is as follows:
 *  ESP			   	: 0x800 (i.e. in physical page 0)
 *  CS 			   	: OFFSET 0, LIMIT 0xFFBFF
 *  DS,ES,FS,GS		: OFFSET 0, LIMIT 0xFFFFF
 *  SS			   	: undefined (!)
 *
 *  CR3 (PBT)			: 0x9E000 (see bootphase2.asm)
 *  PBT[0]		        : 0x9C000, this maps physical pages in 0-4Mb
 *  PBT[1022]           : 0x9D000
 *  PBT[1023]           : 0x9E000, this is the PBT itself
 *  PBT[0][0..1023] 	: Physical memory 0-4Mb identity mapped
 *  PBT[1022][0..1023]	: Kernel 0xFF800000 mapped to 1Mb physical
 *
 *  IDT is in physical page 0 (first 256*8 bytes)
 *  GDT is in the bootsector  (bootsec@0x7c00, GDT@0x7de4), for GRUB boots it is at 0x0000be14
 */
void doDecompress()
{
	ASMVOLATILE(
		"push %ds \t\n"
		"pop %ss  \t\n"
	);	// initialize ss to ds

    // Set stack to initial stack allocated above
    // Reserve some space to make it look like a stack allocated by Thread class
    u32* esp = &initialStack[ (sizeof(initialStack)/sizeof(u32))
                              - ((sizeof(MultiThreading::Thread)+3)/sizeof(u32))];

    // align by 16, GCC expects proper stack alignment
    // esp = (u32*) (((u32) esp) & ~0xf);
    esp -= 3;
    ASMVOLATILE( "mov %0, %%esp" : : "r"( esp ) );

    s32 length = decompress();
    if (length>0) {

      // Clear thread structure, just in case
      memset_aligned( esp, 0, sizeof(MultiThreading::Thread) );

      // prepare new minimal page tables
      memset_aligned( (void*) PBT_PHYSICAL,     0, _4KB );  // clear PBT
      memset_aligned( (void*) PDT0_PHYSICAL,    0, _4KB );  // clear PDT0
      memset_aligned( (void*) PDT1022_PHYSICAL, 0, _4KB );  // clear PDT1022

      // XXX This very much depends on the BIOS, bochs shows this memory dirty
      // after reboot (but it is zero when bochs is first started)
      // memset_aligned( (void*) 0x312000, 0, 4096 );       // clear all mem

      // Put the PDTs into the PBT, with paging bits
      ((u32*)PBT_PHYSICAL)[0]    = PDT0_PHYSICAL    | E_PAGE_KERNEL;
      ((u32*)PBT_PHYSICAL)[1022] = PDT1022_PHYSICAL | E_PAGE_KERNEL;
      ((u32*)PBT_PHYSICAL)[1023] = PBT_PHYSICAL     | E_PAGE_KERNEL;

      u32 codePages = (u32) &_CODE_PAGES_;
      u32 dataPages = (u32) &_DATA_PAGES_;
      u32 bssPages  = (u32) &_BSS_PAGES_;

      // Any module is appended after the code & data binaries, integer number of pages
      u32 modulePages = (((length - (u32) &_SIZE_OF_COMPRESSED_BINARY_)+(0x1000-1))) >> 12;

      // Copy PTEs for .text kernel code
      memcpy_aligned( &((u8*) PDT1022_PHYSICAL)[ PT(&_COMPRESSED_START_LINEAR_) ],
         &((u8*) 0xFFC00000)[ PT(KERNEL_PHYSICAL) ],
         4 /* bytes/PTE */ * codePages );

      // Copy PTEs for .data section to 0xFFA00000
      memcpy_aligned( &((u8*) PDT1022_PHYSICAL)[ PT(&_START_OF_DATA_) ],
         &((u8*) 0xFFC00000)[ PT(KERNEL_PHYSICAL) + 4 * codePages ],
         4 /* bytes/PTE */ * dataPages );

      // Copy PTEs for module to 0xFFB00000 (preliminary)
      memcpy_aligned( &((u8*) PDT1022_PHYSICAL)[ PT(&_START_OF_MODULE_) ],
         &((u8*) 0xFFC00000)[ PT(KERNEL_PHYSICAL) + 4 * (codePages+dataPages) ],
         4 /* bytes/PTE */ * modulePages );

      // Finally, take some *uninitialized* pages after the bootimage for .bss
      memcpy_aligned( &((u8*) PDT1022_PHYSICAL)[ PT(&_BSS_START_) ],
         &((u8*) 0xFFC00000)[ PT(KERNEL_PHYSICAL) + 4 * (codePages+dataPages+modulePages) ],
         4 /* bytes/PTE */ * bssPages );

      // Keep page 0, it contains the IDT ! (could be readonly)
      ((u32*)PDT0_PHYSICAL)[0] = 0 | E_PAGE_KERNEL;

      // Keep the page with the bootsector (0x7c00), it contains the initial GDT & mem map
      ((u32*)PDT0_PHYSICAL)[7] = 0x7000 | E_PAGE_KERNEL;

      // For GRUB, keep the page with the GDT (0x0000be14)
      ((u32*)PDT0_PHYSICAL)[0xB] = 0xB000 | E_PAGE_KERNEL;

      // Map the stack
      ((u32*)PDT1022_PHYSICAL)[ PT(initialStack)>>2 ] = STACK_PHYSICAL | E_PAGE_KERNEL;

      // Map video RAM mapping (4 pages)
      for (int p = 0xB8; p<0xBC; ++p) {
         ((u32*)PDT0_PHYSICAL)[p] = (p<<12) | E_PAGE_MMIO;
      }

      // install new PBT, carefully! The code currently executing is gone!
      memcpy_aligned( (void*) STACK_PHYSICAL, &stackcode, 12 );  // 10
      memcpy_aligned( initialStack, &stackcode, 12 );            // 10

/*
      Cannot do it like this: cannot trust optimizer to keep label!

      ASMVOLATILE( "jmp *%0" : : "r"(initialStack) : "memory" );
stackcode:
      ASMVOLATILE( "movl %0, %%cr3" :  : "a"(PBT_PHYSICAL | E_PAGE_KERNEL) : "memory" );
      ASMVOLATILE( "jmp $0x8,$_COMPRESSED_START_LINEAR_" );   // must be absolute!
*/

      // Pass modulePages in EDX
      ASMVOLATILE(
         "jmp *%0                               ;"
         STACKCODE_ASM_LABEL
         "movl %1, %%cr3                        ;"
         "jmp $0x8,$_COMPRESSED_START_LINEAR_   ;"
         :
         : "r"(initialStack), "a"(PBT_PHYSICAL | E_PAGE_KERNEL), "d"(modulePages)
         : "memory"
      );

   } else {

      length &= ~E_ERROR;    // remove sign bit

      // print error message and hang
      printMsg( "EOrOrOoOrO OdOeOcOoOmOpOrOeOsOsOiOnOgO OkOeOrOnOeOlO!O OCOoOdOeO O=O O-O", length );
      while (1);
   }
}

}}  // namespace
