/**
	Process.h	- Process class

	In MyOS a process is more a passive container for threads and other
	resources than an active task. Currently, a process consists of a
	PageBaseTable(PBT), a linked list of all threads and subprocesses,
	and a reference to the parent process (if any).


	NOTES
	- PID: only 8 bits
		upper 12 bits are equal for all (0xFFC, kernel memory)
		lower 12 bits are all zero (page aligned)
*/

#ifndef PROCESS_H
#define PROCESS_H

#include "defs.h"    // INITSECTION
#include "TSS.h"     // TSS is contained within Process structure
#include "Exceptions/MyOSExceptions.h"

#include "Devices/Processor/Processor.h"

namespace MyOS {

class NVPAIR;

namespace MM { class PBT; } 
namespace MultiThreading { class IRunnable; class Thread; } 
 
namespace MultiProcessing {

using namespace MultiThreading;
using MM::PBT;
using MM::OutOfMemoryException;

/**
 *  Process - A collection of cooperating threads and resources
 *
 *  A process has 1 main thread
 *
 * @note Be careful with inheritance here, ASM code depends on offset of TSS member
 * 
 * Could derive Process from Thread ? -> A Process is a thread with a 
 *                                       separate address space...
 *   .. but TSS should be page-aligned ..
 * 
 * @todo IProcess interface needed!
 * 
 * Each process is responsible for managing its private address space (i.e. allocating / deallocating
 * in virtual memory range 0-0xFF800000
 * Note: Video mem is currently at first 1Mb, should be moved
 */
class Process /* : public IRunnable */
{
   static Process* currentProcess; 
 
   Process( /*Priority prio*/ ) INITSECTION; // used for Process 0 

public:
   static void initOnce() INITSECTION;

   inline static Process& getCurrent() CONSTFUNC { return *currentProcess; }

   /**
    * Constructor for creating a new process/task
    */
	Process( IRunnable& runnable, NVPAIR params[], size_t paramCount, MM::EDPL rpl, u32 allocOrder );

	// Page* allocatePrivatePages( unsigned nPages, /* EPAGEFLAGS */ unsigned flags, unsigned *PTE = 0 );
	// void freePrivatePages( unsigned nPages, Page start[] );
	// void unmapPrivatePages( unsigned nPages, Page lin_adr[] );
	
	// gets
	inline Process* getParent() const 		    { return parent; }
	inline Process* getFirstSubprocess() const 	{ return subprocesses; }
	inline Process* getNext() const 		    { return next; }
	inline Thread&  getMainThread() 			{ return mainThread; }
	inline u32 getPID() const		            { return (u32) this; }

	// called by Thread
	void addThread( Thread& t );

   // called from ~Thread (in another context), deletes this process
   // if thread == main (or perhaps threadcount==0 is better??)
	void removeThread( Thread& t );

	/**
	 * Makes this process the active process
	 */
	inline void	prepareSwitch( u32* threadESP )
	{
	    this->tss.ESP = threadESP;   // make sure we can return
	    
	    // GCC caller-save registers: EBX, EDI, ESI, EBP
	    this->tss.EBX = Processor::EBX();
	    this->tss.EDI = Processor::EDI();
	    this->tss.ESI = Processor::ESI();
	    this->tss.EBP = Processor::EBP();

        // MUST do this before jumping, stack gets swapped out and return address may be elsewhere
        currentProcess = this;
	}
	
#ifdef STATISTICS
	static u32 getProcessesStarted() { return processesStarted; }
	static u32 getProcessesEnded()   { return processesEnded; }
#endif

protected:
   friend class IMultiProcessingImpl;
   void* operator new( size_t ) throw (MM::OutOfMemoryException);

private:
	~Process();	// processes die when MainThread finishes
	void operator delete (void*);

	// needed to separate Initial Process from others
	void initialize( /*Priority prio*/ );

	// Should be within 1 page, aligned... OFFSET MUST MATCH ASM TASKSWITCH !
	TSS tss;
	Thread& mainThread;
	PBT* pbt;

	/// For linking processes that are subprocesses of the same parent together
	Process *next;
	Process *parent;
	Process *subprocesses;			  // start of a linked list of all subprocesses

	/// Linked list of all threads in this process
	// DRINGLIST<Thread> threads;

#ifdef STATISTICS
	static u32 processesStarted, processesEnded;
#endif

}; // sizeof(Process)==_4KB

}}  // namespace

#endif

