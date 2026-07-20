/**
 * Some details about the threading implementation
 *
 * - Threads could be reused by calling 'init' again after they have
 *   succesfully exitted (TODO: Fix that)
 */

#ifndef THREAD_H
#define THREAD_H

#include "Atomic/atomic32.h"
#include "Atomic/Queue2.h"
#include "IThread.h"
#include "signal.h"

// pretty tight coupling with MM classes...
#include "MM/ByteAllocator.h"
#include "ITiming.h"
#include "CPU/Processor.h"

// #include "Init/MyOSCoreContainerDefs.h"
//#ifdef MP_IN_CORE
namespace MyOS
{
namespace MultiProcessing
{
class Process;
}
}
//#endif

namespace MyOS
{
namespace MultiThreading
{

using Timer::ITimer;

class IRunnable;

class Thread: public Timer::ITimerTarget, public IThread
{
    // TimerEvent + ITimerTarget
    ITimer &timer;
    virtual void onTimer(ITimer&) throw ();

    friend class ThreadManager;

    // used for initial Thread
public:
    // temporary, for SMP testing
    inline void* operator new(size_t, void* m)
    {
        return m;
    }
    Thread() INITSECTION;
private:

    // Makes this thread the current one, called by ThreadManager
    void select(u32 quantumUs) REGPARM(2) NOINLINE;

    // datamembers
    friend class SchedulingPolicy;
    volatile enum E_THREADSTATE
    {
        E_CREATED = 1, E_READY = 2,
        // E_SELECTED    = 3,
        E_RUNNING = 4,

        //E_SLEEPY     = 5, //< i.e. usSleep() called
        E_SLEEPING = 6, //< i.e. waiting until timer fires

        E_SUSPENDED_ON_SIGNAL = 7,
        E_SUSPENDED = 8, //< i.e. waiting for wakeup()
        E_ENDING = 9,
        E_ENDED = 10,
        E_DESTRUCTED = 11
    } state;

    friend class DefaultPolicy; // XX TODO redesign
    friend class ConcurrentTM;
    void* policydata;
    Thread* next; // for making linked lists

    u32* esp; // Ring0 ESP register, rest of state is saved on stack
    //#ifdef MP_IN_CORE
    friend class MultiProcessing::Process;

    // @todo homeContext needed?
    MultiProcessing::Process *homeContext, *visitedContext;
    //#endif
    u32 sleeptime; // in us
    int exitcode;
    u32 priority; // Larger value is higher priority

    // Queue of threads to be deleted, done by idle
    static Queue deleteQueue;
    struct ZombieThread: public Queue::Item
    {
        Thread &ended;

        inline ZombieThread(Thread &t) throw () :
            ended(t)
        {
        }
    };

    // each thread gets its own allocator, for super non-blocked operation!
    MM::Allocator allocator;
    u32 stackorder; // size of stack of this thread, in 2^n, currently always 0

    atomic32 locks; // Lock counter, to prevent deletion with pending asyncs

    /**
     * State used for synchronization, @todo finish
     *
     * use 'next' for linked list of threads synchronizing on 'syncOn'
     */
    Thread* syncOn; // currentThread in synchronize, thread this thread is syncing on

    u8* tlsMemory; //< Memory to support thread-local storage

    // Each thread in a C++ program has access to a __cxa_eh_globals object.
    struct __cxa_eh_globals
    {
        void /* __cxa_exception */*caughtExceptions;
        unsigned int uncaughtExceptions;

        inline __cxa_eh_globals() throw () :
            caughtExceptions(0), uncaughtExceptions(0)
        {
        }

    } cxa_eh_globals;

    static void startThread(Thread& _this, IRunnable& runnable, NVPAIR ps[]) REGPARM(3) NORETURN USERSECTION;

    static void processSignals() SIGNAL_SAFE NORETURN;

    void processSignals2(u32* esp) NOINLINE SIGNAL_SAFE;

    void *signalReturnAddress;
    Queue signalQueue; //< Queue of PendingSignal
    IRunnable &runnable; //< Callback, kept for signal callbacks

    struct PendingSignal: public Queue::Item
    {
        u32 code;
        void* data;

        inline PendingSignal(u32 s, void* d) throw () :
            code(s), data(d)
        {
        }
    };

public:
    // private, but called from static function
    void exitThread() NORETURN;

    // for debugging
    inline int getState() const
    {
        return state;
    }

    /**
     * @see IThread, but this one gives access to all of 'Thread' members
     */
    inline static Thread& currentThread() CONSTFUNC
    {
        return *(Thread*) (((u32) Processor::ESP() | (_4KB - 1)) + 1
                - sizeof(Thread));
    }

    inline void* getTLSVariable(u32 index) const
    {
        ASSERTION( index < _4KB, E_ERROR );
        return &tlsMemory[index];
    }

    void* initTLSVariable(u32 index, u32 size, void* value);

    inline void* getEHGlobals()
    {
        return &cxa_eh_globals;
    }

    // Needs special care when allocating memory
    void* operator new(size_t);

    void operator delete(void*);

    Thread(IRunnable& runnable, NVPAIR params[], size_t paramCount,
            MM::EDPL rpl, u32 allocOrder, bool start);

    /* virtual */
    ~Thread() throw ();

    // Used to (re)initialize stack
    void init(IRunnable& runnable, NVPAIR params[], size_t paramCount,
            MM::EDPL rpl);

    // Used by Process
    void start();

    // IThread methods
    virtual MultiProcessing::Process& getProcess() const
    {
        return *visitedContext;
    }

    virtual bool isRunning() const
    {
        return state == E_RUNNING;
    }
    virtual bool isSleeping() const
    {
        return state == E_SLEEPING;
    }
    virtual bool usSleep(u32 us, u32 usAccuracy = 0);
    virtual bool wakeup();
    virtual void suspend();
    virtual void exit(int exitcode) NORETURN;
    virtual void yield(IThread* to);
    virtual void signal(u32 code, void* data);

    // perhaps virtual too
    inline bool isReady() const
    {
        return state == E_READY;
    }

    inline u32 getPriority() const { return priority; }

    inline void addPending()
    {
        ++locks;
    }

    // called by idle thread
    inline void asyncDone()
    {
        if ((--locks) <= 0)
        { // it may fall below 0
            delete this;
        }
    }

    // Use the allocator, used by global 'new' operator in IThread.h
    virtual void* allocate(size_t bytes) throw (MM::OutOfMemoryException);
    virtual void free(void* mem);

    // Used by TSS
    inline u32* getESP() const
    {
        return esp;
    }

    // Used for exception handling
    inline void* allocateNoThrow(size_t bytes) throw ()
    {
        return allocator.allocateAutoSizeNoThrow(bytes);
    }

    inline bool canThrow() const
    {
        return state == E_RUNNING || state == E_ENDED;
    }

    // used for exception handling, public linked list of frames
    void* exception_fc;
    u8 emergencyMem[52]; // == sizeof(eh) + OutOfMemoryEx

    /**
     * Called by MyOSMain after initialization is done
     *
     * @param idleQuantumMs - quantum (ms) for the idle thread, 0 means none
     */
    static void becomeIdle(unsigned idleQuantumMs) NORETURN;

    // debugging
    inline void showThreadMem() const
    {
        allocator.print();
    }
};

}
} // namespace

#endif
