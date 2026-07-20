#include "ISO9660FileInfo.h"
#include "PrimaryVolumeDesc.h"
#include "ISO9660Directory.h"
#include "ISO9660FileFragment.h"

//#include "FATDirEntry.h"
//#include "FATDirectory.h"
//#include "FATFileFragment.h"
//#include "FAT12.h"

#include "../IOpenFile.h"

#include "FS/ISO9660/ISO9660Component.h"
#include "MM/new.h"

#include "Devices/IORequest.h"

#include "MultiThreading/IThread.h"

namespace MyOS { namespace FS { namespace ISO9660 {

using MultiThreading::Thread;
using MultiThreading::IThread;
using namespace Devices;

void*
ISO9660FileFragment::operator new[]( size_t n )
{
   // On return, GCC allocates a 4-byte 'size' as first element, and sets it
   // DPRINTK( "\nnew[] n=%d sizeof(FATFileFragment)=%d", n, sizeof(FATFileFragment) );
   return MyOS::FS::ISO9660::allocateNoThrow( n );
}

void
ISO9660FileFragment::operator delete[]( void* p, size_t n )
{
   // n is the total size of memory allocated
   // DPRINTK( "\np=%x n = %d", p, n );
   MyOS::FS::ISO9660::deallocate( p, n );
}

ISO9660FileInfo::ISO9660FileInfo( const ISO9660FileNode& n, const FileInfo& f )
: node(&n), filesize( f.getSize() ), dir(0), openfiles(0)
{

   // For directories, size==0 (not for ISO9660)
   if (f.isDirectory()) {
      // filesize = 1 * _4KB;       // set this! Else for loops fail  
      dir = (ISO9660Directory*) 1;   // mark it as dir
   }
   
   // parse ISO9660, construct FileFragments per 4KB, use noThrow alloc
   u32 pages = (filesize + _4KB - 1) / _4KB;

   ASSERTIONv( pages, E_ERROR, f.getSize() );

   ISO9660FileFragment *fragments = frags = new ISO9660FileFragment[pages];
   //DPRINTK( "\nsize=%d #clusters=%d #fragments=%d frags=%x", 
   //   filesize, (filesize + 512-1)/512 , pages, frags );

   // Sanity check, should normally fit but
   if (fragments==0) {
      DPRINTK( "\nnew() failed, perhaps %d is larger than component blocksize?", sizeof(ISO9660FileFragment) * (f.getSize() + _4KB - 1) / _4KB );
      return;  // should throw IOException here
   }
   
   // Dont do this, it clears class pointers!
   // memset_aligned( fragments, 0, pages * sizeof(ISO9660FileFragment) );

   for (u32 fr=0; fr<pages-1; ++fr) {
      fragments[fr].setBlocks( f.firstSector + 2*fr, 2 );
   }
   int odd = ((f.getSize() + 2048 - 1) / 2048 ) & 1;
   fragments[pages-1].setBlocks( f.firstSector + 2*(pages-1), 2 - odd );
   
   DPRINTK( "\nthis=%x Name: %n atts=%X", this, &getName(), f.attributes );
}

ISO9660FileInfo::~ISO9660FileInfo()
{
   // DPRINTK( "\n~ISO9660FileInfo(%x) waiting for openfiles=%d", this, (u32) openfiles );

   while (openfiles>0) {
      // IThread::do_yield(0);   // yield, prefer to thread
                                 // holding file open
   }                                 

   if ((u32)dir&~1) {
      delete dir;
      dir = 0;
   }      
      
   // GCC passes the size of the total memory
   delete[] frags;
}

IOpenFile&
ISO9660FileInfo::openPaged( IBlockDevice& dev ) const
{
   // TODO: 1. Check if not already open in some process   

   // Fix device for fragments, best way?
   for (int f=(filesize+_4KB-1)/_4KB; --f>=0; ) {
      frags[f].setDevice( dev ); 
   }   

   /**
    * TODO: Share this code with ISO9660, is really copied
    */
   
   class OpenFile : public IOpenFile {
      const ISO9660FileInfo& info;   // read-only
      u8* vmem;    // Virtual memory (valid in context of process p)
      // TODO: Process& p

   public:
      inline void operator delete( void* _this ) {
         // NOTE: autosize-allocated!
         // DPRINTK( "\nOpenFile::delete this=%x", _this );
         ::operator delete( _this, THREAD );        
      }    
    
      inline OpenFile( const ISO9660FileInfo& n ) : info(n) {
          
            // for now, read-only
            size_t pages = (info.getFileSize() + _4KB - 1) / _4KB;
            vmem = (u8*) ISO9660Component::getDemandPaging().map( info.getFragments(), sizeof(ISO9660FileFragment), pages, true );            
       }
       
      virtual const myos_name_t& getName() const { return info.getName(); }
      virtual size_t getSize() const             { return info.getFileSize(); }

      virtual size_t read( u32 offset, size_t count, const buffer& buf ) const 
         throw (FSException) 
      {
         ASSERTION( count <= buf.getSize(), E_ERROR );
         
         DPRINTK( "\nISO9660FileInfo::read count=%d-> should cause pagefault...", count );

         if ((offset+count) >= info.getFileSize()) {
            // if offset beyond file -> exception
            if (offset >= info.getFileSize()) {
               throw FSException(); // IOException: read beyond EOF 
            }
            count = (info.getFileSize() - offset) | E_EOF;
         }
         
         // memcpy_aligned? cannot be guaranteed..
         memcpy( buf.getData(), &vmem[offset], count & ~E_EOF );
//         DPRINTK( "\nAFTER memcpy to %x, hanging...", buf.getData() );
         //while (1);
         return count;
      }
      virtual void write( u32 offset, size_t count, const buffer& buf ) 
         throw (FSException) { throw FSException(); }

      virtual void close() {

         u32 fs = (info.getFileSize() + _4KB - 1) / _4KB;
         // DPRINTK( "\nFileInfo::OpenFile::close fs=%d vmem=%x", fs, vmem );
         for (u32 p=0; p<fs; ++p) {
            physadr_t f = ISO9660Component::getDemandPaging().unmap( &vmem[p*_4KB] );
            if (f) { // ==0 if it was never paged in
               // TODO: Can check if dirty here, if so and writable -> flush
               info.getFragments()[p].unmapBuffer(f & 0xFFFFF000);
            }
         }

         info.fileClosed();
         delete this;         // call destructor too, if any
      }
      
      virtual void* getMemoryPtr() const { return vmem; }
   };   

   ++openfiles;

   // NOTE: This uses autosize allocation, small waste of space...
   return * new (THREAD) OpenFile( *this );   
}

IOpenFileAsync&
ISO9660FileInfo::openAsync( IBlockDevice& dev ) const
{
   // this->open(dev); no need for this

   class OpenFile : public IOpenFileAsync {
      IBlockDevice& dev;
      const ISO9660FileInfo& node;   // (for now) read-only
      Thread& thread;            // context, locked while file open
      
   public:         
      inline OpenFile( IBlockDevice& d, const ISO9660FileInfo& n ) 
         : dev(d), node(n), thread( Thread::currentThread() ) 
      {
         thread.addPending();
      }
      
      inline void operator delete( void* _this ) {

         Thread& t = ((OpenFile*)_this)->thread;

         // NOTE: autosize-allocated!
         ::operator delete( _this, THREAD );
         
         // Decrease lock-count for associated thread
         t.asyncDone(false);
      }      

      virtual const myos_name_t& getName() const { return node.getName(); }
      virtual size_t getSize() const             { return node.getFileSize(); }

      virtual AsyncHandle& readAsync( u32 offset, size_t count, IFileAsyncCompletion& cb ) const 
         throw (FSException) 
      {
         DPRINTK( "\nreadAsync o=%d c=%d", offset, count );
         
         if ((offset+count) >= node.getFileSize()) {
            // if offset beyond file -> exception
            if (offset >= node.getFileSize()) {
               DPRINTK( "\nOFFSET %d BEYOND EOF: %d", offset, node.getFileSize() );
               throw FSException(); // IOException: read beyond EOF 
            }
            count = (node.getFileSize() - offset); // !not EOF bit
         }

         // Allocate a pass-through object
         class Passer : public IOCompletionHandler {
            AsyncHandle handle;
            IFileAsyncCompletion& callback;
            
         public:
            inline Passer( const IOpenFileAsync& file, u32 o, size_t c, IFileAsyncCompletion& cb ) 
               : handle(file,o,c), callback(cb) {}

            inline void operator delete( void* _this ) {
               // NOTE: autosize-allocated!
               ::operator delete( _this, THREAD );        
            }
            
            inline AsyncHandle& getHandle() { return handle; }
            
            virtual u32 onReadDone( IORequest& rq, const buffer& data ) {

               // Updates offset in page in 'handle'
               AsyncHandle h( handle, data );
               
               // NOTE: Caller assumes sequential reads!
               callback.onReadDone( h );
               
               if (handle.isLastPartial()) {
                  delete this; 
               }               
               return IOCompletionHandler::E_DONE;
            }

            virtual void onError( u32 lbnStart, size_t count ) {
               DPRINTK( "\nPasser::onError -> delete this" );
               delete this;
            }
            
         };

         Passer* p = new (THREAD) Passer(*this,offset,count,cb);

// TODO: Better to do one by one, to guarantee serial reading!

         // Enqueue reads for the relevant fragments
         u32 lastfrag = (offset+count)/_4KB;
         for (u32 f=offset/_4KB; f<=lastfrag; ++f) {
            ISO9660FileFragment* frag = &node.getFragments()[f];
            dev.readBlocks( frag->getFirstBlock(), frag->getCount(), frag->getCount(), *p );
         }
         return p->getHandle();
      }

      virtual void close() {
         // DPRINTK( "\nOpenFile ASYNC::close->delete info=%x", &node );
         node.fileClosed();         
         delete this;   // call destructor too, if any
      }

      virtual void* getMemoryPtr() const { return 0; }   // not supported
   };   

   ++openfiles;

   // NOTE: This uses autosize allocation, small waste of space...
   return * new (THREAD) OpenFile( dev, *this );   
}

ISO9660Directory*
ISO9660FileInfo::loadIfDirectory( Devices::IBlockDevice& dev, ISO9660Directory& p )
{
   if ((u32)dir&1 ) {
      dir = new ISO9660Directory( *this, p );

      // Directory has no size -> read until done, page faulting in
      IOpenFile& d = this->openPaged( dev );
      DPRINTK( "\nloadIfDirectory memptr=%x", d.getMemoryPtr() );
      dir->init( (FileInfo*) d.getMemoryPtr() );
      d.close();
      return dir;
   } else return (ISO9660Directory*) ((u32)dir & ~1);
}

}}}   // namespace
