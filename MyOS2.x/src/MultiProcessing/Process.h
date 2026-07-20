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

namespace MyOS {

class NVPAIR;

namespace MM { class PBT; } 
namespace MultiThreading { class IRunnable; class Thread; } 
 
namespace MultiProcessing {

using namespace MultiThreading;
using MM::PBT;
using Exceptions::OutOfMemoryException;

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
 * 
 */
class Process /* : public IRunnable */
{
   static Process* currentProcess ASMNAME("curProcess"); 
 
   Process( /*Priority prio*/ ) INITSECTION; // used for Process 0 
public:
   static void initOnce() INITSECTION;

   inline static Process& getCurrent() { return *currentProcess; }

   /**
    * Constructor creating a new process/task
    *
    * @param optionflags : bitmask OR of @see E_PROCESS_OPTIONS
    */
	Process( Process& parent, u32 allocBlockOrder );

	// Page* allocatePrivatePages( unsigned nPages, /* EPAGEFLAGS */ unsigned flags, unsigned *PTE = 0 );
	// void freePrivatePages( unsigned nPages, Page start[] );
	// void unmapPrivatePages( unsigned nPages, Page lin_adr[] );
	bool start( IRunnable& main, NVPAIR params[], size_t paramCount );

	// gets
	inline Process* getParent() const 		      { return parent; }
	inline Process* getFirstSubprocess() const 	{ return subprocesses; }
	inline Process* getNext() const 		         { return next; }
	inline Thread&  getMainThread() 			      { return mainThread; }
	inline u32 getPID() const		               { return (u32) this; }

	// called by Thread
	void addThread( Thread& t );

   // called from ~Thread (in another context), deletes this process
   // if thread == main (or perhaps threadcount==0 is better??)
	void removeThread( Thread& t );

#ifdef STATISTICS
	static u32 getProcessesStarted() { return processesStarted; }
	static u32 getProcessesEnded()   { return processesEnded; }
#endif

protected:
   friend class IMultiProcessingImpl;
	void* operator new( size_t ) throw (OutOfMemoryException);

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

