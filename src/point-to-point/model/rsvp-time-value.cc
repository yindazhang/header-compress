#include "rsvp-object.h"
#include "rsvp-time-value.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpTimeValue");

NS_OBJECT_ENSURE_REGISTERED(RsvpTimeValue);

RsvpTimeValue::RsvpTimeValue()
{
    m_period = 0;
}

RsvpTimeValue::~RsvpTimeValue()
{
}

TypeId
RsvpTimeValue::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpTimeValue")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpTimeValue>();
    return tid;
}

TypeId
RsvpTimeValue::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpTimeValue::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpTimeValue::GetSerializedSize() const
{
    return 4;
}

void
RsvpTimeValue::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_period);
}

uint32_t
RsvpTimeValue::Deserialize(Buffer::Iterator start)
{
    m_period = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpTimeValue::GetKind() const
{
    return ((uint16_t)RsvpObject::TimeValue << 8) | 1;
}

uint32_t 
RsvpTimeValue::GetPeriod()
{
    return m_period;
}

void 
RsvpTimeValue::SetPeriod(uint32_t period)
{
    m_period = period;
}

} // namespace ns3