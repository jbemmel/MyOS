#ifndef IMultiProcessing_H
#define IMultiProcessing_H

#include "IInterface.h"
#include "IMultiThreading.h"

using MyOS::Core::IInterface;

namespace MyOS {
namespace MultiProcessing {


class Process;

using MM::OutOfMemoryException;
using MultiThreading::IRunnable;

/**
 * Interface for creating new processes
 */
INTERFACE( IMultiProcessing, "2c902461-c339-4008-ad73-84732a7c9f70" )

    /**
     * Creates a new process (with a new main thread) & starts it
     * 
     * @param runnable   - application provided callback
     * @param params     - optional parameters for IRunnable#run
     * @param paramCount - number of parameters
     * @param rpl        - requested privilege level, <= current thread
     * @param allocOrder - log2 of max allocatable memory (in 4K pages), 0 = 1 page
     * 
     * @throw OutOfMemoryException
     */
    virtual Process& createProcess( IRunnable& runnable, 
            NVPAIR params[], size_t paramCount, MM::EDPL rpl = MM::E_DPL0, u32 allocOrder=0 )
        throw (MM::OutOfMemoryException) = 0;

};

}
} // namespaces
#endif
