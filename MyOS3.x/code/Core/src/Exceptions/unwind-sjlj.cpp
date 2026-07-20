/* GCC unwind-sjlj.c + unwind.inc code
 *
 * This code used to reside in a .c file, and uses C-style calling conventions (!)
 *
 * Tried to link this in binary form, but code requires too many changes
 * (getting/setting context from current thread)
 */

/*
#include "tconfig.h"
#include "tsystem.h"
#include "coretypes.h"
#include "tm.h"
#include "unwind.h"
#include "gthr.h"
*/

#define LIBGCC2_UNWIND_ATTRIBUTE __attribute__((regparm(0)))
#include "unwind.h"

#define longjmp __builtin_longjmp

/* The setjmp side is dealt with in the except.c file.  */
#undef setjmp
#define setjmp setjmp_should_not_be_used_in_this_file

#include "types.h"
#include "CPU/Processor.h"
#include "MyOSExceptions.h"
#include "IThread.h"

#define gcc_assert(x) ASSERTION(x,E_ERROR)

using MyOS::MultiThreading::Thread;

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
  _Unwind_Personality_Fn personality;
  void *lsda;

  void *jbuf[]; // using built-in setjmp
};

struct _Unwind_Context
{
  struct SjLj_Function_Context *fc;
};

typedef struct
{
  _Unwind_Personality_Fn personality;
} _Unwind_FrameState;


/* Manage the chain of registered function contexts.  */

/*
void
_Unwind_SjLj_Register (struct SjLj_Function_Context *fc)
{
  fc->prev = fc_static;
  fc_static = fc;
}
*/

static inline struct SjLj_Function_Context *
_Unwind_SjLj_GetContext (void)
{
    // return fc_static;
    Thread &cur = Thread::currentThread();
    return (SjLj_Function_Context *) cur.exception_fc;
}

static inline void
_Unwind_SjLj_SetContext (struct SjLj_Function_Context *fc)
{
    // fc_static = fc;
    Thread &cur = Thread::currentThread();
    cur.exception_fc = fc->prev;
}

/*

void
_Unwind_SjLj_Unregister (struct SjLj_Function_Context *fc)
{
  _Unwind_SjLj_SetContext (fc->prev);
}*/


/* Get/set the return data value at INDEX in CONTEXT.  */

_Unwind_Word
_Unwind_GetGR (struct _Unwind_Context *context, int index)
{
  return context->fc->data[index];
}

/* Get the value of the CFA as saved in CONTEXT.  */

_Unwind_Word
_Unwind_GetCFA (struct _Unwind_Context *context __attribute__((unused)))
{
  /* ??? Ideally __builtin_setjmp places the CFA in the jmpbuf.  */

  /* This is a crude imitation of the CFA: the saved stack pointer.
     This is roughly the CFA of the frame before CONTEXT.  When using the
     DWARF-2 unwinder _Unwind_GetCFA returns the CFA of the frame described
     by CONTEXT instead; but for DWARF-2 the cleanups associated with
     CONTEXT have already been run, and for SJLJ they have not yet been.  */
  if (context->fc != 0)
    return (_Unwind_Word) context->fc->jbuf[2];

  /* Otherwise we're out of luck for now.  */
  return (_Unwind_Word) 0;
}

void
_Unwind_SetGR (struct _Unwind_Context *context, int index, _Unwind_Word val)
{
  context->fc->data[index] = val;
}

/* Get the call-site index as saved in CONTEXT.  */

_Unwind_Ptr
_Unwind_GetIP (struct _Unwind_Context *context)
{
  return context->fc->call_site + 1;
}

_Unwind_Ptr
_Unwind_GetIPInfo (struct _Unwind_Context *context, int *ip_before_insn)
{
  *ip_before_insn = 0;
  if (context->fc != 0)
    return context->fc->call_site + 1;
  else
    return 0;
}

/* Set the return landing pad index in CONTEXT.  */

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
_Unwind_GetRegionStart (struct _Unwind_Context *context __attribute__((unused)) )
{
  return 0;
}

void *
_Unwind_FindEnclosingFunction (void *pc __attribute__((unused)))
{
  return 0;
}

#ifndef __ia64__
_Unwind_Ptr
_Unwind_GetDataRelBase (struct _Unwind_Context *context __attribute__((unused)) )
{
  return 0;
}

_Unwind_Ptr
_Unwind_GetTextRelBase (struct _Unwind_Context *context __attribute__((unused)) )
{
  return 0;
}
#endif

static inline _Unwind_Reason_Code uw_frame_state_for(
        struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
    if (context->fc == 0) {
        fs->personality = 0;
        return _URC_END_OF_STACK;
    } else {
        fs->personality = context->fc->personality;
        return _URC_NO_REASON;
    }
}

static inline void
uw_update_context (struct _Unwind_Context *context,
		   _Unwind_FrameState *fs __attribute__((unused)) )
{
  context->fc = context->fc->prev;
}

static void
uw_advance_context (struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
  _Unwind_SjLj_Unregister (context->fc);
  uw_update_context (context, fs);
}

