#include "compress-ipv6-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CompressIpv6Header");

NS_OBJECT_ENSURE_REGISTERED(CompressIpv6Header);

CompressIpv6Header::CompressIpv6Header()
{
}

CompressIpv6Header::~CompressIpv6Header()
{
}

TypeId
CompressIpv6Header::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CompressIpv6Header")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<CompressIpv6Header>();
    return tid;
}

TypeId
CompressIpv6Header::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CompressIpv6Header::Print(std::ostream& os) const
{
    return;
}

uint32_t
CompressIpv6Header::GetSerializedSize() const
{
    return 6;
}

void
CompressIpv6Header::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_payloadSize);
    start.WriteHtonU32(m_identification);
}

uint32_t
CompressIpv6Header::Deserialize(Buffer::Iterator start)
{
    m_payloadSize = start.ReadNtohU16();
    m_identification = start.ReadNtohU32();
    return GetSerializedSize();
}

Ipv6Header 
CompressIpv6Header::GetIpv6Header()
{
    Ipv6Header header;
    header.SetPayloadLength(m_payloadSize);
    header.SetTrafficClass(m_identification >> 20);
    header.SetFlowLabel(m_identification & 0xfffff);
    return header;
}

void 
CompressIpv6Header::SetIpv6Header(Ipv6Header header)
{
    m_payloadSize = header.GetPayloadLength();
    uint32_t traffic = header.GetTrafficClass();
    uint32_t label = header.GetFlowLabel();
    m_identification = ((traffic << 20) | label);
}

} // namespace ns3
