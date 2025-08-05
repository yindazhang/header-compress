#include "rohc-ip-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RohcIpHeader");

NS_OBJECT_ENSURE_REGISTERED(RohcIpHeader);

RohcIpHeader::RohcIpHeader()
{
}

RohcIpHeader::~RohcIpHeader()
{
}

TypeId
RohcIpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RohcIpHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RohcIpHeader>();
    return tid;
}

TypeId
RohcIpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RohcIpHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
RohcIpHeader::GetSerializedSize() const
{
    return 4;
}

void
RohcIpHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_ttl);
    start.WriteU8(m_ecn);
    start.WriteHtonU16(m_payloadSize);
}

uint32_t
RohcIpHeader::Deserialize(Buffer::Iterator start)
{
    m_ttl = start.ReadU8();
    m_ecn = start.ReadU8();
    m_payloadSize = start.ReadNtohU16();
    return GetSerializedSize();
}

Ipv4Header 
RohcIpHeader::GetIpv4Header()
{
    Ipv4Header header;
    header.SetTtl(m_ttl);
    header.SetEcn(static_cast<Ipv4Header::EcnType>(m_ecn));
    header.SetPayloadSize(m_payloadSize);
    return header;
}

void 
RohcIpHeader::GetIpv4Header(Ipv4Header& header)
{
    header.SetTtl(m_ttl);
    header.SetEcn(static_cast<Ipv4Header::EcnType>(m_ecn));
    header.SetPayloadSize(m_payloadSize);
}
    
void 
RohcIpHeader::SetIpv4Header(Ipv4Header header)
{
    m_ttl = header.GetTtl();
    m_ecn = static_cast<uint8_t>(header.GetEcn());
    m_payloadSize = header.GetPayloadSize();
}

Ipv6Header 
RohcIpHeader::GetIpv6Header()
{
    Ipv6Header header;
    header.SetHopLimit(m_ttl);
    header.SetEcn(static_cast<Ipv6Header::EcnType>(m_ecn));
    header.SetPayloadLength(m_payloadSize);
    return header;
}

void 
RohcIpHeader::GetIpv6Header(Ipv6Header& header)
{
    header.SetHopLimit(m_ttl);
    header.SetEcn(static_cast<Ipv6Header::EcnType>(m_ecn));
    header.SetPayloadLength(m_payloadSize);
}

void 
RohcIpHeader::SetIpv6Header(Ipv6Header header)
{
    m_ttl = header.GetHopLimit();
    m_ecn = static_cast<uint8_t>(header.GetEcn());
    m_payloadSize = header.GetPayloadLength();
}

} // namespace ns3
