/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
 
#include "Init/MyOSCoreContainerDefs.h"

/*
#ifndef NE2000_IN_CORE
#error This test needs an ethernet driver
#endif 

#ifndef IP_IN_CORE
#error This test needs the IP subsystem
#endif
*/ 
 
#include "Context/IContext.h"
#include "Networking/IP/IIPLayer.h"
#include "Networking/IP/IPPacket.h"
#include "Networking/IP/Sockets/IRawSocket.h"
#include "Devices/Network/IEthernetDevice.h"
#include "MultiThreading/IThread.h"
#include "ICMP.h"

#include "debug.h"

namespace MyOS { namespace Tests {

using namespace Devices::Network;
using namespace Networking::IP;
using MultiThreading::Thread;

bool doTest( Context::IContext& context, NVPAIR params[] )
{
   #define IP ipadr_t(192,168,1,103)
   #define MASK ipadr_t(255,255,255,0)
		
   // By putting this here, I make sure it is initialized!
   static class ICMPHandler : public IRawSocketClient
   {
		/**
		 * Called when data is received on the socket
		 */
		virtual void onRecv( IRawSocket& onSocket, ipadr_t from, 
			IPPacket& p ) {

		   DPRINTK( "\nGot ICMP packet from %x", from.host() );
		
		   // Simulate what happens on exception
		   // Processor::generateInvalidOpcode();
		
		   // only reply to first fragment
		   if (p.isFirstFragment()) {
		      ICMPHeader& h = p.getData<ICMPHeader>();
		      if (h.typecode==HTONS16(ICMPHeader::E_PING)) {
		         // *(GETCOMPONENT( ICMPService ).display()) << "\nGot a PING!";
		
		         // change typecode from 8 to 0, recalculate checksum (carry!)
		         h.typecode = (ICMPHeader::E_TYPE_CODE) HTONS16(ICMPHeader::E_PING_REPLY);
		         h.checksum += h.checksum >= (0xFFFF - 8) ? 9 : 8;
		
		         DPRINTK( "...sending reply" );
		         myos_result_t r = onSocket.sendPacketTo( p.getSource(), p, p.getPayloadSize(), E_SEND_PACKET_REUSED );
		         ASSERTIONv( r==E_MYOS_SUCCESS, E_ERROR, r );
		      } else {
		         DPRINTK( "\nGot some other ICMP packet: %d", (u16) h.typecode );
		      }
		   } else {
				DPRINTK( "\nICMP: Not first fragment.." );
		      /*
		      u8* hdr = (u8*) packet;
		      for (u32 i=0; i<20; ++i) {
		         ICMPService::getInstance().display()->cout().printf( " %X", hdr[i] );
		      }
		      */
		   }

		}

		// optional ones:
		// virtual void onSocketClosing( IRawSocket& onSocket ) {}
		// virtual void onSocketError( IRawSocket& onSocket, u32 errorcode ) {}   	
   } icmphandler;

	PRINTK( "\nPingServer test running" );
   
    IIPLayer& iplayer = context.lookup( myos_name_t(IIPLayer::ID())).castToExcept<IIPLayer>();
    IEthernetDevice& eth = context.lookup( myos_name_t("/dev/eth0")).castToExcept<IEthernetDevice>();
	
	IIPEndpoint* ipEndpoint = iplayer.addBinding( eth, IP, MASK );
	if (ipEndpoint) {
		DPRINTK( "\nIP binding added" );
		
		IRawSocket* socket = ipEndpoint->createRawSocket( E_IPPROTO_ICMP, &icmphandler );
		if (socket) {
			socket->open(0);
			DPRINTK( "\nSocket opened for receiving ICMP packets..." );
			
			// XX cannot let this method return, else PageFaults ??
			// while (1) Thread::do_yield(0);
		}
	}
	return true;  
}

}}
