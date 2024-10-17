#include "rsvp-object.h"
#include "rsvp-hop4.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv4-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpHop4");

NS_OBJECT_ENSURE_REGISTERED(RsvpHop4);

RsvpHop4::RsvpHop4()
{
	m_interface = 0;
}

RsvpHop4::~RsvpHop4()
{
}

TypeId
RsvpHop4::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpHop4")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpHop4>();
    return tid;
}

TypeId
RsvpHop4::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpHop4::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpHop4::GetSerializedSize() const
{
    return 8;
}

void
RsvpHop4::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_address.Get());
    start.WriteHtonU32(m_interface);
}

uint32_t
RsvpHop4::Deserialize(Buffer::Iterator start)
{
    m_address.Set(start.ReadNtohU32());
    m_interface = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpHop4::GetKind() const
{
    return ((uint16_t)RsvpObject::Hop << 8) | 1;
}

Ipv4Address
RsvpHop4::GetAddress()
{
    return m_address;
}
	
void 
RsvpHop4::SetAddress(Ipv4Address address)
{
    m_address.Set(address.Get());
}

uint32_t 
RsvpHop4::GetInterface()
{
    return m_interface;
}

void 
RsvpHop4::SetInterface(uint32_t interface)
{
    m_interface = interface;
}

} // namespace ns3
