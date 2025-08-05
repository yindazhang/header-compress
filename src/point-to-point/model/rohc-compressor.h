#ifndef ROHC_COMPRESSOR_H
#define ROHC_COMPRESSOR_H

#include "ns3/object.h"
#include "ns3/packet.h"

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "hctcp-header.h"

namespace ns3
{

struct RohcContext
{
	int64_t updateTimeNs;
	FlowV4Id flowV4Id;
	FlowV6Id flowV6Id;
	HcTcpHeader hcTcpHeader;
};

class RohcCompressor : public Object
{
	public:
		static TypeId GetTypeId();

    	RohcCompressor();
		~RohcCompressor();

		uint16_t Process(Ptr<Packet> packet, uint16_t protocol);

	private:
		std::vector<RohcContext> m_contextList;
		const uint16_t m_maxContext = 16384;
};

} // namespace ns3

#endif /* ROHC_COMPRESSOR */