static inline void
uw_init_context (struct _Unwind_Context *context)
{
  context->fc = _Unwind_SjLj_GetContext ();
}

static void __attribute__((noreturn))
uw_install_context (struct _Unwind_Context *current __attribute__((unused)),
                    struct _Unwind_Context *target)
{
  _Unwind_SjLj_SetContext (target->fc);
  longjmp (target->fc->jbuf, 1);
}

static inline _Unwind_Ptr
uw_identify_context (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->fc;
}


/* Play games with unwind symbols so that we can have call frame
   and sjlj symbols in the same shared library.  Not that you can
   use them simultaneously...  */
#define _Unwind_RaiseException		_Unwind_SjLj_RaiseException
#define _Unwind_ForcedUnwind		_Unwind_SjLj_ForcedUnwind
#define _Unwind_Resume			_Unwind_SjLj_Resume
#define _Unwind_Resume_or_Rethrow	_Unwind_SjLj_Resume_or_Rethrow

// #include "unwind.inc"

static _Unwind_Reason_Code _Unwind_RaiseException_Phase2(
        struct _Unwind_Exception *exc, struct _Unwind_Context *context)
{
    _Unwind_Reason_Code code;

    while (1)
    {
        _Unwind_FrameState fs;
        int match_handler;

        code = uw_frame_state_for(context, &fs);

        /* Identify when we've reached the designated handler context.  */
        match_handler = (uw_identify_context(context) == exc->private_2
                ? _UA_HANDLER_FRAME : 0);

        if (code != _URC_NO_REASON)
            /* Some error encountered.  Usually the unwinder doesn't
             diagnose these and merely crashes.  */
            return _URC_FATAL_PHASE2_ERROR;

        /* Unwind successful.  Run the personality routine, if any.  */
        if (fs.personality)
        {
            code = (*fs.personality)(1, _UA_CLEANUP_PHASE | match_handler,
                    exc->exception_class, exc, context);
            if (code == _URC_INSTALL_CONTEXT)
                break;
            if (code != _URC_CONTINUE_UNWIND)
                return _URC_FATAL_PHASE2_ERROR;
        }

        /* Don't let us unwind past the handler context.  */
        gcc_assert (!match_handler);

        uw_update_context(context, &fs);
    }

    return code;
}

/* Raise an exception, passing along the given exception object.  */
_Unwind_Reason_Code /* LIBGCC2_UNWIND_ATTRIBUTE */
_Unwind_RaiseException(struct _Unwind_Exception *exc)
{
    struct _Unwind_Context this_context, cur_context;
    _Unwind_Reason_Code code;

    /* Set up this_context to describe the current stack frame.  */
    uw_init_context(&this_context);
    cur_context = this_context;

    /* Phase 1: Search.  Unwind the stack, calling the personality routine
     with the _UA_SEARCH_PHASE flag set.  Do not modify the stack yet.  */
    while (1)
    {
        _Unwind_FrameState fs;

        /* Set up fs to describe the FDE for the caller of cur_context.  The
         first time through the loop, that means __cxa_throw.  */
        code = uw_frame_state_for(&cur_context, &fs);

        if (code == _URC_END_OF_STACK) {
            /* Hit end of stack with no handler found.  */
            DPRINTK( "\n_Unwind_RaiseException: end-of-stack with no handler found" );
            return _URC_END_OF_STACK;
        }

        if (code != _URC_NO_REASON)
            /* Some error encountered.  Ususally the unwinder doesn't
             diagnose these and merely crashes.  */
            return _URC_FATAL_PHASE1_ERROR;

        /* Unwind successful.  Run the personality routine, if any.  */
        if (fs.personality)
        {
            code = (*fs.personality)(1, _UA_SEARCH_PHASE, exc->exception_class,
                    exc, &cur_context);
            if (code == _URC_HANDLER_FOUND) {
                break;
            } else if (code != _URC_CONTINUE_UNWIND) {
                return _URC_FATAL_PHASE1_ERROR;
            }
        }

        /* Update cur_context to describe the same frame as fs.  */
        uw_update_context(&cur_context, &fs);
    }

    /* Indicate to _Unwind_Resume and associated subroutines that this
     is not a forced unwind.  Further, note where we found a handler.  */
    exc->private_1 = 0;
    exc->private_2 = uw_identify_context(&cur_context);

    cur_context = this_context;
    code = _Unwind_RaiseException_Phase2(exc, &cur_context);
    if (code != _URC_INSTALL_CONTEXT)
        return code;

    uw_install_context(&this_context, &cur_context);
}

/* Subroutine of _Unwind_ForcedUnwind also invoked from _Unwind_Resume.  */

static _Unwind_Reason_Code _Unwind_ForcedUnwind_Phase2(
        struct _Unwind_Exception *exc, struct _Unwind_Context *context)
{
    _Unwind_Stop_Fn stop = (_Unwind_Stop_Fn) (_Unwind_Ptr) exc->private_1;
    void *stop_argument = (void *) (_Unwind_Ptr) exc->private_2;
    _Unwind_Reason_Code code;

