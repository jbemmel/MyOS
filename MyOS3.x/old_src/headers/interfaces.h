/*! \mainpage MyOS 3.x Main Index Page
 *
 * \section intro_sec Summary
 * 
 * MyOS design key aspects:
 * - everything has an Core::IInterface
 * - multi-thread safe by construction (not by locking or disabling interrupts)
 * - modular, OS is a container of components that each provide interfaces
 * - locality of reference by associating a memory pool with individual components (when needed)
 *   and threads
 * - efficient and readable code by using exceptions (avoiding branching on error codes)
 *   + also avoiding boolean flags as parameters, using 2 methods instead
 *
 */

#ifndef INTERFACE_FWD_DECLS_H
#define INTERFACE_FWD_DECLS_H

/*
 * Forward declarations of all interfaces
 */
namespace MyOS
{
namespace MM
{
    class IVirtualMemory;
    class IPhysicalMemory;
}

namespace InterruptHandling { class IInterruptHandling; }

namespace MultiThreading
{
    class IMultiThreading;
}

namespace MultiProcessing
{
    class Process;
}

namespace Timer
{
    class ITimerFacility;
}

namespace Networking
{
  namespace IP
  {
  class IIPLayer;
  }
}

namespace Drivers {
    class IDriverManager;
}

namespace Context {
    class IContext;
}

namespace IO {
    class IStream;
    class OStream;
    class IOStream;
}

} // MyOS

#endif
