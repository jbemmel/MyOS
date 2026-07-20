/**
 * 19/8/2004 - Found out that this does not work with GCC 3.4.1 (yet), reverting to 3.3.3
 */

#include "types.h"
#include "Processor/Processor.h"
#include "MultiThreading/IThread.h"

using MyOS::MultiThreading::IThread;
using MyOS::MultiThreading::Thread;
using MyOS::Processor;

/**
 * Attempt to take only the very minimal of GCC exception shit
 */
 
//#include "tconfig.h"
//#include "tsystem.h"
#include "unwind.h"
//#include "gthr.h"

// JvB: typeinfo for int
#include "typeinfo.h"

#include "debug.h"

namespace __cxxabiv1 {

// Prototypes, copied from unwind-cxx.h
extern "C" void *__cxa_allocate_exception(size_t thrown_size) throw();
extern "C" void __cxa_free_exception(void *thrown_exception) throw();
extern "C" void __cxa_throw (void *thrown_exception,
             std::type_info *tinfo,
            void (*dest) (void *))
     __attribute__((noreturn));
extern "C" void *__cxa_begin_catch (void *) throw();
extern "C" void __cxa_end_catch ();
extern "C" void __cxa_rethrow () __attribute__((noreturn));

extern "C" void __cxa_call_unexpected (void *ex) __attribute__((noreturn));

/*

// JvB: Copied from cxxabi.h

// type information for int, float etc
class __fundamental_type_info
  : public std::type_info
{
public:
  virtual ~__fundamental_type_info () {}
public:
  explicit __fundamental_type_info (const char *__n)
    : std::type_info (__n)
    { }
};

// common type information for simple pointers and pointers to member
class __pbase_type_info
  : public std::type_info
{
// abi defined member variables
public:
  unsigned int __flags; // qualification of the target object
  const std::type_info *__pointee;   // type of pointed to object

// abi defined member functions
public:
  virtual ~__pbase_type_info () {}
public:
  explicit __pbase_type_info (const char *__n,
                                int __quals,
                                const std::type_info *__type)
    : std::type_info (__n), __flags (__quals), __pointee (__type)
    { }

// implementation defined types
public:
  enum __masks {
    __const_mask = 0x1,
    __volatile_mask = 0x2,
    __restrict_mask = 0x4,
    __incomplete_mask = 0x8,
    __incomplete_class_mask = 0x10
  };

// implementation defined member functions
protected:
  //virtual bool __do_catch (const std::type_info *__thr_type,
  //                         void **__thr_obj,
  //                         unsigned __outer) const;
protected:
  //inline virtual bool __pointer_catch (const __pbase_type_info *__thr_type,
  //                                     void **__thr_obj,
  //                                     unsigned __outer) const;
};

// type information for simple pointers, used when catching by reference
class __pointer_type_info
  : public __pbase_type_info
{
// abi defined member functions
public:
  virtual ~__pointer_type_info () {}
public:
  explicit __pointer_type_info (const char *__n,
                                int __quals,
                                const std::type_info *__type)
    : __pbase_type_info (__n, __quals, __type)
    { }

// implementation defined member functions
protected:
  //virtual bool __is_pointer_p () const;

protected:
  //virtual bool __pointer_catch (const __pbase_type_info *__thr_type,
  //                              void **__thr_obj,
  //                              unsigned __outer) const;
};

*/

// type information for a class
class __class_type_info
  : public std::type_info
{


// abi defined member functions
public:
  virtual ~__class_type_info ();
public:
  explicit __class_type_info (const char *__n)
    : type_info (__n)
    { }
};

// This has special meaning to the compiler, and will cause it
// to emit the type_info structures for the fundamental types which are
// mandated to exist in the runtime.
//__fundamental_type_info::
//~__fundamental_type_info ()
//{}

//__pointer_type_info::
//~__pointer_type_info ()
//{}

__class_type_info::
~__class_type_info ()
{}

}

/* This structure is allocated on the stack of the target function.
   This must match the definition created in except.c:init_eh.  */
