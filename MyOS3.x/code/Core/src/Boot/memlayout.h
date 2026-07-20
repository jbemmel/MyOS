#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

/**
 * This file defines the physical memory layout for some of the kernel
 * structures
 * 
 * @todo Move from Init to MM/ ? Relevant also after init...
 */
#define PBT_PHYSICAL     0x200000   // 2Mb physical, 1 page
#define PDT0_PHYSICAL    0x201000   // 2Mb physical + 1, maps mainly video memory
#define PDT1022_PHYSICAL 0x202000   // 2Mb physical + 2, maps kernel image
#define STACK_PHYSICAL   0x203000   // 2Mb physical + 3, maps boot stack
#define KERNEL_PHYSICAL  0x204000   // Start of area where kernel is uncompressed to

// linker constants, take their address to use them!
extern void* _KERNEL_BASE_;     //< Base address of kernel memory
extern void* _START_OF_COMPRESSED_BINARY_;
extern void* _COMPRESSED_START_LINEAR_;
extern void* _START_OF_CODE_;   //< Used to determine end of INIT section, beginning of user code
extern void* _START_OF_KERNEL_; //< Used to determine start of kernel code
extern void* _CODE_PAGES_;      //< Number of code pages (including init)
extern void* _START_OF_DATA_;
extern void* _DATA_PAGES_;
extern void* _START_OF_MODULE_; //< Used to map binary attached ELF module

extern void* _BSS_START_;
extern void* _BSS_PAGES_;
extern void* _FREE_KERNEL_SPACE_START_;
extern void* _SIZE_OF_COMPRESSED_BINARY_;
extern void* SIGNAL_SAFE_END;

extern void* _BSS_SIZE_;

extern void* _KERNEL_PAGES_;      //< Total number of kernel pages used in binary

// BIOS info on installed HDDs, max. 8 devices with each max. 0x42 bytes
extern unsigned char bioshddinfo[ 8 * 0x42 ];

/**
 * Extra pages allocated for module during boot
 */
extern int extraPagesAllocated;


#endif
