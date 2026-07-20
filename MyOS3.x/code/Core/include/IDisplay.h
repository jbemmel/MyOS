#ifndef IDisplay_H
#define IDisplay_H

#include "IInterface.h"

namespace MyOS {

namespace IO { class OStream; class IStream; }

namespace Devices {
namespace Display {

INTERFACE( IDisplay, "abe1beba-0457-4375-8a3e-48809a0d5838" )

	virtual IO::OStream& cout() = 0;

	virtual myos_result_t clear() = 0;

	virtual myos_result_t print(IO::IStream& in) = 0;

};

}}} // namespaces

#endif
