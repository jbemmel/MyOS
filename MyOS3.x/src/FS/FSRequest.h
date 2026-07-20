#ifndef FSREQUEST_H
#define FSREQUEST_H

#include "Atomic/Queue2.h"

namespace MyOS { namespace FS {

class File;
class FSCompletionHandler;

/// Represents a request to a file system
/**
 * Like devices, file systems offer non-blocking event based interfaces
 */
class FSRequest : public Queue::Item {

   FSCompletionHandler& handler;
public:
  
};

/// Represents a FS client
/**
 * Callback methods to be implemented by FS clients
 */
class FSCompletionHandler {

public:
   /// Called when an open request has succeeded
   /**
    * Files are memory mapped, so from now on the application/client can
    * read / write from the file using pointer arithmetic
    */
   virtual bool onOpen( FSRequest& openrq, File& file ) = 0;
   virtual void onError( FSRequest& openrq ) = 0; 
   
   /// @return true if file should be included in some resultset
   virtual bool onList( const myos_name_t& name ) = 0; 
   
   /// Use with 'onList' ?
   virtual bool onListingDone( void* resultset ) = 0;      
};
 
 
}} // namespace

#endif
