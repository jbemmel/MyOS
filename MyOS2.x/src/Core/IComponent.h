/***************************************************************************
                          IComponent.h  -  description
                             -------------------
    begin                : Mon Mar 4 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef ICOMPONENT_H
#define ICOMPONENT_H

#include "defs.h"                // STARTSECTION
#include "IInterface.h"

namespace MyOS { namespace Core {

/*
namespace Repository {
   class IRepository;
}

using Repository::IRepository;
*/

class IContainer;

/**
 *	A component exposes one or more interfaces to be used by other components
 *	Components are life-cycle managed
 *
 *  Internally, components can be composed of many classes; they serve as the
 *  single point of entry to a set of required interfaces
 *
 *  They are often (@todo always?) singletons
 */

// not using INTERFACE macro to avoid needless & wrong constructors
class IComponent : public IInterface 
{
   VERSION cVersion;          // *implementation* version
   IContainer& myContainer;   // Reference to the managing container of this component
protected:

   // not using the INTERFACE constructors, pass on IDL
   IComponent( IContainer& container, VERSION v, IComponent*& instance ) 
      throw() INITSECTION;
      
	// virtual ~IComponent() throw() {}

public:
	virtual const buffer getCDL() const = 0;
   inline IContainer& getContainer() /* const? */ { return myContainer; }

// Life-cycle management, only accesible to designated manager class (friend, impl)
private:

   // static const char* const cXML; // XML for IComponent itself, todo

	/// The creation function, implemented in this class
	// static IComponent* create( myos_idl_t idl, void* memory ) STARTSECTION;

	// add authenticate( const char* const challenge ) const ?

    /**
     * Initializes this component
     *
     * Initialization involves acquiring the required and optional resources this
     * component needs to function. This function is called by the component
     * manager (@todo Container ??)
     *
     * @param context   -   The repository where the component can lookup/register its
     *                      interfaces
     * @param params    -   Initialization values
     *
     * @return
     *  <br> E_MYOS_OK iff initialization succeeded
     *  @todo some error code when failed
     */
    virtual myos_result_t initialize( IContext& context, NVPAIR params[] ) INITSECTION = 0;

    /**
     *  Deinitializes this component
     *
     *  Deinitialization is separate from destruction, because sometimes you
     *  want to take a component out of the system but pass its state on to
     *  a replacement component before it is destroyed
     *
     *  @param context   -   The repository where the component registered its
     *                       interfaces
     *  @return @todo
     */
    virtual myos_result_t deinitialize( IContext& context ) = 0;

    /**
     *  Destroys this component: releases resources
     *
     *  After this call, no other calls will be made on the component
     *
     *  @return E_MYOS_SUCCESS
     */
   inline myos_result_t destroy();

public:
   /**
    * Queries the component for a specific interface
    * @param requestor     : Component asking for the interface
    * @param uuid          : UUID of the required interface
    * @param result[out]   : if found, points to the required interface on return
    *
    * @return E_MYOS_OK iff found
    *
    * @since MyOS 2.0
    */
   virtual myos_result_t queryInterface(
      IComponent& requestor,
      const UUID& uuid,
      IInterface*& result
   ) /* const no: result pointer not const */ = 0;

   /**
    * Queries the component for a specific interface
    * @param requestor     : Component asking for the interface
    * @param name          : name of the required interface
    * @param version       : implementation version to find, can be ANY()
    * @param result[out]   : if found, points to the required interface on return
    *
    * @return E_MYOS_OK iff found
    *
    * @since MyOS 2.0
    */
   virtual myos_result_t queryInterface(
      IComponent& requestor,
      const char* const name,
      VERSION version,
      IInterface*& result
   ) /* const */ = 0;


	// @see IInterface::get
	virtual myos_result_t get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const;

	// @see IInterface::put
	virtual myos_result_t put( const char* const call, NVPAIR parameters[], IO::IStream& input ) {
      if (strncmp(call,"destroy",7)==0) {  // @see IComponent.xml
         this->destroy();
         return E_MYOS_SUCCESS;
      } else return E_MYOS_E_FAIL;
	}

   /** Provides access to the container's context */
   inline IContext& getContext() const;
   
   /** @see IInterface */
   virtual const buffer getIDL() const { return buffer( IComponent::iXML ); }

   /** @see IInterface, TODO */   
   virtual const UUID& getUUID() const;
   virtual const myos_name_t getName() const { 
      return myos_name_t( "IComponent" ); 
   }

   // TODO
   // void* allocate( size_t ) { return 0; }

private:
   static const char* const iXML;   
};

}}	// namespaces

#ifndef ICONTAINER_H
#include "IContainer.h"

// inline functions
namespace MyOS { namespace Core {

   inline IContext& IComponent::getContext() const {
      return myContainer.getContext();
   }

}}
#endif

#endif
