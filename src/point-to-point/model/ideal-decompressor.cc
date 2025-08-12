#include "ideal-decompressor.h"

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "port-header.h"

#include "ipv4-tag.h"
#include "ipv6-tag.h"
#include "hctcp-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("IdealDecompressor ");

NS_OBJECT_ENSURE_REGISTERED(IdealDecompressor );

TypeId
IdealDecompressor ::GetTypeId()
{
    static TypeId tid = TypeId("ns3::IdealDecompressor ")
                            .SetParent<Object>()
                            .SetGroupName("PointToPoint");
    return tid;
}

IdealDecompressor ::IdealDecompressor ()
{
}

IdealDecompressor ::~IdealDecompressor ()
{
}

uint16_t 
IdealDecompressor::Process(Ptr<Packet> packet)
{
    HcTcpTag tcpTag;
    if (packet->PeekPacketTag(tcpTag)){
        HcTcpHeader tcpHeader = tcpTag.GetHeader();
        packet->AddHeader(tcpHeader);
    }

    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;
    PortHeader port_header;

    Ipv4Tag ipv4Tag;
    Ipv6Tag ipv6Tag;
    if (packet->PeekPacketTag(ipv4Tag)){
        ipv4Tag.GetHeader(ipv4_header, port_header);
        packet->AddHeader(port_header);
        packet->AddHeader(ipv4_header);
        return 0x0800;
    }
    else if(packet->PeekPacketTag(ipv6Tag)){
        ipv6Tag.GetHeader(ipv6_header, port_header);
        packet->AddHeader(port_header);
        packet->AddHeader(ipv6_header);
        return 0x86DD;
    }
    else {
        std::cerr << "No IPv4 or IPv6 tag found in packet." << std::endl;
        return 0;
    }
}

} // namespace ns3