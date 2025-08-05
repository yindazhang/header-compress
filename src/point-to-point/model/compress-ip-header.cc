#include "compress-ip-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CompressIpHeader");

NS_OBJECT_ENSURE_REGISTERED(CompressIpHeader);

CompressIpHeader::CompressIpHeader()
{
}

CompressIpHeader::~CompressIpHeader()
{
}

TypeId
CompressIpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CompressIpHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<CompressIpHeader>();
    return tid;
}

TypeId
CompressIpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CompressIpHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
CompressIpHeader::GetSerializedSize() const
{
    return 2;
}

void
CompressIpHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_payloadSize);
}

uint32_t
CompressIpHeader::Deserialize(Buffer::Iterator start)
{
    m_payloadSize = start.ReadNtohU16();
    return GetSerializedSize();
}

Ipv4Header 
CompressIpHeader::GetIpv4Header()
{
    Ipv4Header header;
    header.SetPayloadSize(m_payloadSize);
    return header;
}
    
void 
CompressIpHeader::SetIpv4Header(Ipv4Header header)
{
    m_payloadSize = header.GetPayloadSize();
}

Ipv6Header 
CompressIpHeader::GetIpv6Header()
{
    Ipv6Header header;
    header.SetPayloadLength(m_payloadSize);
    return header;
}

void 
CompressIpHeader::SetIpv6Header(Ipv6Header header)
{
    m_payloadSize = header.GetPayloadLength();
}

} // namespace ns3
