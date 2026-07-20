#ifndef DisplayComponent_H
#define DisplayComponent_H

#include "Core/IComponent.h"

#include "Devices/Display/IDisplayImpl.h"


namespace MyOS { namespace Devices { namespace Display {

using MyOS::Core::IComponent;

class DisplayComponent : public IComponent {

public:
	inline static const char* const ID() { return "c706b2e4-5f59-11d6-946d-0010a708e02d"; }
	
    DisplayComponent( MyOS::Core::IContainer& container ) throw() INITSECTION;
    virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( Context::IContext& context );

	virtual myos_result_t queryInterface(
      IComponent& requestor,
      const uuid_t& uuid,
      IInterface*& result
    );

	inline IO::OStream& cout() { return impl1.cout(); }
	
private:
    static DisplayComponent* instance;
public: 
	inline static DisplayComponent& getInstance() { return *instance; }

private:    

   friend class IDisplayImpl;	// access interfaces
   IDisplayImpl impl1;


};

}
}} // namespaces



#endif
