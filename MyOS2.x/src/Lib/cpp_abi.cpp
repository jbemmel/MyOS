#include "defs.h"

// GCC 4.0 needs this
void __cxa_pure_virtual() { __pure_virtual(); }
int atexit( void (*function)(void) ) { return -1; }
