/*! \mainpage MyOS 3.x Main Index Page
 *
 * \section intro_sec Introduction
 *
 * Even since my studies in Computer Science I have been fascinated by and
 * passionate about the topic of Operating Systems (OS) design. The OS is the 
 * fundamental piece of software that brings a computer to life, the key element
 * that makes it usable for its users. Computers, in turn, have become an 
 * essential element in our daily lives. For this reason, understanding and 
 * building one's own OS is exciting and fun - I would recommend 
 * it to anyone aspiring to truely learn and understand the essence of computing. 
 * MyOS was built as an expression of self, consider it as a modern work of art,
 * the implementation of a dream.
 *
 * Of course, many pieces of software exist today which are designed to act as 
 * your 'OS'. But that's their OS, it's not "MyOS". I guess you could say that - 
 * like many programmers - I suffer from NIH-syndrome, but it's more than that.
 * Developing an OS involves many design decisions, and these depend on the 
 * motivation(s) for creating the OS. Commercial OS software is designed to make
 * money; there is a reason that newer versions of Microsoft Windows require
 * increasingly powerful hardware, despite the fact that the basic functions have
 * not changed. Linux and Unix in general have evolved to cover an impressive amount
 * and variety of platforms and architectures, now even including mobile devices 
 * with Android. However, this generality comes at a price: Linux is not the most
 * efficient OS one could build. 
 *
 * This is the goal I set for MyOS: to be a highly efficient, modular, purpose-built
 * OS for networked servers. 
 * - implemented in C++ [ have considered designing my own language / compiler, 
 *   but that's another lifetime project and gains would be marginal. "D" is another
 *   alternative I've played with ]
 * - for modern X86 CPUs (Pentium and up) with 4MB or more memory
 *
 * The first use case I'm targeting, is to make a dedicated HTTP streaming server
 * for delivering videos from a filesystem (local or loaded in memory from a 
 * remote server)
 *
 * MyOS design key aspects:
 * - strong encapsulation/abstraction with only few key concepts, but - unlike Unix -
 *   abstraction does not come at the cost of reduced performance
 * - multi-thread safe by construction (wait-free atomic operations, not by 
 *   locking or disabling interrupts)
 * - modular, OS is a container of active components / services that each provide interfaces
 * - network enabled: TCP/IP, most local interfaces can also be invoked remotely over HTTP
 * - locality of reference by associating a memory pool with individual components (when needed)
 *   and threads, for improved caching
 * - efficient and readable code by using exceptions (avoiding branching on error codes)
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
