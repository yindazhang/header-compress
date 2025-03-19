#include "ipv6-tag.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6Tag");

NS_OBJECT_ENSURE_REGISTERED(Ipv6Tag);


TypeId
Ipv6Tag::GetTypeId()
{
    static TypeId tid = TypeId("Ipv6Tag")
                            .SetParent<Tag>()
                            .AddConstructor<Ipv6Tag>();
    return tid;
}

TypeId
Ipv6Tag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Ipv6Tag::GetSerializedSize() const
{
    return m_header.GetSerializedSize();
}

void
Ipv6Tag::Serialize(TagBuffer i) const
{
    uint32_t vTcFl = (6 << 28) | (uint32_t(m_header.GetTrafficClass()) << 20) | (uint32_t)(m_header.GetFlowLabel());
    i.WriteU32(vTcFl);
    i.WriteU16(m_header.GetPayloadLength());
    i.WriteU8(m_header.GetNextHeader());
    i.WriteU8(m_header.GetHopLimit());
    uint8_t buf[16];
    m_header.GetSourceAddress().GetBytes(buf);
    i.Write(buf, 16);
    m_header.GetDestinationAddress().GetBytes(buf);
    i.Write(buf, 16);
}

void
Ipv6Tag::Deserialize(TagBuffer i)
{
    uint32_t vTcFl = i.ReadU32 ();
    m_header.SetTrafficClass((vTcFl >> 20) & 0x000000ff);
    m_header.SetFlowLabel(vTcFl & 0xfffff);
    m_header.SetPayloadLength(i.ReadU16());
    m_header.SetNextHeader(i.ReadU8());
    m_header.SetHopLimit(i.ReadU8());
    uint8_t buf[16];
    i.Read(buf, 16);
    m_header.SetSourceAddress(Ipv6Address(buf));
    i.Read(buf, 16);
    m_header.SetDestinationAddress(Ipv6Address(buf));
}

void
Ipv6Tag::SetHeader(Ipv6Header header)
{
    m_header = header;
}

Ipv6Header
Ipv6Tag::GetHeader() const
{
    return m_header;
}

void
Ipv6Tag::Print(std::ostream& os) const
{
    return;
}

} // namespace ns3