#ifndef DEBUG_H
#define DEBUG_H

// #include "Init/MyOSCoreContainerDefs.h"    // for IN_CORE macro's

// Always available...
//#ifdef Display_IN_CORE
#include "Display/DisplayComponent.h"

#ifdef MODULE
#include "module.h"
#define COUT MyOS::Module::display->cout()
#else
#define COUT MyOS::Devices::Display::DisplayComponent::getInstance().cout()
#endif
#define PRINTK COUT.printf
//#endif

#ifndef DEBUG

#ifndef DPRINTK
#ifdef __GNUC__
 #define DPRINTK(m,...)
 #define DPRINTP(m,...)
#else
 void DPRINTK( const char* fmt, ... );
 void DPRINTP( const char* fmt, ... );
#endif
#endif
#define FILELINE
#define NOT_IN_IRQ 

#define ASSERTION(a,s)
#define ASSERTIONv(a,s,v)
#define CHECKPOINT(c)
#define CHECKPOINTc(c,v)
#define BUG(b)

#else

#include "Debugging/Assertion.h"

//#ifdef Display_IN_CORE
#define DPRINTK COUT.printf
#define DPRINTN(N) COUT.print(N)
//#else
//#define DPRINTK(x,...)
//#define DPRINTN(N)
//#endif
#define CHECKPOINT(c) MyOS::Debugging::checkpoint(c)
#define CHECKPOINTc(c,v) MyOS::Debugging::checkpoint(c,Devices::Display::v)

//#include "Devices/Printer/LPTStream.h"
//#define DPRINTP(args...) do { MyOS::Devices::LPT::LPTStream __p; __p.printf(args); } while(0)

#include "CPU/Processor.h"
#define NOT_IN_IRQ ASSERTION( Processor::Processor::CS() == 0x8, E_CRITICAL )

// tricky to convert __LINE__ to string
#define LINESTR_2(line)   #line
#define LINESTR_1(line)   LINESTR_2(line)
#define LINESTR           LINESTR_1(__LINE__)
#define FILELINE          (__FILE__ ":" LINESTR)

#define ASSERTION(a,s) \
   do { if ((a)==0) MyOS::Debugging::assertionFailed(MyOS::Debugging::s,__FILE__ ": line " LINESTR "\n" #a,__FUNCTION__,0); } while (0)

#define ASSERTIONv(a,s,v) \
   do { if ((a)==0) MyOS::Debugging::assertionFailed(MyOS::Debugging::s,__FILE__ ": line " LINESTR "\n" #a,__FUNCTION__,(int)(v)); } while(0)

#define BUG(s) ASSERTION( !s, E_CRITICAL )

#endif   // defined(DEBUG)
#endif
