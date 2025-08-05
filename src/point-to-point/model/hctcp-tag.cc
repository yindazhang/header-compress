#include "hctcp-tag.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HcTcpTag");

NS_OBJECT_ENSURE_REGISTERED(HcTcpTag);


TypeId
HcTcpTag::GetTypeId()
{
    static TypeId tid = TypeId("HcTcpTag")
                            .SetParent<Tag>()
                            .AddConstructor<HcTcpTag>();
    return tid;
}

TypeId
HcTcpTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
HcTcpTag::GetSerializedSize() const
{
    return m_header.GetSerializedSize();
}

void
HcTcpTag::Serialize(TagBuffer i) const
{
    i.WriteU32(m_header.GetSequenceNumber().GetValue());
    i.WriteU32(m_header.GetAckNumber().GetValue());
    i.WriteU16(m_header.GetLength() << 12 | m_header.GetFlags());
    i.WriteU16(m_header.GetWindowSize());
}

void
HcTcpTag::Deserialize(TagBuffer i)
{
    m_header.SetSequenceNumber(i.ReadU32());
    m_header.SetAckNumber(i.ReadU32());
    uint16_t field = i.ReadU16();
    m_header.SetFlags(field & 0xff);
    m_header.SetLength(field >> 12);
    m_header.SetWindowSize(i.ReadU16());
}

void
HcTcpTag::SetHeader(const HcTcpHeader& header)
{
    m_header = header;
}

HcTcpHeader
HcTcpTag::GetHeader()
{
    return m_header;
}

void
HcTcpTag::Print(std::ostream& os) const
{
    return;
}

} // namespace ns3

