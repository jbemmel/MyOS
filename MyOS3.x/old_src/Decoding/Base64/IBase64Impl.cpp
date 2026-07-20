#include "IBase64Impl.h"
#include "Decoding/Base64/Base64Component.h"

#include "IO/IStream.h"
#include "buffer.h"

//#define CURCOMPONENT MyOS::Decoding::Base64::Base64Component
#include "MM/new.h"

namespace MyOS { namespace Decoding { namespace Base64 {

IBase64Impl::IBase64Impl( MyOS::Core::IComponent& c )
: IBase64( c, VERSION(1,0) )
{

}

// index by [char-'+'] -> max. index = 122 - 43 = 79 (7 bit)
const u8 IBase64Impl::decode64_0[] = {       // ASCII

    62,                                     // '+' (43 decimal)
    0xC0, 0xC0, 0xC0,                       // invalid
    63,                                     // '/' (47 decimal)
    52,53,54,55,56,57,58,59,60,61,          // '0'..'9'
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,     // invalid, first '=' could be treated specially

    // 'A' = 65
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,
    14,15,16,17,18,19,20,21,22,23,24,25,    // 'A'..'Z'
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,          // invalid


    // 'a' = 97
    26,27,28,29,30,31,32,33,34,35,36,37,38,

    39,40,41,42,43,44,45,46,47,48,49,50,51, // 'a'..'z'

    // fill up with invalid until index 128
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
};

bool 
IBase64Impl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   decode64_1 = (u16*) allocate( 128 * sizeof(u16) );
   decode64_2 = (u32*) allocate( 128 * sizeof(u32) );
   decode64_3 = (u32*) allocate( 128 * sizeof(u32) );
   for (u32 i=0; i<sizeof(decode64_0); ++i) {
   		u8 d64 = decode64_0[i];
   		if (d64 != 0xC0) {
	        decode64_1[i] = d64 << 6;
    	    decode64_2[i] = d64 << 12;
        	decode64_3[i] = d64 << 18;
   		} else {
	        decode64_1[i] = 0xF000;
    	    decode64_2[i] = 0xFF000000;
        	decode64_3[i] = 0xFF000000;   			
   		}
   } 
	return true;
}

void 
IBase64Impl::deinit( IContext& context )
{
   deallocate( decode64_3, 128 * sizeof(u32) );
   deallocate( decode64_2, 128 * sizeof(u32) );
   deallocate( decode64_1, 128 * sizeof(u16) );
}

// virtual 
myos_result_t IBase64Impl::decode( IO::IStream& in )  {

   // There are many ways to do this: stack allocation is one
   // Better might be to use getPutBuffer on the outputstream (now cout)
   // u8 *output = (u8*) __builtin_alloca( ( in.getBuffer().getSize() * 4 ) / 3 );
   
   u8* output = new (THREAD) u8[ ( in.getBuffer().getSize() * 4 ) / 3 ];   
   u8* end = decode64( (char*) in.getBuffer().getData(), output );
   Base64Component::getInstance().iDisplay->cout().putBuffer( buffer(output,end-output) );
   ::operator delete( output, THREAD );
   return E_MYOS_SUCCESS;
}

/**
 *  Decodes a base64 encoded inputstring in-memory
 *
 *  @param  src [in] - Base64 encoded string, length will always be multiple of 4 bytes for valid base64
 *  @note May contain whitespace, but only 4-byte aligned ' '' ''\r''\n' (4 chars)
 *  @param  dest     - Target buffer, can be equal to src
 *
 *  @return dest at point where writing has stopped
 * 
 *  TODO: This currently does not check for end-of-input, could add length param
 */
u8*
IBase64Impl::decode64( char* src, u8* dest ) const
{
    do {
        register u32 d = *((u32*) src);
        src += 4;

        if ((d-=0x2b2b2b2b)&0x80808080) { // subtract '+', check for sign bits
            // Check for whitespace, supported is *only* \0x20\0x20\0xd\0xa
            if (d!=(u32)(0x0a0d2020-0x2b2b2b2b)) break;
            // skip it
            continue;
        }

        register u32 v = decode64_3[ (d) & 0x7f ];  // Little endian !
        v |= decode64_2[ (d>>=8) & 0x7f ];          // Little endian !
        if (v&0xFF000000) break;                    // Stop if v0 or v1 is invalid
        u32 v2 = decode64_1[ (d>>=8) & 0x7f ];      // Little endian !
        u32 v3 = decode64_0[ (d>>=8) & 0x7f ];      // Little endian !

        v |= v2 | v3;
        // char* found = (char*) &v;

        //    Note: Through proper mangling of the tables it is possible
        //    to reverse the order here: found[0],found[1],found[2]

        //    ... but very complicated!

        *dest++ = (v>>16)&0xFF;  // found[2];     // 2 bits of v1 + v0
        if (v2==0xF000) break;
        *dest++ = (v>>8) &0xFF;  // found[1];     // 4 bits of v2 + 4 bits of v1
        if (v3==0xC0) break;

        *dest++ = (v&0xFF);      // found[0];     // v3 + 2 bits of v2
    } while (1);

    return (u8*) dest;
}

}
}}  // namespaces
