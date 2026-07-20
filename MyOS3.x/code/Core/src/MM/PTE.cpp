#include "PTE.h"
#include "PageDirectory.h"		// BASEADDRESS
#include "OStream.h"
#include "IH/LocalAPIC.h"   // SMP
#include "MP/PerCPU.h"

#include "debug.h"

namespace MyOS {
namespace MM {

// Calculates the linear address mapped by this PTE
inline linadr_t PTE::getLinearAddress() const
{
	return (linadr_t) (((u32)this - (u32)PageDirectory::BASEADDRESS) << 10);
}

s32
PTE::list( IO::OStream &o, u32 rangecount ) const
{
    u32 start = o.getMarker();
    if (isMapped())
		o.printf(
            "\n<MAPPING lin=\"%x\" phys=\"%x\" atts=\"%x\" pages=\"%d\"/>",
		    this->getLinearAddress(), getMapping(), getAttributes(), rangecount );
	else
		o.printf(
            "\n<UNMAPPED lin=\"%x\" pages=\"%d\"/>",
            this->getLinearAddress(), rangecount );
	return o.getMarker() - start;
}

physadr_t targetPage ASMNAME("targetPage");

// static
void PTE::invalidateAll()
{
    // @todo refactor
    int r;
    ASMVOLATILE(
       "movl %%cr3, %0 \t\n"
       "movl %0, %%cr3"
       : "=r"(r) : : "memory"
    );

    // In case there are other CPUs running, notify them
    cpu_id_t others = MultiProcessing::getOtherCPUs();
    if ( others ) {
        targetPage = -1;
        InterruptHandling::LocalAPIC::sendIPI( others, InterruptHandling::E_IPI_FLUSHTLB );
    }
}

void PTE::invalidate()
{
    // @todo refactor
    physadr_t physadr = ((u32) this) << 10;
    ASMVOLATILE( "invlpg (%0)" :  : "r"(physadr) );

    // In case there are other CPUs running, notify them
    cpu_id_t others = MultiProcessing::getOtherCPUs();
    if ( others ) {
        // optimize: check address space, only bother CPUs running same CR3/process?
        // BUT: shared kernel space
        //MultiProcessing::Process *cur = MultiProcessing::Process::getCurrent();
        //for ( u32 cpu=0; cpu<MAX_CPUS; ++cpu ) {
        //    if ( MultiProcessing::cpuState[cpu].currentProcess )
        //}

        targetPage = ((u32) this) << 10;
        // DPRINTK( "\nSending TLB flush IPI for %x", targetPage );
        InterruptHandling::LocalAPIC::sendIPI( others, InterruptHandling::E_IPI_FLUSHTLB );
    }
}

/**
 * TODO If needed, can make all memory references relative to %ss instead of implicit %ds
 */
ASMVOLATILE (
        ".globl PTE_flush_TLB_IPI   \n"
        ".align 16                  \n"
        "PTE_flush_TLB_IPI:         \n"
        "pushl %eax                 \n"
        // "incl %ss:intcount               \n" // Keep count
        "movl apic, %eax            \n"
        "movl $0, 4*44(%eax)        \n"   // send EOI to local APIC
        "movl targetPage, %eax      \n"
        "cmpl $-1, %eax             \n"
        "je .1                      \n"
        "invlpg (%eax)              \n"
        "popl %eax                  \n"
        "iretl                      \n"
        ".1:             \n"
        "movl %cr3, %eax \n"
        "movl %eax, %cr3 \n"    // flush all TLB entries by reloading CR3
        "popl %eax       \n"
        "iretl           \n"
);

}}  // namespace
