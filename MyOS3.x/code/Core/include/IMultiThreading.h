#ifndef IMultiThreading_H
#define IMultiThreading_H
#include "IInterface.h"

using MyOS::Core::IInterface;

#include "MyOSExceptions.h"
#include "NVPAIR.h"
#include "memtypes.h"

namespace MyOS
{
namespace MultiThreading
{

class IThread;

/**
 * Callback interface
 */
class IRunnable
{
public:
    /**
     * Callback function called when thread starts executing
     * 
     * @param params - Parameter array as passed in IThread#start, can be NULL
     */
    virtual int run( NVPAIR params[] ) = 0;
    
    /**
     * Optional signal handler, can be overridden by implementation
     */
    virtual void handleSignal( u32 code, void* data ) throw() = 0;
};

/**
 * Interface to create threads
 */
INTERFACE( IMultiThreading, "3db93a90-a7c1-41df-9029-4e3895f9fab2" )

    /**
     * Creates & starts a new thread
     * 
     * @param runnable   - application provided callback
     * @param params     - optional parameters for IRunnable#run
     * @param paramCount - number of parameters
     * @param rpl        - requested privilege level, <= current thread
     * @param allocOrder - log2 of max allocatable memory (in 4K pages), 0 = 1 page
     * @param start      - whether the thread should start running immediately, default true
     */
    virtual IThread& createThread( IRunnable& runnable, NVPAIR params[], size_t paramCount, 
            MM::EDPL rpl = MM::E_DPL0, u32 allocOrder=0, bool start = true ) 
        throw(MM::OutOfMemoryException) = 0;

};

}
} // namespaces
#endif
