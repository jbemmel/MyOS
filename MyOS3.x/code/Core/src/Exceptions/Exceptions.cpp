/**
 * 19/8/2004 - Found out that this does not work with GCC 3.4.1 (yet), reverting to 3.3.3
 */

#include "types.h"
#include "CPU/Processor.h"
#include "MyOSExceptions.h"
#include "IThread.h"
#include "string.h" // memset_aligned
#include "debug.h"

using MyOS::MultiThreading::IThread;
using MyOS::MultiThreading::Thread;
using MyOS::Processor;

extern "C" EXPORT void abort();

/**
 * Attempt to take only the very minimal of GCC exception shit
 *
 * see http://www.codesourcery.com/public/cxx-abi/abi-eh.html
 */

//#include "tconfig.h"
//#include "tsystem.h"
//#include "unwind.h"

// JvB: typeinfo for int
#include "typeinfo.h"
#include "unwind-cxx.h"

namespace __cxxabiv1 {

// JvB: Copied from cxxabi.h

/* type information for int, float etc
class __fundamental_type_info
  : public std::type_info
{
public:
  virtual ~__fundamental_type_info ();
public:
  explicit __fundamental_type_info (const char *__n)
    : std::type_info (__n)
    { }
};
*/

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
  virtual ~__pointer_type_info ();
public:
  explicit __pointer_type_info (const char *__n,
                                int __quals,
                                const std::type_info *__type)
    : __pbase_type_info (__n, __quals, __type)
    { }

// implementation defined member functions
protected:
    virtual bool __is_pointer_p () const { return true; }

protected:
  //virtual bool __pointer_catch (const __pbase_type_info *__thr_type,
  //                              void **__thr_obj,
  //                              unsigned __outer) const;
};

class __class_type_info;

/* helper class for __vmi_class_type */
class __base_class_info
{
  /* abi defined member variables */
  public:
    const __class_type_info *base;    /* base class type */
    long vmi_offset_flags;            /* offset and info */

  /* implementation defined types */
  public:
    enum vmi_masks {
      virtual_mask = 0x1,
      public_mask = 0x2,
      hwm_bit = 2,
      offset_shift = 8          /* bits to shift offset by */
    };

  /* implementation defined member functions */
  public:
    bool __is_virtual_p () const
      { return vmi_offset_flags & virtual_mask; }
    bool __is_public_p () const
      { return vmi_offset_flags & public_mask; }
    __PTRDIFF_TYPE__ __offset () const
      {
        // This shift, being of a signed type, is implementation defined. GCC
        // implements such shifts as arithmetic, which is what we want.
        return static_cast<__PTRDIFF_TYPE__> (vmi_offset_flags) >> offset_shift;
      }
};

// type information for a class
class EXPORT __class_type_info
  : public std::type_info
{
// abi defined member functions
public:
  virtual ~__class_type_info ();
public:
  explicit __class_type_info (const char *__n)
    : type_info (__n)
    { }

  /* implementation defined types */
  public:
    /* sub_kind tells us about how a base object is contained within a derived
       object. We often do this lazily, hence the UNKNOWN value. At other times
       we may use NOT_CONTAINED to mean not publicly contained. */
    enum __sub_kind
    {
      __unknown = 0,              /* we have no idea */
      __not_contained,            /* not contained within us (in some */
                                  /* circumstances this might mean not contained */
                                  /* publicly) */
      __contained_ambig,          /* contained ambiguously */

      __contained_virtual_mask = __base_class_info::virtual_mask, /* via a virtual path */
      __contained_public_mask = __base_class_info::public_mask,   /* via a public path */
      __contained_mask = 1 << __base_class_info::hwm_bit,         /* contained within us */

      __contained_private = __contained_mask,
      __contained_public = __contained_mask | __contained_public_mask
    };
};

