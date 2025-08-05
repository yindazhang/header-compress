#include "ipv4-tag.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4Tag");

NS_OBJECT_ENSURE_REGISTERED(Ipv4Tag);


TypeId
Ipv4Tag::GetTypeId()
{
    static TypeId tid = TypeId("Ipv4Tag")
                            .SetParent<Tag>()
                            .AddConstructor<Ipv4Tag>();
    return tid;
}

TypeId
Ipv4Tag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Ipv4Tag::GetSerializedSize() const
{
    return m_header.GetSerializedSize() + m_portHeader.GetSerializedSize();
}

void
Ipv4Tag::Serialize(TagBuffer i) const
{
    i.WriteU8(0);
    i.WriteU8(m_header.GetEcn());
    i.WriteU16(m_header.GetPayloadSize() + 20);
    i.WriteU16(m_header.GetIdentification());
    i.WriteU16(0);
    i.WriteU8(m_header.GetTtl());
    i.WriteU8(m_header.GetProtocol());
    i.WriteU16(0);
    i.WriteU32(m_header.GetSource().Get());
    i.WriteU32(m_header.GetDestination().Get());
    i.WriteU16(m_portHeader.GetSourcePort());
    i.WriteU16(m_portHeader.GetDestinationPort());
}

void
Ipv4Tag::Deserialize(TagBuffer i)
{
    i.ReadU8();
    m_header.SetEcn(Ipv4Header::EcnType(i.ReadU8()));
    m_header.SetPayloadSize(i.ReadU16() - 20);
    m_header.SetIdentification(i.ReadU16());
    i.ReadU16();
    m_header.SetTtl(i.ReadU8());
    m_header.SetProtocol(i.ReadU8());
    i.ReadU16();
    m_header.SetSource(Ipv4Address(i.ReadU32()));
    m_header.SetDestination(Ipv4Address(i.ReadU32()));
    m_portHeader.SetSourcePort(i.ReadU16());
    m_portHeader.SetDestinationPort(i.ReadU16());
}

void
Ipv4Tag::SetHeader(Ipv4Header header, PortHeader portHeader)
{
    m_header = header;
    m_portHeader = portHeader;
}

void
Ipv4Tag::GetHeader(Ipv4Header& header, PortHeader& portHeader) const
{
    header = m_header;
    portHeader = m_portHeader;
}

void
Ipv4Tag::Print(std::ostream& os) const
{
    return;
}

} // namespace ns3

