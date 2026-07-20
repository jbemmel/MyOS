#ifndef IZipUtil_H
#define IZipUtil_H
#include "Core/IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS {
namespace Zip {

INTERFACE( IZipUtil, "afedc4f2-5f5a-11d6-8272-0010a708e02d" )

	virtual myos_result_t unzip(const buffer& zipped, buffer& result) const = 0;
};

}
} // namespaces
#endif
