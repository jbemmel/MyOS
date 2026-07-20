/***************************************************************************
                          IInterface.h  -  description
                             -------------------
    begin                : Mon Mar 4 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

An attempt to design a COM-like component model. I dont like the 'queryInterface'
on every interface, but reference counting is OK

I want to add an XML description with UUID (use 'uuidgen -t'), and the means to get at this
description (perhaps simply a public attribute)
 ***************************************************************************/
#ifndef IINTERFACE_H
#define IINTERFACE_H

#include "myosresult.h"
#include "Version.h"
#include "UUID.h"
#include "buffer.h"
#include "NVPAIR.h"
#include "Name.h"

/// The namespace for all MyOS related classes
namespace MyOS { 

namespace Context { class IContext; }
using Context::IContext;

namespace IO {
    class IStream;
    class OStream;
}

namespace Exceptions {
   class CastException {
     
   };
};
   
namespace Core {

class IComponent;
class IService;

typedef const char* const myos_idl_t;

/**
 * The mother of all interfaces
 */
class IInterface
{
private:
	const VERSION version;  // *implementation* version
	IComponent &component;  // the component of which this interface is a part

protected:
	IInterface( IComponent& c, VERSION v ) throw()
   : version(v), component(c)
   {

   }

	// deallocation must be done in derived class' destructor
  inline void operator delete( void* ) {}
  
  // prevent code bloat! should add 'throw()', but requires destructors
  // in all derived classes
  inline virtual ~IInterface() /* throw() */ {}

    /**
     *  Callback method invoked when a component requests this interface
     *  @return @todo result ? deny access ?
     *  @note Default empty
   */
  virtual myos_result_t onAccess( IComponent& byComponent ) { return E_MYOS_SUCCESS; }

public:
	/**
	 * @return Description of this interface in IDL (XML)
	 */
	virtual const buffer getIDL() const = 0;
   
	/**
     * Convenience method, returned is part of IDL
     */   
	virtual const UUID& getUUID() const = 0;

	/**
     * Convenience method, returned is part of IDL
     */   
	virtual const myos_name_t getName() const = 0;

	/**
	 * @return The component by which this interface is offered
	 */
	inline IComponent& getComponent() const { return component; }

   /** @return the version of the *implementation* */
   inline VERSION getVersion() const { return version; }

   /**
    * Safe typecast by comparing pointer to IDL
    * 
    * @throws CastException when the XML description pointer does not match
    */
   template <typename TO>
   inline TO* castTo() const // throw (Exceptions::CastException)
   {
      // type check by comparing IDL pointers (or address of those?)
      if ( (u8*) TO::getXML() == this->getIDL().getData()) return (TO*) this;
      
      // Some code depends on being able to use a 0 pointer return
      // throw Exceptions::CastException();
      return 0;      
   }


   /** @todo: implement */
   virtual void release() {}

	// General methods that most interfaces should support

   /**
    * Gets data from the interface
    * @param call       : Call to make, "" (empty) or "IDL" will return idl
    * @param parameters : (optional) parameters to the call, null if none
    * @param output     : output stream to receive data
    */
	virtual myos_result_t
      get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const;

	virtual myos_result_t put( const char* const call, NVPAIR parameters[], IO::IStream& input ) {
		return E_MYOS_E_NOTIMPL;
	}

   /**
    * For efficiency, a caller may open a put stream to write directly to the
    * destination it wants, without copying bytes
    *
    * @param
    * @param
    * @param maxbytes : if !=0, number of bytes expected by caller
    *
    * @return pointer to OStream if supported, <code>null</code> otherwise
    * @note If not supported, caller may use repeated 'put' calls instead
    * @note if supported, called interface must close() the stream when callback returns
    */
   class IPutReceiver {
      public:
         virtual void onPut( IO::OStream& openPutStream ) = 0;
   };

   virtual bool openPutStream(
      IPutReceiver& rx, const char* const call, NVPAIR params[], size_t maxbytes=0 )
   {
      return false;   // default not supported
   }

};

// dummy implementation
// inline IInterface::~IInterface() {}

}}	// namespace

#endif
