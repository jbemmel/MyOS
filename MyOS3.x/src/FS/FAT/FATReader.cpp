#include "FATReader.h"

#include "FATBootSector.h"
#include "FAT12.h"
#include "FATDirEntry.h"
#include "FATFileInfo.h"

#include "Devices/IORequest.h"
#include "MultiThreading/IThread.h"

#include "FS/FAT/FATComponent.h"
//#define CURCOMPONENT MyOS::FS::FAT::FATComponent
#include "MM/new.h"

namespace MyOS { namespace FS { namespace FAT {

using MultiThreading::IThread;

void*
FATReader::operator new( size_t s ) 
{
   return MyOS::FS::FAT::allocate( s );
}

void
FATReader::operator delete( void* _this ) 
{
   MyOS::FS::FAT::deallocate( _this, sizeof(FATReader) );
}

FATReader::FATReader( IBlockDevice& d ) // INITSECTION ??
{
   if (d.open() == E_MYOS_SUCCESS) {   
      device = &d;   
         
      // prefetch root here, and check that it is a FAT system
      // For now, use a synchronous version ?
   
      // returns result asynchronous...
      u8 count = sizeof(FATBootSector)/device->getBlocksize();
      device->readBlocks( 0, count, count, *this );

      // return r==E_MYOS_SUCCESS;
   } else {
      DPRINTK( "\nFATReader Unable to open device" );
      // should throw IOException here
   }      
}

// should be virtual, is not yet
const IDirectory&
FATReader::getRoot() const
{
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
FATReader::onReadDone( IORequest& rq, const buffer& data )
{
   // DPRINTK( "\nFATReader onReadDone start=%u data=%x", rq.getStartBlock(), data.getData() );

   if (rq.getStartBlock()==0) {
      FATBootSector* b = (FATBootSector*) data.getData();
      rootEntries = b->getRootEntries();
      //DPRINTK( "\nROOT entries: %d boot@%x firstCluster=%d", 
      //   rootEntries, b, b->getSector(0) );
#ifdef DEBUG      
      // b->dump(); while (1);
#endif      

      // NOTE: This reads 9 sectors, which allocates a DMA buffer for 16
      // Better to do 8+1 ? or release 7 left-over buffers below?
      return rq.reissue( 1, sizeof(FAT12) / device->getBlocksize() );
   } else if (rq.getStartBlock()==1) {  // FAT0
      fatbuffer = data;      
//      FAT12* fat = (FAT12*) data.getData();
//      for (int e = 0; e<5; ++e) {
//         DPRINTK( "\nFAT12 [%d]=%x", e, (*fat)[e] );
//      }
      // assuming sizeof(FAT12)=9 sectors, could read from bootsector
      u32 es = (rootEntries * sizeof(FATDirEntry) + device->getBlocksize() - 1);
      return E_KEEPBUFFER | rq.reissue( 1+2*9, es / device->getBlocksize() );
   } else {
      root.init( (FATDirEntry*) data.getData(), rootEntries, * ((FAT12*) fatbuffer.getData()) );

      // Could delay this
      // device->releaseBuffer( fatbuffer );
   }      
   
   // Done, allow buffer to be released
   return E_DONE;
}

// virtual 
void 
FATReader::onError( u32 lbnStart, size_t count )
{
   if (lbnStart==0) {
      DPRINTK( "\nError reading ROOT blocks, FAT FS unusable" );  
   }      
}

// virtual 
myos_result_t
FATReader::readFile( const myos_name_t& filepath,IO::OStream& out ) const {
  
   // NOTE: open call is *blocking*
   IOpenFileAsync& file = this->processDirs(filepath).openAsync( *device );

   DPRINTK( "\nFATReader::readFile open ok name=%n len=%d size=%d", 
      &file.getName(), file.getName().length, file.getSize() );

   // Allocated dynamically, to support parallel calls
   class Completion : public IFileAsyncCompletion {
      IOpenFileAsync& file;
      IO::OStream& out;
      
      friend class FATReader;   // to prevent warning on destructor   
   
   public:     
      inline Completion( IOpenFileAsync& f, IO::OStream& o ) 
         : file(f), out(o) 
      {

      }
      
      inline void operator delete( void* _this ) {
         ::operator delete( _this, THREAD );       
      }
      
   private:
      // 'virtual' not really needed, but keeps the compiler happy
      virtual ~Completion() { 
         file.close();  // this unlocks thread for deletetion
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
FATReader::openFilePaged( const myos_name_t& filepath )
   throw (FileNotFoundException, FSException)
{
   return processDirs(filepath).openPaged( *device );
}   

// virtual 
IOpenFileAsync& 
FATReader::openFileAsync( const myos_name_t & filepath ) const
   throw (FileNotFoundException, FSException)
{
   return processDirs(filepath).openAsync( *device );
}

/**
 * NOTE: In this way, all 'open' operations must go through the FATReader
 * It should also be possible on individiual subdirectories (ie when clicked
 * in some GUI) but that is already difficult, wince they wont know the 
 * device...
 */
 
// private
FATFileInfo&
FATReader::processDirs( const myos_name_t& filepath ) const
   throw (FileNotFoundException)
{
   FATDirectory* dir = (FATDirectory*) &getRoot();
   myos_name_t path = filepath;
   s32 s=-1; 
   do {
      s = filepath.substring(s+1).indexOf('/');

      FATFileInfo* r;   // XX not nice
      if (dir->findNode( s>=0 ? path.substring(0,s) : path, VERSION::ANY(), (IInterface*&) r ).length == 0) {
         if ((dir = r->loadIfDirectory( *device, * (const FAT12*) fatbuffer.getData(), *dir ))) {
            path = path.substring(s+1);
         } else {
            // found the file
            return *r;        
         }
      } else throw FileNotFoundException( /* filepath */ );        
   } while (1);
}

// virtual 
void 
FATReader::deleteFile( const myos_name_t & filepath ) 
   throw (FSException)
{
    
}

// virtual 
void 
FATReader::unmount() throw (FSException)
{
   DPRINTK( "\nFS::unmount" );
   if (device!=0) {  // ==0 when open failed

      // TODO: Wait for pending operations ?

      device->releaseBuffer( fatbuffer );
      delete this;   // calls device->close()
   } else throw FSException();   // not mounted   
}

// virtual 
void 
FATReader::renameFile( const myos_name_t & filepath, const myos_name_t& newname ) 
   throw (FSException)
{
 
}

// virtual 
myos_result_t
FATReader::enumerateFiles( IO::OStream& out ) const {
   return getRoot().list( myos_name_t(""),0,out );
}
 
  
}}}   // namespace
