/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
 
#include "Init/MyOSCoreContainerDefs.h"
#ifndef NE2000_IN_CORE
#error This test needs an ethernet driver
#endif 

#ifndef IP_IN_CORE
#error This test needs the IP subsystem
#endif 
 
#include "Context/IContext.h"
#include "Networking/IP/IIPLayer.h"
#include "Networking/IP/IPPacket.h"
#include "Networking/IP/Sockets/TCP/ITCPServerSocket.h"
#include "Networking/IP/Sockets/TCP/ITCPSocket.h"
#include "Devices/Network/IEthernetDevice.h"
#include "MultiThreading/IThread.h"

#include "Buffering/CBlockOutputStream.h"
#include "Context/IDirectory.h"
#include "HTTPReplyGenerator.h"

#include "debug.h"

namespace MyOS { namespace Tests {

using namespace Devices::Network;
using namespace Networking::IP;
using MultiThreading::Thread;

using IO::CBlockOutputStream;
using Context::IDirectory;

class SocketWriter : public CBlockOutputStream::Observer {

	ITCPSocket &socket;

public:
	SocketWriter( ITCPSocket &s ) : socket(s) {}

	virtual buffer onBytesWritten( const buffer& bytes )
   {
		// GETCOMPONENT(WebServer).log( "packetReady size=%u", packet.getSize() );
		socket.send( bytes, 0 );
		return bytes;
	};
};

bool doTest( IContext& context, NVPAIR params[] )
{
   #define IP ipadr_t(192,168,2,103)
   #define MASK ipadr_t(255,255,255,0)

	// Look for root dir
	IInterface* root;
	if (context.lookup(myos_name_t("/"),root) != E_MYOS_SUCCESS) {
		return 01;	
	}

	static class WebServer : public ITCPSocketClient
	{
		IDirectory& root;
	public:	
		WebServer( IDirectory& c ) : root(c) {}

	   virtual void onRecv( ITCPSocket& onSocket, const buffer& data ) 
	   {
			DPRINTK( "\nWebServer::onRecv data=%x", data.getData() );
			//for (u32 c=0; c<data.getSize(); ++c) {
			//	DPRINTK( "%c", data.getData()[c] );	
			//}
			char *rq = (char*) data.getData();	// modified
			if (strncmp(rq,"GET ",4)==0) {
				char *endOfURL = strchr(rq+=4,' ');
				if (endOfURL) {
					*--rq = '.';			// make it start with .
					*endOfURL = '\0';
					DPRINTK( "\nRequested URL: %s", rq );

					// For now, a trick to fool browsers, not sure if this ".xml" is really required
					if ( char *xml = strrchr(rq,'.')) {
						if (strcmp(xml,".xml")==0) strcpy( xml, "/idl" );
					}

					const char* MYOSWEBSERVERREPLY =
						"HTTP/1.0 200 OK\r\n"
						"Server: MyOSWebServer\r\n"
						"Content-type: text/html\r\n"
				// "Accept-Ranges: bytes\r\n" // this would mean the server supports partial rq
				// "Content-length: 46\r\n"		// assumption: count starts after \r\n below, includes closing
				// The above seems to imply binary contents!
						"\r\n"
						"<html><body>Hello, MyOSBrowser</body></html>\r\n";

					// XX too big -> crash!
					u8 replybuffer[ 120 ]; // XX on.getMaxTxSegSize()
					buffer msg( (u8*) replybuffer, sizeof(replybuffer) );
					SocketWriter sw( onSocket );
					CBlockOutputStream ostream( sizeof(replybuffer), msg, sw );

					ostream.printf( MYOSWEBSERVERREPLY );
					// HTTPReplyGenerator generator;
					// generator.generateReply( &root, rq, ostream );
					ostream.flush();
				} else DPRINTK( "\nInvalid HTTP request, no URL end" );
   			}
			onSocket.close();	// let the client close the connection?
	   }	   
   } webserver( * root->castTo<IDirectory>() );
		
   // By putting this here, I make sure it is initialized!
   static class AcceptHandler : public ITCPServerSocketClient
   {
	public:
		inline void operator delete( void* m ) {
			DPRINTK( "\nAcceptHandler::delete(%x)", m );
		}	
		virtual ~AcceptHandler() {
			DPRINTK( "\n~AcceptHandler" );
		}   		
   	
      virtual bool onConnection( ITCPServerSocket &on, const sockadr_t& from )
      {
      	DPRINTK( "\nonConnection from=%x:%d", from.getIP().host(), from.getPort() );
      	
		ITCPSocket* result;
      	on.accept( result, &webserver, 0 /* wait indefinitely */ );
      	return true;         
      }

      // optional
	   virtual void onClose( ITCPServerSocket& onSocket ) 
	   {
	      
	   }
	} accepter;
      
	PRINTK( "\nWebServer test running" );
	IInterface *i1, *i2;
   
	if (context.lookup( IIPLayer::uuid(), i1 )==E_MYOS_SUCCESS) {
		IIPLayer* iplayer = i1->castTo<IIPLayer>();
   
		if (context.lookup( myos_name_t("/dev/eth0"), i2 )==E_MYOS_SUCCESS) {
			IEthernetDevice* eth = i2->castTo<IEthernetDevice>();
			IIPEndpoint* ipEndpoint = iplayer->addBinding( *eth, IP, MASK );
			if (ipEndpoint) {
				DPRINTK( "\nIP binding added" );
				
				ITCPServerSocket* websocket = ipEndpoint->createTCPServerSocket( 80, &accepter );
				if (websocket) {
					websocket->listen(5);
					DPRINTK( "\nSocket opened for listening to HTTP connections..." );
				}
			}
		}
	}
	return true;  
}

}}
