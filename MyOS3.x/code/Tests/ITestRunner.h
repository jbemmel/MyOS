#ifndef ITestRunner_H
#define ITestRunner_H

#include "IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS
{
namespace Tests
{
INTERFACE( ITestRunner, "65ccb493-30ce-4958-9d4a-3152df94bbd9" )

virtual myos_result_t showResults(
        IO::OStream& out
) const = 0;

};

}
} // namespaces
#endif
