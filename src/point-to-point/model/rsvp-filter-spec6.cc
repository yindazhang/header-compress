#include "rsvp-object.h"
#include "rsvp-filter-spec6.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv6-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpFilterSpec6");

NS_OBJECT_ENSURE_REGISTERED(RsvpFilterSpec6);

RsvpFilterSpec6::RsvpFilterSpec6()
{
	m_id = 0;
}

RsvpFilterSpec6::~RsvpFilterSpec6()
{
}

TypeId
RsvpFilterSpec6::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpFilterSpec6")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpFilterSpec6>();
    return tid;
}

TypeId
RsvpFilterSpec6::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpFilterSpec6::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpFilterSpec6::GetSerializedSize() const
{
    return 20;
}

void
RsvpFilterSpec6::Serialize(Buffer::Iterator start) const
{
    uint8_t buf[16];
    m_address.GetBytes(buf);
    for(uint32_t i = 0;i < 8;++i)
        start.WriteU8(buf[i]);
    
    start.WriteHtonU16(0);
    start.WriteHtonU16(m_id);
}

uint32_t
RsvpFilterSpec6::Deserialize(Buffer::Iterator start)
{
    uint8_t buf[16];
    for(uint32_t i = 0;i < 8;++i)
        buf[i] = start.ReadU8();
    m_address.Set(buf);

    start.ReadNtohU16();
    m_id = start.ReadNtohU16();
    return GetSerializedSize();
}

uint16_t 
RsvpFilterSpec6::GetKind() const
{
    return ((uint16_t)RsvpObject::FilterSpec << 8) | 8;
}

Ipv6Address
RsvpFilterSpec6::GetAddress()
{
    return m_address;
}
	
void 
RsvpFilterSpec6::SetAddress(Ipv6Address address)
{
    m_address = address;
}

uint16_t 
RsvpFilterSpec6::GetId()
{
    return m_id;
}

void 
RsvpFilterSpec6::SetId(uint16_t id)
{
    m_id = id;
}

} // namespace ns3
