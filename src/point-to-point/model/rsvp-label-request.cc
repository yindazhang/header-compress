#include "rsvp-object.h"
#include "rsvp-label-request.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpLabelRequest");

NS_OBJECT_ENSURE_REGISTERED(RsvpLabelRequest);

RsvpLabelRequest::RsvpLabelRequest()
{
    m_id = 0;
}

RsvpLabelRequest::~RsvpLabelRequest()
{
}

TypeId
RsvpLabelRequest::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpLabelRequest")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpLabelRequest>();
    return tid;
}

TypeId
RsvpLabelRequest::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpLabelRequest::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpLabelRequest::GetSerializedSize() const
{
    return 4;
}

void
RsvpLabelRequest::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(0);
    start.WriteHtonU16(m_id);
}

uint32_t
RsvpLabelRequest::Deserialize(Buffer::Iterator start)
{
    start.ReadNtohU16();
    m_id = start.ReadNtohU16();
    return GetSerializedSize();
}

uint16_t 
RsvpLabelRequest::GetKind() const
{
    return ((uint16_t)RsvpObject::LabelRequest << 8) | 1;
}

uint16_t 
RsvpLabelRequest::GetId()
{
    return m_id;
}

void 
RsvpLabelRequest::SetId(uint16_t id)
{
    m_id = id;
}

} // namespace ns3
