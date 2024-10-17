#include "rsvp-object.h"
#include "rsvp-label.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpLabel");

NS_OBJECT_ENSURE_REGISTERED(RsvpLabel);

RsvpLabel::RsvpLabel()
{
    m_label = 0;
}

RsvpLabel::~RsvpLabel()
{
}

TypeId
RsvpLabel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpLabel")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpLabel>();
    return tid;
}

TypeId
RsvpLabel::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpLabel::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpLabel::GetSerializedSize() const
{
    return 4;
}

void
RsvpLabel::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_label);
}

uint32_t
RsvpLabel::Deserialize(Buffer::Iterator start)
{
    m_label = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpLabel::GetKind() const
{
    return ((uint16_t)RsvpObject::Label << 8) | 1;
}

uint32_t 
RsvpLabel::GetLabel()
{
    return m_label;
}

void 
RsvpLabel::SetLabel(uint32_t label)
{
    m_label = label;
}

} // namespace ns3
