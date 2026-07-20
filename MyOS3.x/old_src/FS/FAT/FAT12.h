#ifndef FAT12_H
#define FAT12_H

namespace MyOS { namespace FS { namespace FAT {
  
/// Represents a 12-bit File Allocation table as it is read from a floppy disk
/**
 * This class makes some assumptions about constants that are generally true
 * for Floppy disks, but not so for HDs etc:
 * 
 * @) Sectors/FAT  = 9
 * @) Bytes/sector = 512
 * 
 * So sizeof(FAT) = 9*512 bytes, #entries(FAT)=(9*512)/1.5 = 6*512 = 3072
 */
class FAT12
{
public:

   /// @return 12-bit value corresponding to index i
   inline u32 operator[]( size_t i ) const { 
     
     // Calculate base (of per 3 dwords), 8 entries per series
     register u32 d = 3*(i/8);
     
     // check it it falls on a boundary between 2 dwords)     
     switch (i%8)
     {
     case 0:
         // 8 entries of 12 bits per 3*32 bits
         d = entries[ d ]; break;

     case 1:
         d = (entries[ d ] >> 12); break;
       
     case 2:   // boundary: 8+4 bits
         d = shr<24>( &entries[ d ] ); break;

     case 3:
         d = (entries[ d + 1 ] >> 4); break;

     case 4:
         d = (entries[ d + 1 ] >> 16); break;

     case 5:   // boundary: 4 MSB of dword[1] and 8 bits of dword[2]
         d = shr<28>( &entries[ d + 1 ] ); break;

     case 6:
         d = (entries[ d + 2 ] >> 8); break;

     case 7:
         d = (entries[ d + 2 ] >> 20); break;              
     }
     return d & 0xFFF;
   }
      

private:
   // 9 sectors/FAT12, 512 bytes per sector, 12 bits per entry
   u32 entries[ ((9 * 512 * 8) / 12) / sizeof(u32) ];
};  
  
}}}   // namespace

#endif
