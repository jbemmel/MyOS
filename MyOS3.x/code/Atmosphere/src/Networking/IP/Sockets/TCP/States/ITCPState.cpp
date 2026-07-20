#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

// Timers, can be overridden
// virtual 
void 
ITCPState::onTimer( CTCPSocket &s ) 
{
    // default: nothing
}

/// User actions
// virtual 
myos_result_t 
ITCPState::connect( CTCPSocket &s, const sockadr_t &remote, u32 options )
{
    return E_MYOS_E_FAIL;   // exists
}

// virtual 
s32 
ITCPState::send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload ) 
{
    return E_NOT_IMPLEMENTED;
}

}}}}
