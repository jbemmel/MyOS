#ifndef ASSERTION_H
#define ASSERTION_H

#include "Devices/Display/screentypes.h"    // EColor

//#ifdef DEBUG

namespace MyOS { namespace Debugging {
  
using namespace Devices::Display;

/// Severity as defined in RFC 3164 Table 2
enum E_SEVERITY {
   E_EMERGENCY = 0,     ///< Emergency: system is unusable
   E_ALERT     = 1,     ///< Alert: action must be taken immediately
   E_CRITICAL  = 2,     ///< Critical: critical conditions
   E_ERROR     = 3,     ///< Error: error conditions
   E_WARNING   = 4,     ///< Warning: warning conditions
   E_NOTICE    = 5,     ///< Notice: normal but significant condition
   E_INFORM    = 6,     ///< Informational: informational messages
   E_DEBUG     = 7,     ///< Debug: debug-level messages

   // My own additions
   E_NONE      = 8,     ///< No severity (i.e. disables display of debug statements)
};

void assertionFailed( E_SEVERITY s, const char* fileline, const char* func, int debugvalue );
void checkpoint( char c, EColor color = E_WHITE );
  
}} // namespace

//#endif   // DEBUG

#endif
