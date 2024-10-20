#include "compress-ipv4-header.h"
#include "rsvp-object.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CompressIpv4Header");

NS_OBJECT_ENSURE_REGISTERED(CompressIpv4Header);

CompressIpv4Header::CompressIpv4Header()
{
}

CompressIpv4Header::~CompressIpv4Header()
{
}

TypeId
CompressIpv4Header::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CompressIpv4Header")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<CompressIpv4Header>();
    return tid;
}

TypeId
CompressIpv4Header::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CompressIpv4Header::Print(std::ostream& os) const
{
    return;
}

uint32_t
CompressIpv4Header::GetSerializedSize() const
{
    return 4;
}

void
CompressIpv4Header::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_payloadSize);
    start.WriteHtonU16(m_identification);
}

uint32_t
CompressIpv4Header::Deserialize(Buffer::Iterator start)
{
    m_payloadSize = start.ReadNtohU16();
    m_identification = start.ReadNtohU16();
    return GetSerializedSize();
}

Ipv4Header 
CompressIpv4Header::GetIpv4Header()
{
    Ipv4Header header;
    header.SetPayloadSize(m_payloadSize);
    header.SetIdentification(m_identification);
    return header;
}
    
void 
CompressIpv4Header::SetIpv4Header(Ipv4Header header)
{
    m_payloadSize = header.GetPayloadSize();
    m_identification = header.GetIdentification();
}

} // namespace ns3