struct SjLj_Function_Context
{
  /* This is the chain through all registered contexts.  It is
     filled in by _Unwind_SjLj_Register.  */
  struct SjLj_Function_Context *prev;

  /* This is assigned in by the target function before every call
     to the index of the call site in the lsda.  It is assigned by
     the personality routine to the landing pad index.  */
  int call_site;

  /* This is how data is returned from the personality routine to
     the target function's handler.  */
  _Unwind_Word data[4];

  /* These are filled in once by the target function before any
     exceptions are expected to be handled.  */
  void* /*_Unwind_Personality_Fn*/ personality;

  // JvB: Language specific data area
  void *lsda;

  void *jbuf[];
};

struct _Unwind_Context
{
  struct SjLj_Function_Context *fc;
};

#define setjmp __builtin_setjmp
#define longjmp __builtin_longjmp

// JvB: added
extern "C" void abort() throw (MyOS::MultiThreading::ThreadExitException);
inline void abort() throw (MyOS::MultiThreading::ThreadExitException) { 
   NOT_IN_IRQ;
   DPRINTK( "\nabort() called" ); 
   Thread::currentThread().exit(-2);
   while(1);   // never comes here
}

/* JvB: copied from eh_personality.cc */
extern "C" _Unwind_Reason_Code
__gxx_personality_sj0(int version,
           _Unwind_Action actions,
           _Unwind_Exception_Class exception_class,
            struct _Unwind_Exception *ue_header,
          struct _Unwind_Context *context)
{
   DPRINTK( "\n__gxx_personality_sj0 called" );
   return _URC_NO_REASON;
}

extern "C" _Unwind_Reason_Code
__gxx_personality_v0(int version,
          _Unwind_Action actions,
          _Unwind_Exception_Class exception_class,
          struct _Unwind_Exception *ue_header,
          struct _Unwind_Context *context)
{
   return __gxx_personality_sj0(version, actions, exception_class, ue_header, context);
}

// in unwind-cxx.h, a header is allocated for each exception.
// Part of this header is '_Unwind_Exception unwindHeader', the
// address of which is passed back to the user program
//
typedef struct MyOS_ExHeader {    
   std::type_info *tinfo;   
   _Unwind_Ptr catchTemp;
   int         handler_switch_value;
   void *adjustedPtr;
   
   // structure read by calling program
   _Unwind_Exception unwindHeader;
   
   inline static MyOS_ExHeader& getFrom( void* obj ) {
      return ((MyOS_ExHeader*)obj)[-1];
   }

   inline static MyOS_ExHeader& getFromUE( _Unwind_Exception* uwe ) {
      return * ((MyOS_ExHeader*)((u8*)uwe - OFFSET_OF(MyOS_ExHeader,unwindHeader)));
   }

   
} MyOS_ExHeader;

// JvB: copied from eh_alloc.cc
// this function is called wherever you say 'throw'
extern "C" void *
__cxa_allocate_exception( unsigned thrown_size )
{
/*   
   DPRINTK( "\n__cxa_allocate_exception size=%d curthread=%x isrunning=%b cs=%x", 
      thrown_size, 
      &Thread::currentThread(), 
      Thread::currentThread().isRunning(),
      Processor::CS() 
   );
*/   
   // Cannot use this functionality before threading is initialized,
   // 'isRunning' is an indicator of that
   ASSERTION( Thread::currentThread().canThrow(), E_ERROR );   
   ASSERTION( Processor::CS() == 0x8, E_ERROR );   // not in IRQs !
   
   // Allocate memory in the context of the current thread
   thrown_size += sizeof(MyOS_ExHeader);
   u8* mem = (u8*) Thread::currentThread().allocateNoThrow( thrown_size );
   if (RARELY(mem==0)) {
      mem = Thread::currentThread().emergencyMem;
   }
   
   // just in case, should not be needed if mem interface returns zeroed
   memset_aligned( mem, 0, thrown_size );

   // per thread but not cleared, and quite small..
   return &mem[ sizeof(MyOS_ExHeader) ];
}

