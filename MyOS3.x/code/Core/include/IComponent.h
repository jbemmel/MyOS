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

namespace MyOS
{

namespace Context
{
class IContext;
}

namespace Core
{

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

class EXPORT IComponent
{
    VERSION cVersion; //< *implementation* version
    IContainer& myContainer; //< Reference to the managing container of this component
    const char* const uuid; //< The UUID of this component

protected:
    IComponent(IContainer& container, VERSION v, const char* const uuid,
      IComponent*& instance) throw() INITSECTION;

    // virtual ~IComponent() throw() {}

public:
    inline IContainer& getContainer() /* const? */
    {
        return myContainer;
    }

    // Life-cycle management, only accesible to designated manager class (friend, impl)
private:

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
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION = 0;

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
    virtual myos_result_t deinitialize(Context::IContext& context) = 0;

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
     *
     * @todo version ?
     */
    virtual myos_result_t
            queryInterface(IComponent& requestor, const uuid_t& uuid,
                    IInterface*& result) /* const no: result pointer not const */= 0;

    /** Provides access to the container's context */
    inline Context::IContext& getContext() const;
};

}
} // namespaces

#ifndef ICONTAINER_H
#include "IContainer.h"

// inline functions
namespace MyOS
{
namespace Core
{

inline Context::IContext& IComponent::getContext() const
{
    return myContainer.getContext();
}

inline myos_result_t IComponent::destroy()
{
    return getContainer().destroyComponent(*this);
}

}
}
#endif

#endif
