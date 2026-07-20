#ifndef SIGNAL_H
#define SIGNAL_H

/**
 * Note: Signals in MyOS have slightly different semantics in MyOS compared to e.g. Linux.
 * 
 * Situations that cause signals in other systems may not cause them in MyOS.
 * For example, a NULL pointer reference causes a NullPointerException to be thrown (containing
 * the address of the culprit instruction), instead of a SIGBUS
 * 
 * Also, in MyOS signals do not break procedures. They are only handled upon specific points in
 * the execution stream: upon execution of a 'ret' (or 'iret')  
 */ 

/**
 * This macro is to be used for the declaration of functions that (may) hold a lock and 
 * call sibling functions (i.e. execute a 'call' or 'call far', not inlined)
 * 
 * The kernel will ensure that these calls are not interrupted by signals
 **/
#define SIGNAL_SAFE __attribute__ ((section(".text.signal_safe")))

#endif