    while (1)
    {
        _Unwind_FrameState fs;
        int action;

        /* Set up fs to describe the FDE for the caller of cur_context.  */
        code = uw_frame_state_for(context, &fs);
        if (code != _URC_NO_REASON && code != _URC_END_OF_STACK)
            return _URC_FATAL_PHASE2_ERROR;

        /* Unwind successful.  */
        action = _UA_FORCE_UNWIND | _UA_CLEANUP_PHASE;
        if (code == _URC_END_OF_STACK) {
            action |= _UA_END_OF_STACK;
        }
        if ( _URC_NO_REASON != (*stop)(1, action, exc->exception_class, exc, context, stop_argument) ) {
            return _URC_FATAL_PHASE2_ERROR;
        }

        /* Stop didn't want to do anything.  Invoke the personality
         handler, if applicable, to run cleanups.  */
        if (code == _URC_END_OF_STACK)
            break;

        if (fs.personality)
        {
            code = (*fs.personality)(1, _UA_FORCE_UNWIND | _UA_CLEANUP_PHASE,
                    exc->exception_class, exc, context);
            if (code == _URC_INSTALL_CONTEXT)
                break;
            if (code != _URC_CONTINUE_UNWIND)
                return _URC_FATAL_PHASE2_ERROR;
        }

        /* Update cur_context to describe the same frame as fs, and discard
         the previous context if necessary.  */
        uw_advance_context(context, &fs);
    }

    return code;
}


/* Raise an exception for forced unwinding.  */

_Unwind_Reason_Code /* LIBGCC2_UNWIND_ATTRIBUTE */
_Unwind_ForcedUnwind (struct _Unwind_Exception *exc,
		      _Unwind_Stop_Fn stop, void * stop_argument)
{
  struct _Unwind_Context this_context, cur_context;
  _Unwind_Reason_Code code;

  uw_init_context (&this_context);
  cur_context = this_context;

  exc->private_1 = (_Unwind_Ptr) stop;
  exc->private_2 = (_Unwind_Ptr) stop_argument;

  code = _Unwind_ForcedUnwind_Phase2 (exc, &cur_context);
  if (code != _URC_INSTALL_CONTEXT)
    return code;

  uw_install_context (&this_context, &cur_context);
}


/* Resume propagation of an existing exception.  This is used after
   e.g. executing cleanup code, and not to implement rethrowing.  */

void LIBGCC2_UNWIND_ATTRIBUTE
_Unwind_Resume (struct _Unwind_Exception *exc)
{
  struct _Unwind_Context this_context, cur_context;
  _Unwind_Reason_Code code;

  uw_init_context (&this_context);
  cur_context = this_context;

  /* Choose between continuing to process _Unwind_RaiseException
     or _Unwind_ForcedUnwind.  */
  if (exc->private_1 == 0)
    code = _Unwind_RaiseException_Phase2 (exc, &cur_context);
  else
    code = _Unwind_ForcedUnwind_Phase2 (exc, &cur_context);

  gcc_assert (code == _URC_INSTALL_CONTEXT);

  uw_install_context (&this_context, &cur_context);
}


/* Resume propagation of an FORCE_UNWIND exception, or to rethrow
   a normal exception that was handled.  */

_Unwind_Reason_Code LIBGCC2_UNWIND_ATTRIBUTE
_Unwind_Resume_or_Rethrow (struct _Unwind_Exception *exc)
{
  struct _Unwind_Context this_context, cur_context;
  _Unwind_Reason_Code code;

  /* Choose between continuing to process _Unwind_RaiseException
     or _Unwind_ForcedUnwind.  */
  if (exc->private_1 == 0)
    return _Unwind_RaiseException (exc);

  uw_init_context (&this_context);
  cur_context = this_context;

  code = _Unwind_ForcedUnwind_Phase2 (exc, &cur_context);

  gcc_assert (code == _URC_INSTALL_CONTEXT);

  uw_install_context (&this_context, &cur_context);
}


/* A convenience function that calls the exception_cleanup field.  */

void
_Unwind_DeleteException (struct _Unwind_Exception *exc)
{
  if (exc->exception_cleanup)
    (*exc->exception_cleanup) (_URC_FOREIGN_EXCEPTION_CAUGHT, exc);
}


/* Perform stack backtrace through unwind data.  */

_Unwind_Reason_Code LIBGCC2_UNWIND_ATTRIBUTE
_Unwind_Backtrace(_Unwind_Trace_Fn trace, void * trace_argument)
{
  struct _Unwind_Context context;
  _Unwind_Reason_Code code;

  uw_init_context (&context);

  while (1)
    {
      _Unwind_FrameState fs;

      /* Set up fs to describe the FDE for the caller of context.  */
      code = uw_frame_state_for (&context, &fs);
      if (code != _URC_NO_REASON && code != _URC_END_OF_STACK)
	return _URC_FATAL_PHASE1_ERROR;

      /* Call trace function.  */
      if ((*trace) (&context, trace_argument) != _URC_NO_REASON)
	return _URC_FATAL_PHASE1_ERROR;

      /* We're done at end of stack.  */
      if (code == _URC_END_OF_STACK)
	break;

      /* Update context to describe the same frame as fs.  */
      uw_update_context (&context, &fs);
    }

  return code;
}
