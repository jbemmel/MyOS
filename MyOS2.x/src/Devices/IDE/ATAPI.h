#ifndef ATAPI_H
#define ATAPI_H

namespace MyOS { namespace Devices { namespace IDE {

// Table 37 of ATAPI std, dont confuse these with SCSI command values...
enum E_BASIC_OPCODE {
   E_INQUIRY      = 0x12,
   E_READ         = 0x28,
   E_SEEK         = 0x2B,  // ATA version now obsolete, redefined as 'READ STREAM'
                           // Linux does not use this one!
   
   E_PLAY_AUDIO   = 0x45,  // 16-bit length
   
   E_READ_CD      = 0xBE,
   E_READ_CD_MSF  = 0xB9,
};

/// Commands that use 32bit length
enum E_EXTENDED_OPCODE {
   E_READ_EXT     = 0xA8,
   // .. more ?
};



// From http://www.stanford.edu/~csapuntz/specs/INF-8020.PDF Table 34
/**
 * Note : It's better to implement this as methods that write the bytes
 * one by one, instead of putting them in a buffer...I only need 3 or 4 commands
 */
struct ATAPICommandPacket
{
   u8 opcode;
   u8 __res0;
   nw32_t lba;    // BIG-endian ! Can also be 'allocation length' (byte)

   union {
      struct {
         u8 __res1;
         nw16_t length; // BIG-endian !
         u8 __res2;
      } basic;
      struct {
         nw32_t length; // BIG-endian !
      } extended;
   } type;
   u8 __res3;
   u8 __res4;

   // SEEK
   ATAPICommandPacket( E_BASIC_OPCODE o, u32 _lba )
   {
      memset_aligned( this, 0, sizeof(ATAPICommandPacket) );
      opcode = o;
      lba = _lba;
   }

   
   ATAPICommandPacket( E_BASIC_OPCODE o, u32 _lba, u16 l )
   {
      memset_aligned( this, 0, sizeof(ATAPICommandPacket) );
      opcode = o;
      lba = _lba;
      type.basic.length = l;
   }

   // For extended commands (32bit length)
   ATAPICommandPacket( E_EXTENDED_OPCODE o, u32 _lba, u32 l )
   {
      memset_aligned( this, 0, sizeof(ATAPICommandPacket) );
      opcode = o;
      lba = _lba;
      type.extended.length = l;
   }

   
   inline u16* getBytes() const { return (u16*) this; }
   
} PACKED;

}}}   // namespace

#endif
