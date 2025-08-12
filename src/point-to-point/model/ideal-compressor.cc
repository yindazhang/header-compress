#include "ideal-compressor.h"

#include "ns3/simulator.h"

#include "port-header.h"

#include "ipv4-tag.h"
#include "ipv6-tag.h"
#include "hctcp-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("IdealCompressor");

NS_OBJECT_ENSURE_REGISTERED(IdealCompressor);

TypeId
IdealCompressor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::IdealCompressor")
                            .SetParent<Object>()
                            .SetGroupName("PointToPoint");
    return tid;
}

IdealCompressor::IdealCompressor()
{
}

IdealCompressor::~IdealCompressor()
{
}

uint16_t 
IdealCompressor::Process(Ptr<Packet> packet, Ipv4Header header)
{
    PortHeader port_header;
    packet->RemoveHeader(port_header);
    if(header.GetProtocol() == 6)
        AddHcTcpTag(packet);
    Ipv4Tag ipv4Tag;
    ipv4Tag.SetHeader(header, port_header);
    packet->ReplacePacketTag(ipv4Tag);
    return 0x0171;
}
	
uint16_t 
IdealCompressor::Process(Ptr<Packet> packet, Ipv6Header header)
{
    PortHeader port_header;
    packet->RemoveHeader(port_header);
    if(header.GetNextHeader() == Ipv6Header::IPV6_TCP)
        AddHcTcpTag(packet);
    Ipv6Tag ipv6Tag;
    ipv6Tag.SetHeader(header, port_header);
    packet->ReplacePacketTag(ipv6Tag);
    return 0x0171;
}

void
IdealCompressor::AddHcTcpTag(Ptr<Packet> packet)
{
    HcTcpHeader tcpHeader;
    packet->RemoveHeader(tcpHeader);

    HcTcpTag tcpTag;
    tcpTag.SetHeader(tcpHeader);
    packet->ReplacePacketTag(tcpTag);
}

} // namespace ns3