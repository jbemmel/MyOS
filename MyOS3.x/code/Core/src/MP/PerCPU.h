/*
 * PerCPU.h
 *
 *  Created on: 21-sep-2008
 *      Author: Jeroen
 */
#ifndef PERCPU_H_
#define PERCPU_H_

#include "IH/LocalAPIC.h"
#include "Atomic/atomic.h"

namespace MyOS {

namespace MultiThreading { class Thread; }

namespace MultiProcessing {

#define MAX_CPUs 4
#define MAX_PRIORITIES 10

class Process;
using MultiThreading::Thread;

extern struct PerCPU {
    Process * volatile currentProcess;

    Thread * volatile active;   //< The active or next-to-be-made-active thread

    Thread *readyQueue[ MAX_PRIORITIES ];
    u32 highestPriority;
} cpuState[ MAX_CPUs ];

extern volatile u32 runningCPUs;

static inline cpu_id_t getCurrentCPU() {
    return InterruptHandling::LocalAPIC::getID();
}

/**
 * Returns a bitmask of other running CPUs besides current
 */
static inline cpu_id_t getOtherCPUs() {
    // during boot, cannot read APIC ID
    return (runningCPUs!=1) ? runningCPUs & ~( 1<<getCurrentCPU() ) : 0;
}

static inline void setCPUIsOnline() {
    // @todo atomic_set_mask( 1<<getCurrentCPU(), runningCPUs );, invalid code generated(!)
    runningCPUs |= 1<<getCurrentCPU();
}


}}

#endif /* PERCPU_H_ */
