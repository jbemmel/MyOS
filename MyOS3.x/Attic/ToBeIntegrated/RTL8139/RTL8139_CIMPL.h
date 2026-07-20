/***************************************************************************
                          RTL8139CIMPL.h  -  description
                             -------------------
    begin                : Sun Aug 19 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#ifndef RTL8139CIMPL_H
#define RTL8139CIMPL_H

#include "Component.h"
#include "InterfaceRepository.h"

/// implementation headers
#include "RTL8139_PCIDriver.h"

#include "Display.h"
using Display::I_Display;
#define PRINTM(x) *GETCOMPONENT( RTL8139C ).display() << x

// need the headers, because of inline implementation of release() in REQUIRES_INTERFACE
#include "Display.h"
#include "Core/PCISupport.h"
#include "Core/KernelMemory.h"
#include "Core/Timing.h"

namespace RTL8139 {

using Timing::I_Timing;

/**
 *  Support for the RealTek 8139C chip
 *
 *  Exports interfaces for:
 *  - Sending ethernet packets      (I_Ethernet)
 *  - Managing the network card     (I_PCIDriver)
 */
COMPONENT( RTL8139C )

    RTL8139C() INITSECTION;
		virtual ~RTL8139C();

private:
    /// Management: Initialize the component, called after succesful authentication
    virtual u32 initialize( I_InterfaceRepository &m ) INITSECTION;
    virtual void deinitialize( I_InterfaceRepository &m );
    /// Destruction, cannot use destructor because that is implemented in macro COMPONENT
    virtual void destruct();
    virtual I_Interface* getInterface( u32 i ) const;

		/// Exported functions
	  virtual s32 put( const char* callarguments, const Buffer &buf, bool cancel /*=false*/ );
		virtual const Buffer getWriteBuffer( const char *forcall, size_t bytes, u32 block );
    virtual bool hotswap( const Buffer &withComponent );

/// Implementation

    REQUIRED_INTERFACE<I_PCISupport> pcisupport;
    REQUIRED_INTERFACE<I_KernelMemory> kernelmem;
		REQUIRED_INTERFACE<I_Timing> timing;
		REQUIRED_INTERFACE<I_Display> display;

    // keep a copy of this, so that PCIDevice and Ethernet can register
    I_InterfaceRepository *repository;

    friend class C_RTL8139_PCIDriver;
    friend class C_RTL8139_PCIDevice;
    friend class C_RTL8139_Ethernet;

    /// The I_PCIDriver implementation
    C_RTL8139_PCIDriver pcidriver;
};

};  // namespace

#endif