/* type information for a class with a single non-virtual base */
class EXPORT __si_class_type_info
  : public __class_type_info
{
/* abi defined member variables */
protected:
  const __class_type_info *base;    /* base type */

/* abi defined member functions */
public:
  virtual ~__si_class_type_info ();
public:
  explicit __si_class_type_info (const char *__n,
                                 const __class_type_info *__base)
    : __class_type_info (__n), base (__base)
    { }

/* implementation defined member functions
protected:
  virtual bool __do_dyncast (__PTRDIFF_TYPE__ __src2dst,
                             __sub_kind __access_path,
                             const __class_type_info *__dst_type,
                             const void *__obj_ptr,
                             const __class_type_info *__src_type,
                             const void *__src_ptr,
                             __dyncast_result &__result) const;
  virtual __sub_kind __do_find_public_src (__PTRDIFF_TYPE__ __src2dst,
                                           const void *__obj_ptr,
                                           const __class_type_info *__src_type,
                                           const void *__sub_ptr) const;
  virtual bool __do_upcast (const __class_type_info *__dst,
                            const void *__obj,
                            __upcast_result &__restrict __result) const;
*/
};

/*
 * type information for a class with multiple and/or virtual bases
 *
 * obtained from gcc-cxxabi.h
 */
class EXPORT __vmi_class_type_info : public __class_type_info {
/* abi defined member variables */
public:
  int vmi_flags;                  /* details about the class heirarchy */
  int vmi_base_count;             /* number of direct bases */
  __base_class_info vmi_bases[1]; /* array of bases */
  /* The array of bases uses the trailing array struct hack
     so this class is not constructable with a normal constructor. It is
     internally generated by the compiler. */

/* abi defined member functions */
public:
  virtual ~__vmi_class_type_info ();
public:
  explicit __vmi_class_type_info (const char *__n,
                                  int __flags)
    : __class_type_info (__n), vmi_flags (__flags), vmi_base_count (0)
    { }

/* implementation defined types */
public:
  enum vmi_flags_masks {
    non_diamond_repeat_mask = 0x1,   /* distinct instance of repeated base */
    diamond_shaped_mask = 0x2,       /* diamond shaped multiple inheritance */
    non_public_base_mask = 0x4,      /* has non-public direct or indirect base */
    public_base_mask = 0x8,          /* has public base (direct) */

    __flags_unknown_mask = 0x10
  };

/* implementation defined member functions
protected:
  virtual bool __do_dyncast (__PTRDIFF_TYPE__ __src2dst,
                             __sub_kind __access_path,
                             const __class_type_info *__dst_type,
                             const void *__obj_ptr,
                             const __class_type_info *__src_type,
                             const void *__src_ptr,
                             __dyncast_result &__result) const;
  virtual __sub_kind __do_find_public_src (__PTRDIFF_TYPE__ __src2dst,
                                           const void *__obj_ptr,
                                           const __class_type_info *__src_type,
                                           const void *__src_ptr) const;
  virtual bool __do_upcast (const __class_type_info *__dst,
                            const void *__obj,
                            __upcast_result &__restrict __result) const;
*/
};

// This has special meaning to the compiler, and will cause it
// to emit the type_info structures for the fundamental types which are
/* mandated to exist in the runtime.
__fundamental_type_info::
~__fundamental_type_info ()
{}
*/

__pointer_type_info::
~__pointer_type_info ()
{}

__class_type_info::
~__class_type_info ()
{}

__vmi_class_type_info::
~__vmi_class_type_info ()
{}

__si_class_type_info::
~__si_class_type_info ()
{}

}   // namespace

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
  _Unwind_Personality_Fn personality;

  // JvB: Language specific data area
  void *lsda;

  void *jbuf[];
};

extern "C" EXPORT void _Unwind_SjLj_Register( struct SjLj_Function_Context *fc );

void
_Unwind_SjLj_Register (struct SjLj_Function_Context *fc ) // allocated on stack by gcc!
{
    // Also called very early during boot, cannot use display here!

   Thread& cur = Thread::currentThread();
   fc->prev = (SjLj_Function_Context*) cur.exception_fc;
   cur.exception_fc = fc;
}

extern "C" EXPORT void _Unwind_SjLj_Unregister( struct SjLj_Function_Context *fc );

