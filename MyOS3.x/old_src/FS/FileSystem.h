
class FileNotFoundException;

class File;

class FileSystem  /* ...Map ? for subdirectories.. */
{
public:
   /// Attemps to interpret data on device according to this FS
   virtual bool apply( IBlockDevice& toDevice ) = 0;
   
   // non-blocking?? Or perhaps 'open' is easier to use synchronized...
   virtual File& openFile( const myos_name_t& filename ) 
      throw (FileNotFoundException) = 0;

   // TODO: See if this is useful
   virtual void openFileNonBlocking( 
      const myos_name_t& filename, FSCompletionHandler& handler ) = 0;

   // This can be useful non-blocking, eg to populate a GUI listview
   virtual void listFiles( FSCompletionHandler& handler ) = 0;

   // No flag bits in the interface! This is like open, but will not
   // allow paging to take place
   virtual File& openFileNonPaged( const myos_name_t& filename ) 
      throw (FileNotFoundException) = 0;
   
   
   
};