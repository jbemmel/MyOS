#include "Processor.h"
#include "debug.h"

namespace MyOS
{

// Borrowed from Linux, eax == low, edx == hi
// Put these here and not in header -> code must call this class!
#define rdmsr(msr,eax,edx) \
   ASMVOLATILE( "rdmsr" : "=a" (eax), "=d" (edx) : "c"(msr) )

#define wrmsr(msr,eax,edx) \
   ASMVOLATILE( "wrmsr" : : "a" (eax), "d" (edx) , "c"(msr) )

// struct used with CPUID function 0x1, compatible with 'REGS' structure
struct CPUID
{
    // EAX: Stepping, model, family
    unsigned stepping : 4;
    unsigned model : 4;
    unsigned family : 4;
    unsigned _res1 : 4;
    unsigned ext_model : 4;  // only if family=0xF
    unsigned ext_family : 8; // only if family=0xF
    unsigned _res2 : 4;

    // EBX (AMD: brand, apic, clflush)
    u8 brandid;
    u8 clflush;
    u8 _res3;
    u8 apic_initid;

    // ECX entirely reserved (0)
    u32 ecx_res;

    // EDX: expand bits, see http://faydoc.tripod.com/cpu/cpuid.htm
    union
    {
        u32 features;
        struct
        {
            unsigned fpu_on_chip : 1;
            unsigned VME_supported : 1;
            unsigned DE_supported : 1; // debugging extensions
            unsigned PSE_supported : 1;
            unsigned TSC_supported : 1;
            unsigned MSR_supported : 1;
            unsigned PAE_supported : 1;
            unsigned MCE_supported : 1; // Machine Check Enable
            unsigned CX8_supported : 1; // CMPXCHG8B
            unsigned APIC_enabled : 1; // supported *and* enabled
            unsigned __res0__ : 2;
            unsigned MTRR_supported : 1;
            unsigned PGE_supported : 1;
            unsigned MCA_supported : 1; // Supports MGC_CAP MSR
            unsigned CMOV_supported : 1;
            unsigned __res1__ : 7;
            unsigned MMX_supported : 1;
            u8 __res2__;
        } PACKED;
    };
} PACKED;

// TODO: Could inline these, as they are typically only used during startup!

/* static, originally for AMD_K6 but should work in general   
 void Processor::writeMSR(u32 msr, u32 v1, u32 v2, REGS& regs)
 {
 ASMVOLATILE (
 //      "pushl %%ebx\n"
 //      "movl %%edi, %%ebx\n"   // needed for -fPIC
 "pushfl\n"
 "cli\n"
 "wbinvd\n"
 "wrmsr\n"
 "popfl\n"
 //      "movl %%ebx, %%edi\n"
 //      "popl %%ebx\n"
 : "=a" (regs.eax),
 //          "=D" (regs.ebx),
 "=b" (regs.ebx),
 "=c" (regs.ecx),
 "=d" (regs.edx)
 : "0" (v1),
 "3" (v2),
 "2" (msr)
 : "memory" );
 }
 */
/*
 void Processor::readMSR(E_MSR msr, REGS& regs)
 {
 ASMVOLATILE (
 //      "pushl %%ebx\n"
 //      "movl %%edi, %%ebx\n"   // needed for -fPIC
 "pushfl\n"
 "cli\n"
 "wbinvd\n"
 "xorl %%eax, %%eax\n"
 "xorl %%edx, %%edx\n"
 "rdmsr\n"
 "popfl\n"
 //      "movl %%ebx, %%edi\n"   // needed for -fPIC
 //      "popl %%ebx\n"
 : "=a" (regs.eax),
 //        "=D" (regs.ebx),
 "=b" (regs.ebx),
 "=c" (regs.ecx),
 "=d" (regs.edx)
 : "2" (msr)
 : "memory" );
 }   
 */

// Executes the CPUID instruction
void Processor::cpuId(E_CPUID_OPCODES opcode, REGS& result)
{
    ASMVOLATILE(
            //      "pushl %%ebx\n"         // needed for -fPIC
            "cpuid   \t\n"
            //      "movl %%ebx, %%edi\t\n" // needed for -fPIC
            //      "popl %%ebx\n"          // needed for -fPIC
            : "=a"(result.eax),
            //        "=D"(result.ebx), 
            "=b"(result.ebx),
            "=c"(result.ecx),
            "=d"(result.edx)
            : "0"( opcode )
            : "memory"
    );
}

Processor::E_CPUID_OPCODES Processor::getVendorId(char id[13]) // 12 bytes + '0'
{
    REGS result;
    cpuId(E_GetVendorID, result ); // EBX-EDX-ECX

    E_CPUID_OPCODES maxlevel = (E_CPUID_OPCODES) result.eax;
    *((u32*) &id[0]) = result.ebx;
    *((u32*) &id[4]) = result.edx;
    *((u32*) &id[8]) = result.ecx;
    id[12] = 0;

    // DPRINTK( "\nMax Supported StdLevel %x, Vendor Id=%s", maxlevel, id );
    return maxlevel;
}

// static 
bool Processor::enableLocalAPIC()
{
    // Could cache part of the CPUID calls instead of doing it each time...
    char vendor[16];
    getVendorId(vendor ); // for now, simple check on vendor first
    // Better would be to check on model, etc

    CPUID cpu;
    if (strncmp(vendor, "GenuineIntel", 12)==0)
    {
        // Pentium-S does not support rdmsr, check using cpuid
        // TODO: Merge checks for AMD and Intel

        // XX should check if CPUID supported...      
        cpuId(E_GetFeatures, * ((REGS*) &cpu));

        // first test if APIC already enabled, if yes return
        // Don't try rdmsr on MSR_IA32_APIC_BASE, some pentiums don't support it
        if (cpu.APIC_enabled)
        {
            DPRINTK("\nIntel CPU with APIC already enabled");
            return true;
        }
        else if (!cpu.MSR_supported)
        {
            DPRINTK("\nIntel CPU without MSR support, no APIC");
            return false;
        }
        else if (cpu.family==5&& cpu.model >=2)
        {
            DPRINTK("\nPentium P54C detected, TODO:more testing, no APIC for now");
            return false;
        }
    }
    else if (strncmp(vendor, "AuthenticAMD", 12)==0)
    {

        // see http://www.amd.com/us-en/assets/content_type/
        //     white_papers_and_tech_docs/20734.pdf p83 : AMD Athlon/Duron models 6 
        //     and higher contain an APIC too (see also linux:apic.c)

        // XX should check if CPUID supported...      
        cpuId(E_GetFeatures, * ((REGS*) &cpu));
        if ((cpu.family == 6&& cpu.model > 1) || cpu.family == 0xF)
        {
            // ok, continue below  
        }
        else
        {
            DPRINTK("\nAMD CPU without local APIC support detected");
            return false;
        }
    }
    else
    {
        DPRINTK("\nUnsupported CPU vendor, probably without local APIC");
        return false;
    }

    // this rdmsr crashes a AMD K6-II laptop and a Pentium-S 100Mhz machine
    u32 eax, edx;
    rdmsr( MSR_IA32_APIC_BASE, eax, edx )
;    if ((eax & 0x800)==0)
    {
        DPRINTK( "\nLocal APIC disabled in MSR, enabling..." );
        wrmsr( MSR_IA32_APIC_BASE, eax | 0x800, edx );
    }

    // ..and test if it worked..         
    cpuId( E_GetFeatures, * ((REGS*) &cpu) );
    if (cpu.APIC_enabled)
    {
        DPRINTK( "worked!" );
        return true;
    }
    else
    {
        DPRINTK( "failed :(" );
        return false;
    }
}

// see http://grafi.ii.pw.edu.pl/gbm/x86/cpuid.html for more help on this

bool
Processor::enableGlobalPages() // TODO: see http://www.ussg.iu.edu/hypermail/linux/kernel/9910.3/0336.html

{ // AMD K5 uses APIC bit to indicate PGE support...
    // XX should check if CPUID supported...      
    CPUID cpu;
    cpuId( Processor::E_GetFeatures, * ((REGS*) &cpu) );
    if (cpu.PGE_supported)
    {
        // DPRINTK( "\nEnabling CR4.PGE for global pages support..." );
        setCR4( CR4() | (1<<7) );
        return true;
    }
    else
    {
        // ASSERTIONv( sizeof(CPUID) == 32, E_CRITICAL, sizeof(CPUID) );
        DPRINTK( "\nAlas, PGE not supported by CPU (features=%x)", cpu.features );
        return false;
    }
}

} // namespace
