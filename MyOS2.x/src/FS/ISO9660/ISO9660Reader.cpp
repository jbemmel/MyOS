#include "ISO9660Reader.h"

#include "Devices/IORequest.h"
#include "MultiThreading/IThread.h"

#include "ISO9660FileInfo.h"
#include "PrimaryVolumeDesc.h"

#include "FS/ISO9660/ISO9660Component.h"
#include "MM/new.h"

namespace MyOS { namespace FS { namespace ISO9660 {

using MultiThreading::IThread;

void*
ISO9660Reader::operator new( size_t s ) 
{
   return MyOS::FS::ISO9660::allocate( s );
}

void
ISO9660Reader::operator delete( void* _this ) 
{
   MyOS::FS::ISO9660::deallocate( _this, sizeof(ISO9660Reader) );
}

ISO9660Reader::ISO9660Reader( IBlockDevice& d ) // INITSECTION ??
{
   if (d.open() == E_MYOS_SUCCESS) {   
      device = &d;   
         
      // prefetch root here, and check that it is a ISO9660 system
      // For now, use a synchronous version ?
   
      // Sectors 0..15 are all zero, great standard...
      device->readBlocks( 16, 1, 1, *this );

      // return r==E_MYOS_SUCCESS;
   } else {
      DPRINTK( "\nUnable to open device" );
      // should throw IOException here
   }      
}

// should be virtual, is not yet
const IDirectory&
ISO9660Reader::getRoot() const
{
   DPRINTK( "\ngetRoot" );

   // could throw exception if not valid fs or not yet initialized
   if (!root.isInitialized()) {
      DPRINTK( "\nTODO: Throw exception, root not initialized" );
      while (!root.isInitialized()) { // ASMVOLATILE("hlt");
         CHECKPOINT('w');
         for (int i=0; i<1000000; ++i);
      }
         // does not work...
         // IThread::do_usSleep( 100000 * 1000 );
      //}
      
      // or wait until it is done  
   }
   return root;
}

// virtual 
u32
ISO9660Reader::onReadDone( IORequest& rq, const buffer& data )
{
   DPRINTK( "\nISO9660Reader onReadDone start=%u data=%x %s", 
      rq.getStartBlock(), data.getData(), data.getData() );

   if (rq.getStartBlock()==16) {
      PrimaryVolumeDesc* desc = (PrimaryVolumeDesc*) data.getData();
      DPRINTK( "\nVolume desc totalSectors=%u root.firstSector=%u size=%u", 
         desc->totalSectors, desc->root.firstSector, desc->root.filesize ); 
         
      return rq.reissue( desc->root.firstSector, desc->root.filesize / 2048 );         
   } else {
      // Assume this is the read for the ROOT directory 
      FileInfo* entry = (FileInfo*) data.getData();
      DPRINTK( "\ne[0].recsize=%u", entry->infosize );
      root.init( entry );
   }
       
   // Done, allow buffer to be released
   return E_DONE;
}

// virtual 
void 
ISO9660Reader::onError( u32 lbnStart, size_t count )
{
   if (lbnStart==0) {
      DPRINTK( "\nError reading ROOT blocks, ISO9660 FS unusable" );  
   }      
}

// virtual 
myos_result_t
ISO9660Reader::readFile( const myos_name_t& filepath,IO::OStream& out ) const 
{
   // NOTE: open call is *blocking*
   IOpenFileAsync& file = this->processDirs(filepath).openAsync( *device );

   DPRINTK( "\nISO9660Reader::readFile open ok name=%n len=%d size=%d", 
      &file.getName(), file.getName().length, file.getSize() );

   // Allocated dynamically, to support parallel calls
   class Completion : public IFileAsyncCompletion {
      IOpenFileAsync& file;
      IO::OStream& out;
      
      friend class ISO9660Reader;   // to prevent warning on destructor   
   public:     
      Completion( IOpenFileAsync& f, IO::OStream& o ) 
         : file(f), out(o) 
      {

      }
   private:
      // 'virtual' not really needed, but keeps the compiler happy
      virtual ~Completion() { 
         file.close();  // this unlocks thread for deletetion
      }
   
      void operator delete( void* _this ) {
         ::operator delete( _this, THREAD );       
      }
           
      // Called for partial blocks
      virtual void onReadDone( AsyncHandle& h ) {
         // DPRINTK( "\nCompletion::onReadDone bufsize=%d", h.getBuffer().getSize() );
         out.putBuffer( h.getBuffer() );               
         if (h.isLast()) {
            delete this;
         }            
      }
      
      virtual void onError( AsyncHandle& h ) {
         delete this;
      }
   };

   file.readAsync( 0, file.getSize(), * new (THREAD) Completion(file,out) );
   return E_MYOS_PROCESSING;
}


// virtual 
IOpenFile& 
ISO9660Reader::openFilePaged( const myos_name_t& filepath )
   throw (FileNotFoundException, FSException)
{
   return processDirs(filepath).openPaged( *device );
}   

// virtual 
IOpenFileAsync& 
ISO9660Reader::openFileAsync( const myos_name_t & filepath ) const
   throw (FileNotFoundException, FSException)
{
   return processDirs(filepath).openAsync( *device );
}

// private
ISO9660FileInfo&
ISO9660Reader::processDirs( const myos_name_t& filepath ) const
   throw (FileNotFoundException)
{

   ISO9660Directory* dir = (ISO9660Directory*) &getRoot();
   myos_name_t path = filepath;
   s32 s=-1; 
   do {
      s = filepath.substring(s+1).indexOf('/');

      ISO9660FileInfo* r;   // XX not nice
      if (dir->findNode( s>=0 ? path.substring(0,s) : path, VERSION::ANY(), (IInterface*&) r ).length == 0) {
         if ((dir = r->loadIfDirectory( *device, *dir ))) {
            path = path.substring(s+1);
         } else {
            // found the file
            return *r;        
         }
      } else throw FileNotFoundException( );   // TODO: add path info
   } while (1);

   throw FileNotFoundException( );
}

// virtual 
void 
ISO9660Reader::deleteFile( const myos_name_t & filepath ) 
   throw (FSException)
{
   throw FSException(); // read-only FS    
}


// virtual 
void 
ISO9660Reader::unmount() throw (FSException)
{
   if (device!=0) {  // ==0 when open failed

      // TODO: Wait for pending operations ?

      // device->releaseBuffer( fatbuffer );
      delete this;	// calls device->close()
   } else throw FSException();   // not mounted   
}

// virtual 
void 
ISO9660Reader::renameFile( const myos_name_t & filepath, const myos_name_t& newname ) 
   throw (FSException)
{
 
}

// virtual 
myos_result_t
ISO9660Reader::enumerateFiles( IO::OStream& out ) const {
   return getRoot().list( myos_name_t(""),0,out );
}
 
  
}}}   // namespace
