#ifndef ROHC_DECOMPRESSOR_H
#define ROHC_DECOMPRESSOR_H

#include "ns3/object.h"
#include "ns3/packet.h"

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "port-header.h"
#include "hctcp-header.h"

namespace ns3
{

struct RohcContent
{
	uint8_t profile;
	Ipv4Header ipv4Header;
	Ipv6Header ipv6Header;
	PortHeader portHeader;
	HcTcpHeader hcTcpHeader;
};

class RohcDecompressor : public Object
{
	public:
		static TypeId GetTypeId();

    	RohcDecompressor();
		~RohcDecompressor();

		uint16_t Process(Ptr<Packet> packet);

	private:
		std::vector<RohcContent> m_contentList;
		const uint16_t m_maxContent = 16384;
};

} // namespace ns3

#endif /* ROHC_DECOMPRESSOR */