void
_Unwind_SjLj_Unregister (struct SjLj_Function_Context *fc)
{
    ASSERTION( fc, E_CRITICAL );
    Thread& cur = Thread::currentThread();
    cur.exception_fc = fc->prev;
}

namespace __cxxabiv1 {

// Prototypes, copied from unwind-cxx.h
extern "C" EXPORT void *__cxa_allocate_exception(size_t thrown_size) throw() SIGNAL_SAFE;
extern "C" EXPORT void __cxa_free_exception(void *thrown_exception) throw() SIGNAL_SAFE;
extern "C" EXPORT void __cxa_throw (void *thrown_exception,
             std::type_info *tinfo,
            void (*dest) (void *)) NORETURN SIGNAL_SAFE;
extern "C" EXPORT void *__cxa_begin_catch (void *) throw() SIGNAL_SAFE;
extern "C" EXPORT void __cxa_end_catch () SIGNAL_SAFE;
extern "C" EXPORT void __cxa_call_unexpected (void *ex) NORETURN SIGNAL_SAFE;
extern "C" EXPORT void __cxa_rethrow() NORETURN SIGNAL_SAFE;

// Called from personality routine
extern "C" void
__cxa_call_terminate( /*_Unwind_Exception*/void* ue_header) // NORETURN
{
    BUG( "__cxa_call_terminate called" );
}

typedef void (*_Unwind_Exception_Cleanup_Fn) (_Unwind_Reason_Code,
                          struct _Unwind_Exception *);

struct _Unwind_Exception
{
  _Unwind_Exception_Class exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;
  _Unwind_Word private_1;
  _Unwind_Word private_2;

  /* @@@ The IA-64 ABI says that this structure must be double-word aligned.
     Taking that literally does not make much sense generically.  Instead we
     provide the maximum alignment required by any type for the machine.  */
} __attribute__((__aligned__));

// in unwind-cxx.h, a header is allocated for each exception.
// Part of this header is '_Unwind_Exception unwindHeader', the
// address of which is passed back to the user program
/*
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
} MyOS_ExHeader;*/

// JvB: copied from eh_alloc.cc
// this function is called wherever you say 'throw'
void*
__cxa_allocate_exception( size_t thrown_size ) throw()
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
   ASSERTIONv( Thread::currentThread().canThrow(), E_CRITICAL, Thread::currentThread().getState() );
   // ASSERTION( Processor::CS() == 0x8, E_ERROR );   // not in IRQs ?

   // Allocate memory in the context of the current thread
   thrown_size += sizeof( __cxa_exception );
   u8* mem = (u8*) Thread::currentThread().allocateNoThrow( thrown_size );
   if (RARELY(mem==0)) {
      mem = Thread::currentThread().emergencyMem;
   }

   // just in case, should not be needed if mem interface returns zeroed
   MyOS::memset_aligned( mem, 0, thrown_size );

   // per thread but not cleared, and quite small..
   return &mem[ sizeof(__cxa_exception) ];
}

void
__cxa_free_exception( void* vptr ) throw()
{
    u8* p = ((u8*) vptr) - sizeof(__cxa_exception);

    ASSERTION( p != Thread::currentThread().emergencyMem, E_ERROR );
    Thread::currentThread().free( p );
}

extern "C" EXPORT __cxa_eh_globals*
__cxa_get_globals() throw();

__cxa_eh_globals*
__cxa_get_globals() throw()
{
    return (__cxa_eh_globals*) Thread::currentThread().getEHGlobals();
}

extern "C" EXPORT __cxa_eh_globals*
__cxa_get_globals_fast() throw();

__cxa_eh_globals*
__cxa_get_globals_fast() throw()
{
    return __cxa_get_globals();
}

/* The current installed user handler.  */
EXPORT std::terminate_handler __terminate_handler = 0; /* TODO std::abort */

/* The current installed user handler.  */
EXPORT std::unexpected_handler __unexpected_handler = 0; /* TODO std::terminate */


/* EXPORT */ void __terminate (std::terminate_handler handler);

void
__terminate (std::terminate_handler handler)
{
  try {
      handler();
      abort();
  } catch (...) {
      abort();
  }
}

}   // namespace __cxxabiv1
