#ifndef IBase64_H
#define IBase64_H

#include "Core/IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS
{
namespace Decoding
{
namespace Base64
{
INTERFACE( IBase64, "fe310684-a446-45a7-9e38-d7556e966166" )

    virtual myos_result_t decode( IO::IStream &in ) = 0;

};

}
}
} // namespaces
#endif
