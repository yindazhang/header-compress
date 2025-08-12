#ifndef IDEAL_COMPRESSOR_H
#define IDEAL_COMPRESSOR_H

#include "ns3/object.h"
#include "ns3/packet.h"

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

namespace ns3
{

class IdealCompressor : public Object
{
	public:
		static TypeId GetTypeId();

    	IdealCompressor();
		~IdealCompressor();

		uint16_t Process(Ptr<Packet> packet, Ipv4Header header);
		uint16_t Process(Ptr<Packet> packet, Ipv6Header header);

	private:
		void AddHcTcpTag(Ptr<Packet> packet);
};

} // namespace ns3

#endif /* IDEAL_COMPRESSOR */