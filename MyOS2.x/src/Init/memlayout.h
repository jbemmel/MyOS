#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

/**
 * This file defines the physical memory layout for some of the kernel
 * structures
 */
#define PBT_PHYSICAL     0x200000   // 2Mb physical, 1 page
#define PDT0_PHYSICAL    0x201000   // 2Mb physical + 1, maps mainly video memory
#define PDT1022_PHYSICAL 0x202000   // 2Mb physical + 2, maps kernel image
#define STACK_PHYSICAL   0x203000   // 2Mb physical + 3, maps boot stack
#define KERNEL_PHYSICAL  0x204000   // Area where kernel is uncompressed to

// linker constants, take their address to use them!
extern void* _START_OF_COMPRESSED_BINARY_;
extern void* _COMPRESSED_START_LINEAR_;
extern void* _START_OF_CODE_;
extern void* _FREE_KERNEL_SPACE_START_;

extern void* _BSS_SIZE_;

// BIOS info on installed HDDs, max. 8 devices with each max. 0x42 bytes
extern unsigned char bioshddinfo[ 8 * 0x42 ];

#endif
