#include "rsvp-object.h"
#include "rsvp-filter-spec4.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv4-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpFilterSpec4");

NS_OBJECT_ENSURE_REGISTERED(RsvpFilterSpec4);

RsvpFilterSpec4::RsvpFilterSpec4()
{
	m_id = 0;
}

RsvpFilterSpec4::~RsvpFilterSpec4()
{
}

TypeId
RsvpFilterSpec4::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpFilterSpec4")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpFilterSpec4>();
    return tid;
}

TypeId
RsvpFilterSpec4::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpFilterSpec4::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpFilterSpec4::GetSerializedSize() const
{
    return 8;
}

void
RsvpFilterSpec4::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_address.Get());
    start.WriteHtonU16(0);
    start.WriteHtonU16(m_id);
}

uint32_t
RsvpFilterSpec4::Deserialize(Buffer::Iterator start)
{
    m_address.Set(start.ReadNtohU32());
    start.ReadNtohU16();
    m_id = start.ReadNtohU16();
    return GetSerializedSize();
}

uint16_t 
RsvpFilterSpec4::GetKind() const
{
    return ((uint16_t)RsvpObject::FilterSpec << 8) | 7;
}

Ipv4Address
RsvpFilterSpec4::GetAddress()
{
    return m_address;
}
	
void 
RsvpFilterSpec4::SetAddress(Ipv4Address address)
{
    m_address.Set(address.Get());
}

uint16_t 
RsvpFilterSpec4::GetId()
{
    return m_id;
}

void 
RsvpFilterSpec4::SetId(uint16_t id)
{
    m_id = id;
}

} // namespace ns3
