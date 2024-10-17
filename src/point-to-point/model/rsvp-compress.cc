#include "rsvp-compress.h"
#include "rsvp-object.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpCompress");

NS_OBJECT_ENSURE_REGISTERED(RsvpCompress);

RsvpCompress::RsvpCompress()
{
}

RsvpCompress::~RsvpCompress()
{
}

TypeId
RsvpCompress::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpCompress")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpCompress>();
    return tid;
}

TypeId
RsvpCompress::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpCompress::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpCompress::GetSerializedSize() const
{
    //TODO
    return 0;
}

void
RsvpCompress::Serialize(Buffer::Iterator start) const
{
    //TODO
}

uint32_t
RsvpCompress::Deserialize(Buffer::Iterator start)
{
    //TODO
    return GetSerializedSize();
}

uint16_t 
RsvpCompress::GetKind() const
{
    return ((uint16_t)RsvpObject::Compress << 8) | 1;
}


} // namespace ns3
