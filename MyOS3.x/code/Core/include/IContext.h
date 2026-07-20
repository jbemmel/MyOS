#ifndef IContext_H
#define IContext_H

#include "IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS {
namespace Context {

namespace Exceptions {
    class InterfaceNotFoundException
    {
        const myos_name_t& name;

    public:
        inline InterfaceNotFoundException( const myos_name_t& n ) throw() : name(n) {}
        
        inline const myos_name_t& getInterfaceName() const { return name; }
    };
}

using Exceptions::InterfaceNotFoundException;

/**
 * Interface representing a context to register interface implementation instances
 * 
 * @todo Context factory, e.g. to create a context for registering all thread instances, process
 *   instances, devices, etc. Something like IDirectory, investigate relationship
 * 
 * (Currently IContext is implemented in terms of IDirectory
 */
INTERFACE( IContext, "bdf79a36-5871-11d6-858d-0010a708e02e" )

	virtual myos_result_t add(IInterface& intf, myos_name_t* altname=0) = 0;

	virtual myos_result_t remove(IInterface& intf) = 0;

	/**
	 * @param name - typically an UUID, but may be any name in general
	 * 
	 * @throw InterfaceNotFoundException - when interface is not found
	 */
	virtual IInterface& lookup( const myos_name_t& name ) const 
	    throw (InterfaceNotFoundException) = 0;

	virtual myos_result_t enumerate(IO::OStream& out) const = 0;

	virtual myos_result_t copy(IContext& to) const = 0;

};

}
} // namespaces
#endif
