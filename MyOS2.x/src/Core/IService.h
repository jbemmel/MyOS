#ifndef ISERVICE_H
#define ISERVICE_H

#include "defs.h"                // STARTSECTION
#include "IComponent.h"

namespace MyOS { namespace Core {

class IContainer;

/**
 * A service is a component that can be attached to selected target objects. It
 * listens for objects of the appropriate type to be created, and depending on 
 * configuration attaches a session to each such object
 *
 * A typical example is a DHCP client service, that can be started on IP 
 * endpoints to request and update the IP address from a DHCP server
 * 
 * For an object to be 'servicable', its creator interface should implement the 
 * 'registerService' method (declare <servicePoint/> in XML).
 * A service will register itself with one or more creator components upon
 * initialization.
 * 
 * IDEA: Another way to implement this, is to replace the original interface
 * 		 in the repository with a wrapper that calls the original one but 
 *       attaches service instances as well!
 */

class IService : public IComponent
{
	IService* nextInChain;
protected:

   IService( IContainer& container, VERSION v, IService*& instance ) 
      throw() INITSECTION;
      
	// virtual ~IComponent() throw() {}

public:
	inline IService* getNextInChain() const   { return nextInChain; }
	inline void setNextInChain( IService* s ) { nextInChain = s; }

// Life-cycle management, only accesible to designated manager class (friend, impl)
private:

    /**
     * Initializes this service
     *
     * Initialization involves acquiring the required and optional resources this
     * component needs to function. This function is called by the service
     * manager
     *
     * @param context   -   The repository where the service can lookup/register its
     *                      interfaces
     * @param params    -   Initialization values
     *
     * @return
     *  <br> E_MYOS_OK iff initialization succeeded
     *  @todo some error code when failed
     */
    virtual myos_result_t initialize( IContext& context, NVPAIR params[] ) INITSECTION = 0;

    /**
     *  Deinitializes this service
     *
     *  Deinitialization is separate from destruction, because sometimes you
     *  want to take a service out of the system but pass its state on to
     *  a replacement service before it is destroyed
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

	// @see IInterface::get
	// virtual myos_result_t get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const;
   
	/** @see IInterface */
	virtual const buffer getIDL() const { return buffer( IService::iXML ); }

	/** @see IInterface, TODO */   
	virtual const UUID& getUUID() const;
	virtual const myos_name_t getName() const { 
		return myos_name_t( "IService" ); 
	}

	// Called by the servicePoint with which this service is registered
	// TODO: remodel servicePoint parameter? IInterface? other interface ?
	virtual bool onServicePointCreated( void* servicePoint ) = 0;

private:

	// @see IComponent
	virtual myos_result_t queryInterface(
		IComponent& requestor,
		const UUID& uuid,
		IInterface*& result
	) { return E_MYOS_E_NOTIMPL; }

	// @see IComponent
	virtual myos_result_t queryInterface(
      IComponent& requestor,
      const char* const name,
      VERSION version,
      IInterface*& result
	) { return E_MYOS_E_NOTIMPL; }

	static const char* const iXML;   
};

}}	// namespaces

#endif