// JvB: declaration copied from eh_throw.cc, implementation is mine
extern "C" void __cxa_throw (void *thrown_exception,
             std::type_info *tinfo,
            void (*dest) (void *))
     __attribute__((noreturn));

// see http://www.opensource.apple.com/darwinsource/10.3/gccfast-1614/gcc/unwind-pe.h
#include "unwind-pe.h"

static SjLj_Function_Context* throw_internal(
   void *obj, std::type_info *tinfo );
   
// phase 1: search only
static _Unwind_Reason_Code phase1(
   SjLj_Function_Context* ctxt, std::type_info *t_type, void* ex, bool ph1 );

extern "C" void
__cxa_throw (void *obj, std::type_info *tinfo, void (*ex_destructor) (void *))
{
   DPRINTK( "\n__cxa_throw: obj=%x name=%s", obj, tinfo->name() );
   ASSERTION( false, E_CRITICAL );
   
   // keep type, via header of 'obj'
   MyOS_ExHeader::getFrom(obj).tinfo = tinfo;
   
   // Don't clobber any registers here... 
   SjLj_Function_Context* target = throw_internal(obj,tinfo);
   Thread::currentThread().exception_fc = target;  // unwind
   longjmp( target->jbuf, 1 );      // does not return here
}
   
/*
 * A separate function!
 * 
 * We need to be very careful to restore register state appropriately.
 * 'longjmp' will skip any POP sequences the compiler emits for saved registers 
 */   
static SjLj_Function_Context* throw_internal(void *obj, std::type_info *tinfo) {   
      
   // Phase1: iterate over all stackframes until 0, calling phase1 for each frame
   SjLj_Function_Context* c;
   for (c = (SjLj_Function_Context*)Thread::currentThread().exception_fc; c!=0; c = c->prev) {
      _Unwind_Reason_Code code = phase1(c,tinfo,obj,true);
      if (code==_URC_HANDLER_FOUND) {
         break;
      } else if (code != _URC_CONTINUE_UNWIND) {
        // return _URC_FATAL_PHASE1_ERROR;
        abort(); // die
      }      
   }
   
   // remember where we found a handler... for Resume
   MyOS_ExHeader& hdr = MyOS_ExHeader::getFrom(obj);
   hdr.unwindHeader.private_2 = (_Unwind_Ptr) c;

   // do phase2
   c->data[0]   = (_Unwind_Ptr) &hdr.unwindHeader;
   c->data[1]   = hdr.handler_switch_value;
   c->call_site = hdr.catchTemp - 1;
   return c;
}

// Copied from eh_personality.cc
static bool
get_adjusted_ptr (const std::type_info *catch_type,
      const std::type_info *throw_type,
       void **thrown_ptr_p)
{
   void *thrown_ptr = *thrown_ptr_p;

  // Pointer types need to adjust the actual pointer, not
  // the pointer to pointer that is the exception object.
  // This also has the effect of passing pointer types
  // "by value" through the __cxa_begin_catch return value.

   if (throw_type->__is_pointer_p ())
      thrown_ptr = *(void **) thrown_ptr;

   if (catch_type->__do_catch (throw_type, &thrown_ptr, 1)) {
      *thrown_ptr_p = thrown_ptr;
      return true;
   }
   return false;
}

// Copied from eh_personality.cc
static bool
check_exception_spec ( const u8 *ttype, u8 ttype_encoding, const std::type_info *throw_type, void *thrown_ptr, _Unwind_Sword filter_value)
{
   const u8 *e = (const u8*) (ttype - filter_value - 1);
   while (1)
   {
      const std::type_info *catch_type;
      _Unwind_Word tmp;

      e = read_uleb128 (e, &tmp);

      // Zero signals the end of the list.  If we've not found
      // a match by now, then we've failed the specification.
      if (tmp == 0)
        return false;

      // Match a ttype entry.
      // catch_type = get_ttype_entry (info, tmp);
      read_encoded_value_with_base( ttype_encoding, 0, (u8*)
         ttype - tmp * size_of_encoded_value (ttype_encoding), 
         (_Unwind_Ptr*) &catch_type ); // need reinterpret_cast ??

      // ??? There is currently no way to ask the RTTI code about the
      // relationship between two types without reference to a specific
      // object.  There should be; then we wouldn't need to mess with
      // thrown_ptr here.
      if (get_adjusted_ptr (catch_type, throw_type, &thrown_ptr))
         return true;
      }
}

