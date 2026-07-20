#ifndef ARPDISCOVERY_H
#define ARPDISCOVERY_H

#include "Timer/timetypes.h"
#include "../ActiveGatherBuffer.h"
#include "Networking/Ethernet/EthernetPacket.h"
#include "ARPCache.h"

namespace MyOS { namespace Networking { namespace ARP {

using Timer::timer_t;
using ASync::ActiveGatherBuffer;
using namespace Ethernet;
	
/// Represents an ongoing ARP discovery
class ARPDiscovery : public Timer::ITimerTarget
{
public:
	ARPDiscovery( ASync::ActiveGatherBuffer& tx, EthernetPacket& arprq, ENTRY& entry );
	void operator delete( void* );

	void start();
	void end();		// called when reply is received, sends enqueued packets
	void abort();	// Called when interrupted, should also delete the entry

	bool enqueue( EthernetPacket& p, u32 flags, size_t payload ) {
		for (int n=0; n<4; ++n) {
			if (!pending[n]) {			// TODO: Atomic CmpXchg(p)
				pending[n] = &p;
				
				u32 *u = (u32*) &p;	// keep flags within packet
				u[0] = flags;
				u[1] = payload;
				return true;
			}	
		}
		DPRINTK( "\nenqueue failed" );
		return false;
	}
	
private:
	virtual void onTimer();

	EthernetPacket& request;		// original ARP request, for retransmission
	ENTRY& entry;					// Entry, for timeout
	ASync::ActiveGatherBuffer& tx;	/// Channel to send on
	timer_t timer;					/// Timeout timer
	EthernetPacket* pending[4];		/// Pending packets, waiting for the resolve
									/// For now a fixed maximum

	u8 retries;
	bool aborted;
};

}}}	// namespace

#endif
