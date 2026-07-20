/***************************************************************************
                          PCNet32CIMPL.h  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef DEC21041COMPONENT_H
#define DEC21041COMPONENT_H

#include "Component.h"
#include "Display.h"
#include "Core/PCISupport.h"
#include "Core/KernelMemory.h"
#include "Core/Timing.h"

#include "Dec21041Driver.h"
class I_InterfaceRepository;

namespace Dec21041 {

using Display::I_Display;
using Timing::I_Timing;

/**
 * 	This component provides support for Dec21041 based PCI network cards
 */
class DEC21041 : public MyOS::Drivers::CDriverBase
{
    DEC21041(MyOS::Core::IContainer& container) throw() INITSECTION;
    
    virtual myos_result_t initialize(Context::IContext& context,
                NVPAIR params[]) INITSECTION;

    virtual myos_result_t deinitialize(Context::IContext& context);

    // CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

	Dec21041Driver driver;
};

};	// namespace

#endif
