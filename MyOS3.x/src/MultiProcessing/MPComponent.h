#ifndef MPComponent_H
#define MPComponent_H

#include "Core/IComponent.h"

#include "MultiProcessing/IMultiProcessingImpl.h"

namespace MyOS
{
namespace MultiProcessing
{

using MyOS::Core::IComponent;

class MPComponent : public IComponent
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;     // note: uses GDT directly, not entirely clean
    IPhysicalMemory* iPhysicalMemory;
    IMultiThreading* iMultiThreading;

public:
   inline static const char* const ID() { return "ba2161d8-487c-426d-8465-64e0f722ce40"; }

    MPComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

private:
    static MPComponent* instance;
public:
    inline static MPComponent& getInstance()
    {
        return *instance;
    }

private:

    friend class IMultiProcessingImpl; // access interfaces
    IMultiProcessingImpl impl1;

};

}
} // namespaces


#endif
