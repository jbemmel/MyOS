#ifndef PACKET_H
#define PACKET_H

#include "debug.h"

namespace MyOS {
namespace Networking {

/**
 * A generic, queueable packet buffer structure very similar to Linux' skbuf
 * 
 */
class Packet
{
    // void operator delete( void* );    //< Disallow, call free instead
    
public:
    inline Packet( u8 * const _bytes, size_t _maxBytes ) 
        : prev(0), next(0), bytes(_bytes), maxBytes(_maxBytes), curIndex(_bytes)
        , refcount(1), token(0) {}
    
    inline void append( u8* bytes, size_t count ) {
        memcpy( curIndex, bytes, count );
        curIndex += count;
    }
    
    inline u8 const * getFirstByte() const { return bytes; }
    
    /**
     * Returns a pointer to the bytes at the current index
     */
    inline u8* getData() const { return curIndex; }

    /**
     * Returns the amount of data received in this packet
     */
    inline u32 getRxSize() const { return maxBytes; }
        
    /**
     * Returns the amount of data available in this packet
     */
    inline u32 getTxSize() const { return curIndex - bytes; }
    
    /**
     * Returns a reference to the data at the current index, interpreted as the given type
     */
    template <class H>
        inline H& getDataAs( u32 offset=0 ) const { 
        ASSERTION( curIndex+offset < bytes+maxBytes, E_CRITICAL );
        return * (H*) &curIndex[offset]; 
    }
    
    /**
     * Returns a reference to the data at the current index, interpreted as the given type
     * Also advances current index
     */
    template <class H>
        inline H& removeHeader() { 
        ASSERTION( curIndex < bytes+maxBytes, E_CRITICAL );
        H& result = * (H*) curIndex;
        curIndex += sizeof(H);
        return result;
    }
    
    /**
     * Reserves room for n bytes
     * @param n - number of bytes to reserve
     */
    inline u8* reserve( size_t n ) {
        ASSERTION( curIndex+n < bytes+maxBytes, E_CRITICAL );
        
        return curIndex += n;
    }
    
    inline void free() {
        if (--refcount==0) {
            delete this;
        }
    }
    
    inline void* getToken() const   { return token; }
    inline void setToken( void* t ) { this->token = t; }
    
private:    
    Packet *prev, *next;    //< For inclusion in doubly linked lists
   
    u8 const * bytes;       //< Byte contents of this packet, allocated separately(!)
    const size_t maxBytes;  //< Maximum number of bytes
    
    u8* curIndex;
    
    /* TODO atomic32 */ int refcount;
    void* token;
};

}}

#endif