// Also used in phase2...
static _Unwind_Reason_Code phase1(
SjLj_Function_Context* ctxt, std::type_info *throw_type, void* ex, bool phase1 )
{
   // 1. Look for cleanup code & handlers, by parsing lsda (TODO: Per thread)
   const u8* p = (const u8*) ctxt->lsda;
   
   // no LSDA means no handlers or cleanup code to execute in this frame -> go on
   if (!p) {
      DPRINTK( "\nphase1: no LSDA -> continue unwind" );
      return _URC_CONTINUE_UNWIND;
   } else {     
      _Unwind_Ptr ip = ctxt->call_site;     
      // The given "IP" is an index into the call-site table, with two
      // exceptions -- -1 means no-action, and 0 means terminate.
      // JvB: seems obvious to first do the common case...is this it?
      if (ip>0) {
         // _Unwind_Ptr landing_pad;   // goal is to find a "landing_pad"
         _Unwind_Ptr LPStart;

         // Find @LPStart, the base to which landing pad offsets are relative.
        u8 lpstart_encoding = *p++;
        if (lpstart_encoding != DW_EH_PE_omit) {
          // original code, but base is always 0 !
          // p = read_encoded_value (context, lpstart_encoding, p, &LPStart );
          p = read_encoded_value_with_base( lpstart_encoding, 0, p, &LPStart );
        } else LPStart = 0;
        
        u8 ttype_encoding = *p++;
        const u8* ttype;
        if (ttype_encoding != DW_EH_PE_omit)
        {
            _Unwind_Word tmp;
            p = read_uleb128 (p, &tmp);
            ttype = p + tmp;
        } else ttype = 0; 

        // u8 call_site_encoding = *p++; not used, also not in original src
        ++p;
        
        _Unwind_Word tmp;
        p = read_uleb128 (p, &tmp);
        const u8* action_table = p + tmp;
        
        _Unwind_Word cs_lp, cs_action;
        do {
         p = read_uleb128 (p, &cs_lp);
         p = read_uleb128 (p, &cs_action);
        } while (--ip);
        
        // and finally...
        _Unwind_Ptr landing_pad = cs_lp + 1;
        if (landing_pad==0) {
          DPRINTK( "\nNo cleanups or handlers, unwind" );
          return _URC_CONTINUE_UNWIND;  // no cleanup or handlers to run -> go on
        } else {                 
          if (cs_action==0) {
            // found a cleanup! 
            // DPRINTK( "\nCleanup found(register it?)" );
            if (phase1) return _URC_CONTINUE_UNWIND;
            DPRINTK( "\nProcess cleanup..." );
          } else {
            // DPRINTK( "\ncatch handler or exception specification" );
            const u8* action_record = action_table + cs_action - 1; 
            // catch handler or exception specification
   
            // throw_type = xh->exceptionType, now parameter to this function

            // Get this before the ptr is adjusted!
            MyOS_ExHeader& hdr = MyOS_ExHeader::getFrom(ex);
                        
            _Unwind_Sword ar_filter, ar_disp;
            bool saw_handler = false, saw_cleanup = false;
            while (1)
            {
               p = action_record;
               p = read_sleb128 (p, &ar_filter);
               read_sleb128 (p, &ar_disp);
               if (ar_filter==0) {
                  // Zero filter values are cleanups.
                  saw_cleanup=true;
               } else if (ar_filter > 0) {   // handlers
                  const std::type_info *catch_type;
                  read_encoded_value_with_base( ttype_encoding, /*ttype_base*/0, 
                     ttype - ar_filter*size_of_encoded_value (ttype_encoding), 
                     (_Unwind_Ptr*) &catch_type );
                     
                  // Null catch type is a catch-all handler.  We can catch
                  // foreign exceptions with this.
                  if (!catch_type) {
                     // GCC: if (!(actions & _UA_FORCE_UNWIND)) {
                     saw_handler = true;
                     break;
                  } else if (throw_type) {
                     // thrown_ptr == thrown exception == ex parameter
                     if (get_adjusted_ptr (catch_type, throw_type, &ex)) {
                        saw_handler = true;
                        break;
                     }
                  }                     
               } else { // Negative filter values are exception specifications.
                  if (throw_type && 
                     !check_exception_spec ( ttype, ttype_encoding, throw_type, ex, ar_filter)) {
                     saw_handler = true;
                     break;
                  }
               }
               if (ar_disp == 0) break;
               action_record = p + ar_disp;
            }
            
            if (saw_handler) {
               // eventually...
               hdr.catchTemp = landing_pad;
               hdr.handler_switch_value = ar_filter;
               hdr.adjustedPtr = ex;
               if (phase1) return _URC_HANDLER_FOUND;             
            } else if (phase1 || !saw_cleanup) return _URC_CONTINUE_UNWIND;
            
            // phase2: install the context
            ctxt->data[0]   = (_Unwind_Ptr) &hdr.unwindHeader;
            ctxt->data[1]   = ar_filter;       // handler_switch_value;
            ctxt->call_site = landing_pad;
            return _URC_INSTALL_CONTEXT;
          }
        }
      } else if (ip<0) {
         return _URC_CONTINUE_UNWIND;
      } else {
         // __cxa_begin_catch (&xh->unwindHeader);
         // __terminate (xh->terminateHandler);
         // abort(); see below
      }
   }
   abort();
}

