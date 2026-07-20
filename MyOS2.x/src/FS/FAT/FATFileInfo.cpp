#include "FATFileInfo.h"
#include "FATDirEntry.h"
#include "FATDirectory.h"
#include "FATFileFragment.h"
#include "FAT12.h"

#include "FS/FAT/FATComponent.h"
#include "MM/new.h"

#include "Devices/IORequest.h"

#include "MultiThreading/IThread.h"

namespace MyOS { namespace FS { namespace FAT {

using MultiThreading::Thread;
using MultiThreading::IThread;

void*
FATFileFragment::operator new[]( size_t n )
{
   // On return, GCC allocates a 4-byte 'size' as first element, and sets it
   // DPRINTK( "\nnew[] n=%d sizeof(FATFileFragment)=%d", n, sizeof(FATFileFragment) );
   return MyOS::FS::FAT::allocate( n );
}

void
FATFileFragment::operator delete[]( void* p, size_t n )
{
   // n is the total size of memory allocated
   // DPRINTK( "\np=%x n = %d", p, n );
   MyOS::FS::FAT::deallocate( p, n );
}

FATFileInfo::FATFileInfo( const FATFileNode& n, const FATDirEntry& e, const FAT12& fat )
: node(&n), filesize( e.getSize() ), dir(0), openfiles(0)
{
/*   
   {
      MyOS::Devices::LPT::LPTStream printer;
      e.print( printer );
   }
*/   

   // For directories, size==0, so choose a fixed (max) size
   if (e.isDirectory()) {
      filesize = 1 * _4KB;       // set this! Else for loops fail  
      dir = (FATDirectory*) 1;   // mark it as dir
   }
   
   // parse FAT, construct FileFragments per 4KB, use noThrow alloc

   u32 pages = (filesize + _4KB - 1) / _4KB;

   ASSERTIONv( pages, E_ERROR, e.getSize() );
   
   // XX if file later expands by writing, cannot guarantee sequential!
   // Perhaps implement writing as another FileInfo part alltogether?
   FATFileFragment *fragments = frags = new FATFileFragment[pages];
   //DPRINTK( "\nsize=%d #clusters=%d #fragments=%d frags=%x", 
   //   filesize, (filesize + 512-1)/512 , pages, frags );

   // Sanity check, should normally fit but
   if (fragments==0) {
      DPRINTK( "\nnew() failed, perhaps %d is larger than component blocksize?", sizeof(FATFileFragment) * (e.getSize() + _4KB - 1) / _4KB );
      return;  // should throw IOException here
   }
   
   // Dont do this, it clears class pointers!
   // memset_aligned( fragments, 0, pages * sizeof(FATFileFragment) );
   
   u8 range = 0;
   for (u32 c = e.getFirstCluster(), p=c-1,n=0,i=0;  ; ++i) {
      // DPRINTK( "\nCluster=%x currange=%d", c, range );
      if (p+1==c) {  // continue current range
         ++range;
      } else {       // start new range
         fragments->setBlock( n++, (c-range) | range<<12 );
         range = 1;
      }

      // break it into max. 8 blocks per fragment
      if (i==8) {
         --range; // correct this one
         fragments->setBlock( n, (c-range) | range<<12 );
         // DPRINTK( "\nMoving to next fragment, cur val = %x", (c-range) | range<<12 );
         ++fragments;
         i = n = 0;
         range = 1;
      }
      
      p = c;
      if ((c = fat[c]) >= 0xFF8) {
         // Complete final fragment
         fragments->setBlock( n, (p-range+1) | range<<12 );
         //DPRINTK( "\nLast fragment n=%X = %x @%x", n, (p-range+1) | range<<12, fragments );
         break;         
      }
   }
   //DPRINTK( "\nthis=%x Name: %n", this, &getName() );
}

FATFileInfo::~FATFileInfo()
{
   // DPRINTK( "\n~FATFileInfo(%x) waiting for openfiles=%d", this, (u32) openfiles );

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
FATFileInfo::openPaged( IBlockDevice& dev ) const
{
   // TODO: 1. Check if not already open in some process   

   // Fix device for fragments, best way?
   for (int f=(filesize+_4KB-1)/_4KB; --f>=0; ) {
      frags[f].setDevice( dev ); 
   }   
   
   class OpenFile : public IOpenFile {
      const FATFileInfo& info;   // (for now) read-only
      u8* vmem;    // Virtual memory (valid in context of process p)
      // TODO: Process& p

      inline void operator delete( void* _this ) {
         // NOTE: autosize-allocated!
         // DPRINTK( "\nOpenFile::delete this=%x", _this );
         ::operator delete( _this, THREAD );        
      }
      
   public:         
       OpenFile( const FATFileInfo& n ) : info(n) {
          
            // for now, read-only
            size_t pages = (info.getFileSize() + _4KB - 1) / _4KB;
            vmem = (u8*) FATComponent::getDemandPaging().map( info.getFragments(), sizeof(FATFileFragment), pages, true );            
       }
       
      virtual const myos_name_t& getName() const { return info.getName(); }
      virtual size_t getSize() const             { return info.getFileSize(); }

      virtual size_t read( u32 offset, size_t count, const buffer& buf ) const 
         throw (FSException) 
      {
         ASSERTION( count <= buf.getSize(), E_ERROR );
         
         DPRINTK( "\nFATFileInfo::read count=%d-> should cause pagefault...", count );

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
            physadr_t f = FATComponent::getDemandPaging().unmap( &vmem[p*_4KB] );
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

using namespace Devices;

IOpenFileAsync&
FATFileInfo::openAsync( IBlockDevice& dev ) const
{
   // this->open(dev); no need for this

   class OpenFile : public IOpenFileAsync {
      IBlockDevice& dev;
      const FATFileInfo& node;   // (for now) read-only
      Thread& thread;            // context, locked while file open

      inline void operator delete( void* _this ) {

         Thread& t = ((OpenFile*)_this)->thread;

         // NOTE: autosize-allocated!
         ::operator delete( _this, THREAD );
         
         // Decrease lock-count for associated thread
         t.asyncDone(false);
      }
      
   public:         
      OpenFile( IBlockDevice& d, const FATFileInfo& n ) 
         : dev(d), node(n), thread( Thread::currentThread() ) 
      {
         thread.addPending();
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
            
            inline void operator delete( void* _this ) {
               // NOTE: autosize-allocated!
               ::operator delete( _this, THREAD );        
            }
            
         public:
            Passer( const IOpenFileAsync& file, u32 o, size_t c, IFileAsyncCompletion& cb ) 
               : handle(file,o,c), callback(cb) {}
            
            AsyncHandle& getHandle() { return handle; }
            
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
            FATFileFragment* frag = &node.getFragments()[f];
            u16 b;
            for (u32 i=0; (b=frag->getNextBlock( i )); ++i) {
               // Could accumulate multiple*4KB blocks here if needed...
               dev.readBlocks( b&0xFFF, b>>12, b>>12, *p );
            }
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

FATDirectory*
FATFileInfo::loadIfDirectory( 
   Devices::IBlockDevice& dev, const FAT12& fat12, FATDirectory& p )
{
   // TODO: Check attributes that *this is really a directory
   if ((u32)dir&1 /* && this->isDirectory() */ ) {
      dir = new FATDirectory( *this, p );

      // Directory has no size -> read until done, page faulting in
      IOpenFile& d = this->openPaged( dev );
      DPRINTK( "\nloadIfDirectory memptr=%x", d.getMemoryPtr() );
      dir->init( (FATDirEntry*) d.getMemoryPtr(), 9999, fat12 );
      d.close();
      return dir;
   } else return (FATDirectory*) ((u32)dir & ~1);
}

}}}   // namespace
