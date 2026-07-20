/*
 * ConcurrentTM.cpp
 *
 *  Created on: 13-okt-2008
 *      Author: Jeroen
 */

#include "ConcurrentTM.h"
#include "MTComponent.h"
#include "MP/PerCPU.h"

namespace MyOS {
namespace MultiThreading {

void
ConcurrentTM::safe_selectNext( cpu_id_t forCPU )
{
    // Could delegate to an interface implementation
    Thread* target;
    MultiProcessing::PerCPU &cpu = MultiProcessing::cpuState[forCPU];
    u32 curPrio = cpu.active->getPriority();
    if ( cpu.highestPriority > curPrio ) {
        target = cpu.readyQueue[ cpu.highestPriority ];
    } else if ( curPrio > cpu.active->next->getPriority() ) {   // next!=0, idle.next==&idle
        target = cpu.readyQueue[ curPrio ];
    } else {
        target = cpu.active->next;
    }

    cpu.active = target;

    // Could also collect a bitmask of multiple selectNext calls, and broadcast the IPI here
    InterruptHandling::LocalAPIC::sendIPI( forCPU, InterruptHandling::E_IPI_THREAD_SELECT );
}

ASMVOLATILE (
    ".globl IPI_ThreadSelect        \n"
    ".align 16                      \n"
    "IPI_ThreadSelect:              \n"
    "pushal                         \n"
    // "incl %ss:intcount               \n" // Keep count
    "movl %ss:apic, %eax            \n"
    "movl $0, 4*44(%eax)            \n" // send EOI to local APIC
    "call THREAD_SWITCH            \n"
    "popal                          \n"
    "iretl                          \n"
);

// static
void
ConcurrentTM::thread_switch( )  // ASMNAME( "THREAD_SWITCH")
{
    MultiProcessing::PerCPU &cpu = MultiProcessing::cpuState[ MultiProcessing::getCurrentCPU() ];
    cpu.active->select( 1000 ); // @todo quantum inside Thread itself!
}

/**
 * Below is all the plumbing needed to make this class concurrent
 */

// static
volatile int ConcurrentTM::lock = 0;
AtomicQueue ConcurrentTM::pending;      // TODO initialize! or not static

struct AbstractActivation : public AtomicQueue::Item
{
    virtual void execute() = 0;
};

void ConcurrentTM::safe_processActivations() // SIGNAL_SAFE
{
    _UNWIND( lock );
        AbstractActivation *a = (AbstractActivation*) pending.dequeue();
        if (a) {
            a->execute();
        } else {
            _BREAK_UNWIND( lock );
        }
    _END_UNWIND( lock );
}

// static
void
ConcurrentTM::onThreadReady( Thread &t )
{
    struct onThreadReadyActivation : public AbstractActivation
    {
        Thread &thread;

        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(onThreadReadyActivation) );
        }

        inline onThreadReadyActivation( Thread &t ) throw() : thread(t) {}

        virtual void execute() {
            ConcurrentTM::safe_onThreadReady( thread );
            deallocate( this, sizeof(onThreadReadyActivation) );
        }
    };

    _LOCK( lock );
        safe_onThreadReady(t);
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
        onThreadReadyActivation *a = new onThreadReadyActivation(t);
        if (a) pending.enqueue( *a );
    _ENDLOCK( lock );
}

// static
void
ConcurrentTM::onCurThreadBlocked()
{
    struct onThreadBlockedActivation : public AbstractActivation
    {
        Thread &thread;
        cpu_id_t onCPU;

        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(onThreadBlockedActivation) );
        }

        inline onThreadBlockedActivation( Thread &t, cpu_id_t c ) throw()
            : thread( t ), onCPU( c ) {}

        virtual void execute() {
            ConcurrentTM::safe_onThreadBlocked( thread, onCPU );
            deallocate( this, sizeof(onThreadBlockedActivation) );
        }
    };

    Thread &cur = Thread::currentThread();
    cpu_id_t cpu = MultiProcessing::getCurrentCPU();
    _LOCK( lock );
        safe_onThreadBlocked( cur, cpu );
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
    onThreadBlockedActivation *a = new onThreadBlockedActivation(cur,cpu);
        if (a) pending.enqueue( *a );
    _ENDLOCK( lock );
}

// static
void
ConcurrentTM::selectNext()
{
    struct selectNextActivation : public AbstractActivation
    {
        cpu_id_t forCPU;

        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(selectNextActivation) );
        }

        inline selectNextActivation( cpu_id_t c ) throw() : forCPU(c) {}

        virtual void execute() {
            ConcurrentTM::safe_selectNext( forCPU );
            deallocate( this, sizeof(selectNextActivation) );
        }
    };

    cpu_id_t cpu = MultiProcessing::getCurrentCPU();
    _LOCK( lock );
        safe_selectNext(cpu);
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
        selectNextActivation *a = new selectNextActivation(cpu);
        if (a) pending.enqueue( *a );
    _ENDLOCK( lock );
}

}}