extern "C" void __cxa_rethrow () __attribute__((noreturn));
extern "C" void
__cxa_rethrow ()
{
   DPRINTK( "\n__cxa_rethrow() TODO" );
   while (1);
}

// JvB: copied from eh_catch.cc
extern "C" void *
__cxa_begin_catch (void *ex)
{
   // DPRINTK( "\n__cxa_begin_catch()" );
   return MyOS_ExHeader::getFrom(ex).adjustedPtr;
}

// JvB: copied from eh_catch.cc
extern "C" void
__cxa_end_catch ()
{
   // DPRINTK( "\n__cxa_end_catch()" );

   // TODO: Free the exception object here?
}

extern "C" void
__cxa_free_exception( void* vptr ) throw()
{
   // TODO, see eh_alloc.cc:
   // std::free (ptr - sizeof (__cxa_exception));
}

typedef struct
{
  _Unwind_Personality_Fn personality;
} _Unwind_FrameState;

// if only this one could be inlined...
void
_Unwind_SjLj_Register (struct SjLj_Function_Context *fc)
{
   // DPRINTK( "\n_Unwind_SjLj_Register fc=%x", fc );
   register Thread* cur = &Thread::currentThread();
   fc->prev = (SjLj_Function_Context*)cur->exception_fc;
   cur->exception_fc = fc;
}

/*
static inline struct SjLj_Function_Context *
_Unwind_SjLj_GetContext (void)
{
#if __GTHREADS
  if (use_fc_key < 0)
    fc_key_init_once ();

  if (use_fc_key)
    return __gthread_getspecific (fc_key);
#endif
  return fc_static;
}

static inline void
_Unwind_SjLj_SetContext (struct SjLj_Function_Context *fc)
{
#if __GTHREADS
  if (use_fc_key < 0)
    fc_key_init_once ();

  if (use_fc_key)
    __gthread_setspecific (fc_key, fc);
  else
#endif
    fc_static = fc;
}
*/

void
_Unwind_SjLj_Unregister (struct SjLj_Function_Context *fc)  // UNCOMPRESSED
{
  Thread::currentThread().exception_fc = fc->prev;
}

extern "C" void
__cxa_call_unexpected (void *exc_obj_in)
{
   DPRINTK( "\n__cxa_call_unexpected called!" );
   while (1);
}

extern "C" void
__cxa_call_terminate (void *)
{
   DPRINTK( "\n__cxa_call_terminate called!" );
   while (1);
}

