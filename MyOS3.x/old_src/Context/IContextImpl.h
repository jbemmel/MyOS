#ifndef IContextImpl_H
#define IContextImpl_H

#include "Context/IContext.h"

#include "MM/IVirtualMemory.h"
#include "MM/Virtual/ByteAllocator.h"

namespace MyOS {
namespace Context {

class IDirectory;

// For RepositoryComponent
using MM::IVirtualMemory;

class IContextImpl : public IContext {
public:
	IContextImpl(MyOS::Core::IComponent& c) INITSECTION;
	
	inline ~IContextImpl() throw() {}

	bool init(IContext& context, NVPAIR params[]) INITSECTION;
	void deinit(IContext& context);

	virtual myos_result_t add(IInterface& intf, myos_name_t* altname = 0);
	virtual myos_result_t remove(IInterface& intf);
	virtual IInterface& lookup( const myos_name_t& name ) const 
	        throw (InterfaceNotFoundException);
	virtual myos_result_t enumerate(IO::OStream& out) const;
	virtual myos_result_t copy(IContext& to) const;

private:

	void bootstrap() INITSECTION;

	// each context is backed up by a directory
	// This must be dynamically created, to prevent early alloc calls
	IDirectory *dir;
};

}
} // namespaces
#endif
