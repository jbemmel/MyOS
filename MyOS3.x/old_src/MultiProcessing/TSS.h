/*
	TSS structure (from "Alles over PC hardware", p 107)
*/

#ifndef TSS_H
#define TSS_H

#include "defs.h"
#include "MM/Virtual/Selector.h"
#include "MM/Virtual/FarPointer.h"

namespace MyOS {
 
namespace MultiThreading { class Thread; }  
 
namespace MultiProcessing {

using MM::Selector;
using MM::FarPointer;
using MM::linadr_t;
using MultiThreading::Thread;

class Process;

/**
 * Represents a Task State Segment, which is used to load/store processor state during task switches.
 * 
 * This was designed to give each process its own TSS. However, it is possible to reduce this to 1 TSS
 * instance per CPU   
 */
class TSS
{
   friend class Process;
   TSS();
   void initOnce( ) INITSECTION;  // called for Init process 0 

   // separate, cannot be done in TSS()
   // Called just before process is started, 'main' must be initialized!
   void initialize( linadr_t pbt, Thread& main );

public:
    
   inline static Selector getTSSSelector() {
       return tssPointer.selector();
   }
    
   inline static void taskSwitch()
   {
       tssPointer.jmp();
   }
   
private:
    static FarPointer tssPointer;
    
	Selector TSS_backlink;					            // selector for previous TSS

	/***
	 * This part is never written by the processor, only stored
	 */
	struct // Separate stacks for privilege levels 0..2
	{
		void *ESP;
		Selector SS;
	} PACKED stacks[3];
	u32 CR3;		  // physical address of PBT, plus paging bits (never stored)
	
	/***
	 * These are stored when switching "away", and loaded when swapping back in
	 */
	void *EIP;    // Start of execution
	u32 EFLAGS;
	u32 EAX;      // parameter ThreadStart( _main )
	u32 ECX;
	u32 EDX;
	u32 EBX;
	void *ESP;
	u32 EBP;
	u32 ESI;
	u32 EDI;

	// segment selectors, their constructors clear them
	Selector ES;
	Selector CS;
	Selector SS;
	Selector DS;
	Selector FS;
	Selector GS;
	Selector LDT;

	// and finally
	int TRAP 	   : 1;	// TRAP bit: 1 -> DEBUG Exception
	int reserved 	: 15;
	u16 IObase;	         // 16 bit offset from start of TSS, beginning of IOmap + end of INT redirection map

	u32 iomap[990];  // to make TSS 64-bit aligned (==cache line), and to allow 38*32=0x7BC0 ports in V86 mode
	                 // (needed since VGA registers are at 0x3c0 for instance)
	                 // Diamond Stealth S220 even accesses port 0x6571...
} PACKED;

}}  // namespace

#endif