/*

// Get/set the return data value at INDEX in CONTEXT. 

_Unwind_Word
_Unwind_GetGR (struct _Unwind_Context *context, int index)
{
  return context->fc->data[index];
}

void
_Unwind_SetGR (struct _Unwind_Context *context, int index, _Unwind_Word val)
{
  context->fc->data[index] = val;
}

// Get the call-site index as saved in CONTEXT.  

_Unwind_Ptr
_Unwind_GetIP (struct _Unwind_Context *context)
{
  return context->fc->call_site + 1;
}

// Set the return landing pad index in CONTEXT.

void
_Unwind_SetIP (struct _Unwind_Context *context, _Unwind_Ptr val)
{
  context->fc->call_site = val - 1;
}

void *
_Unwind_GetLanguageSpecificData (struct _Unwind_Context *context)
{
  return context->fc->lsda;
}

_Unwind_Ptr
_Unwind_GetRegionStart (struct _Unwind_Context *context )
{
  return 0;
}

void *
_Unwind_FindEnclosingFunction (void *pc)
{
  return 0;
}

#ifndef __ia64__
_Unwind_Ptr
_Unwind_GetDataRelBase (struct _Unwind_Context *context )
{
  return 0;
}

_Unwind_Ptr
_Unwind_GetTextRelBase (struct _Unwind_Context *context )
{
  return 0;
}
#endif

static inline _Unwind_Reason_Code
uw_frame_state_for (struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
  if (context->fc == 0)
    {
      fs->personality = 0;
      return _URC_END_OF_STACK;
    }
  else
    {
      fs->personality = context->fc->personality;
      return _URC_NO_REASON;
    }
}

static inline void
uw_update_context (struct _Unwind_Context *context,
         _Unwind_FrameState *fs )
{
  context->fc = context->fc->prev;
}

static inline void
uw_init_context (struct _Unwind_Context *context)
{
  context->fc = _Unwind_SjLj_GetContext ();
}

// ??? There appear to be bugs in integrate.c wrt __builtin_longjmp and
//   virtual-stack-vars.  An inline version of this segfaults on SPARC.
#define uw_install_context(CURRENT, TARGET)     \
  do                    \
    {                   \
      _Unwind_SjLj_SetContext ((TARGET)->fc);      \
      longjmp ((TARGET)->fc->jbuf, 1);       \
    }                   \
  while (0)


static inline _Unwind_Ptr
uw_identify_context (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->fc;
}

*/

/* Play games with unwind symbols so that we can have call frame
   and sjlj symbols in the same shared library.  Not that you can
   use them simultaneously...  */
#define _Unwind_RaiseException    _Unwind_SjLj_RaiseException
#define _Unwind_ForcedUnwind      _Unwind_SjLj_ForcedUnwind
// Modern GCC emits the generic name even for this freestanding runtime.

void
_Unwind_Resume (struct _Unwind_Exception *exc)
{
   DPRINTK( "\n_Unwind_Resume exc=%x", exc );

   MyOS_ExHeader& hdr = MyOS_ExHeader::getFromUE(exc);
   SjLj_Function_Context* handlerframe = (SjLj_Function_Context*) exc->private_2;

   SjLj_Function_Context* c;
   for (c = (SjLj_Function_Context*)Thread::currentThread().exception_fc; c!=0; c = c->prev) {
      // shortcut to found handler
      if ( c == handlerframe ) {
         c->data[0] = (_Unwind_Ptr) exc;
         c->data[1] = hdr.handler_switch_value;
         c->call_site = hdr.catchTemp - 1;
         break;   // install context
      } else {      
         _Unwind_Reason_Code code = phase1(c,hdr.tinfo,exc+1,false);
         if (code==_URC_HANDLER_FOUND) {
            break;
         } else if (code != _URC_CONTINUE_UNWIND) {
           // return _URC_FATAL_PHASE1_ERROR;
           abort(); // die
         }
      }         
   }

   Thread::currentThread().exception_fc = c;          // unwind
   longjmp( c->jbuf, 1 );  // does not return here      
}

// #include "unwind.inc"
