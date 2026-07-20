#ifndef IDisplay_H
#define IDisplay_H

#include "Devices/ICharDevice.h"

namespace MyOS {

namespace IO { class OStream; class IStream; }

namespace Devices {
namespace Display {

class IDisplay;

// namespace Default {

SUB_INTERFACE( IDisplay, ICharDevice, "abe1beba-0457-4375-8a3e-48809a0d5838" )

	virtual IO::OStream& cout() = 0;

	virtual myos_result_t clear() = 0;

	virtual myos_result_t print(IO::IStream& in) = 0;

};


}}} // namespaces

#endif
