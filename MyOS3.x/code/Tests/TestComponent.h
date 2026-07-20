#ifndef TestComponent_H
#define TestComponent_H

#include "IComponent.h"
#include "ITestRunnerImpl.h"

namespace MyOS { namespace Tests {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MM::IVirtualMemory;
using MultiThreading::IMultiThreading;
using MyOS::MultiProcessing::IMultiProcessing;
using MultiProcessing::Process;

using Devices::Display::IDisplay;



class TestComponent : public IComponent
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;
    IMultiThreading* iMultiThreading;
    IMultiProcessing* iMultiProccessing; 
    IDisplay* iDisplay;

public:
    inline static const char* const ID()
    {
        return "3af4da31-43d2-4724-a372-6832b5241725";
    }

    TestComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

private:
    static TestComponent* instance;
public:
    inline static TestComponent& getInstance()
    {
        return *instance;
    }

    IDisplay& getDisplay()
    {
        return *iDisplay;
    }
    IMultiThreading& getMT()
    {
        return *iMultiThreading;
    }
    
    IMultiProcessing& getMP()
    {
        return *iMultiProccessing;        
    }

private:

    friend class ITestRunnerImpl; // access interfaces
    ITestRunnerImpl impl1;

};

}
} // namespaces


#endif
