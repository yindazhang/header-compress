#include "rsvp-object.h"
#include "rsvp-style.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpStyle");

NS_OBJECT_ENSURE_REGISTERED(RsvpStyle);

RsvpStyle::RsvpStyle()
{
    m_option = 0;
}

RsvpStyle::~RsvpStyle()
{
}

TypeId
RsvpStyle::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpStyle")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpStyle>();
    return tid;
}

TypeId
RsvpStyle::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpStyle::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpStyle::GetSerializedSize() const
{
    return 4;
}

void
RsvpStyle::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_option);
}

uint32_t
RsvpStyle::Deserialize(Buffer::Iterator start)
{
    m_option = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpStyle::GetKind() const
{
    return ((uint16_t)RsvpObject::Style << 8) | 1;
}

uint32_t 
RsvpStyle::GetOption()
{
    return m_option;
}

void 
RsvpStyle::SetOption(uint32_t option)
{
    m_option = option;
}

} // namespace ns3