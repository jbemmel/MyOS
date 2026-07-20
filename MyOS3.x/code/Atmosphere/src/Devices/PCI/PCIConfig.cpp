#include "PCIConfig.h"

/**
 * Earlier GCC versions did not need this, but 3.4 does
 */
 
namespace MyOS {
namespace Drivers {   
namespace PCI {

struct PCI_DEFINITION_VARIABLE PCICONFIG_VAR;
struct PCI_DEFINITION_STATIC PCICONFIG_FIXED;

}}}
