/*
 * ConcurrentTM.h
 *
 *  Created on: 13-okt-2008
 *      Author: Jeroen
 */

#ifndef CONCURRENTTM_H_
#define CONCURRENTTM_H_

#include "CPU/Processor.h"
#include "Atomic/AtomicQueue.h"

namespace MyOS { namespace MultiThreading {

class Thread;
class Scheduler;

/**
 * A non-blocking concurrent Thread Manager
 */
class ConcurrentTM
{
    static volatile int lock;

    /**
     * FIFO queue of pending thread operations
     */
    static AtomicQueue pending;

    static Scheduler *scheduler;

public:
    //static inline void onNewThread( Thread &t ) {
    //    scheduler->onNewThread( t );    // not protected
    //}

    static void onThreadReady( Thread &t );
    static void onCurThreadBlocked();
    static void selectNext();

private:
    static void safe_processActivations();
    static void safe_onThreadReady( Thread &t );
    static void safe_onThreadBlocked( Thread &t, cpu_id_t onCPU );
    static void safe_selectNext( cpu_id_t forCPU );

    static void onThreadSelectIPI() ASMNAME( "IPI_ThreadSelect" );
    static void thread_switch( ) ASMNAME( "THREAD_SWITCH") /* SIGNAL_SAFE ?? */;
};

}}

#endif /* CONCURRENTTM_H_ */
