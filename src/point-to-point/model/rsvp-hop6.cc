#include "rsvp-object.h"
#include "rsvp-hop6.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv6-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpHop6");

NS_OBJECT_ENSURE_REGISTERED(RsvpHop6);

RsvpHop6::RsvpHop6()
{
	m_interface = 0;
}

RsvpHop6::~RsvpHop6()
{
}

TypeId
RsvpHop6::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpHop6")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpHop6>();
    return tid;
}

TypeId
RsvpHop6::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpHop6::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpHop6::GetSerializedSize() const
{
    return 20;
}

void
RsvpHop6::Serialize(Buffer::Iterator start) const
{
    uint8_t buf[16];
    m_address.GetBytes(buf);
    for(uint32_t i = 0;i < 8;++i)
        start.WriteU8(buf[i]);

    start.WriteHtonU32(m_interface);
}

uint32_t
RsvpHop6::Deserialize(Buffer::Iterator start)
{
    uint8_t buf[16];
    for(uint32_t i = 0;i < 8;++i)
        buf[i] = start.ReadU8();
    m_address.Set(buf);

    m_interface = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpHop6::GetKind() const
{
    return ((uint16_t)RsvpObject::Hop << 8) | 2;
}

Ipv6Address
RsvpHop6::GetAddress()
{
    return m_address;
}
	
void 
RsvpHop6::SetAddress(Ipv6Address address)
{
    m_address = address;
}

uint32_t 
RsvpHop6::GetInterface()
{
    return m_interface;
}

void 
RsvpHop6::SetInterface(uint32_t interface)
{
    m_interface = interface;
}

} // namespace ns3