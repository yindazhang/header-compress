#include "port-tag.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PortTag");

NS_OBJECT_ENSURE_REGISTERED(PortTag);


TypeId
PortTag::GetTypeId()
{
    static TypeId tid = TypeId("PortTag")
                            .SetParent<Tag>()
                            .AddConstructor<PortTag>();
    return tid;
}

TypeId
PortTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
PortTag::GetSerializedSize() const
{
    return m_header.GetSerializedSize();
}

void
PortTag::Serialize(TagBuffer i) const
{
    i.WriteU16(m_header.GetSourcePort());
    i.WriteU16(m_header.GetDestinationPort());
}

void
PortTag::Deserialize(TagBuffer i)
{
    m_header.SetSourcePort(i.ReadU16());
    m_header.SetDestinationPort(i.ReadU16());
}

void
PortTag::SetHeader(const PortHeader& header)
{
    m_header = header;
}

PortHeader
PortTag::GetHeader()
{
    return m_header;
}

void
PortTag::Print(std::ostream& os) const
{
    return;
}

} // namespace ns3