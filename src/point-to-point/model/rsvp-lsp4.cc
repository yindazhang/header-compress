#include "rsvp-object.h"
#include "rsvp-lsp4.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv4-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpLsp4");

NS_OBJECT_ENSURE_REGISTERED(RsvpLsp4);

RsvpLsp4::RsvpLsp4()
{
	m_id = 0;
    m_extend = 0;
}

RsvpLsp4::~RsvpLsp4()
{
}

TypeId
RsvpLsp4::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpLsp4")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpLsp4>();
    return tid;
}

TypeId
RsvpLsp4::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpLsp4::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpLsp4::GetSerializedSize() const
{
    return 12;
}

void
RsvpLsp4::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_address.Get());
    start.WriteHtonU16(0);
    start.WriteHtonU16(m_id);
    start.WriteHtonU32(m_extend);
}

uint32_t
RsvpLsp4::Deserialize(Buffer::Iterator start)
{
    m_address.Set(start.ReadNtohU32());
    start.ReadNtohU16();
    m_id = start.ReadNtohU16();
    m_extend = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpLsp4::GetKind() const
{
    return ((uint16_t)RsvpObject::Session << 8) | 7;
}

Ipv4Address
RsvpLsp4::GetAddress()
{
    return m_address;
}
	
void 
RsvpLsp4::SetAddress(Ipv4Address address)
{
    m_address.Set(address.Get());
}

uint16_t 
RsvpLsp4::GetId()
{
    return m_id;
}

void 
RsvpLsp4::SetId(uint16_t id)
{
    m_id = id;
}

uint32_t 
RsvpLsp4::GetExtend()
{
    return m_extend;
}
	
void 
RsvpLsp4::SetExtend(uint32_t extend)
{
    m_extend = extend;
}

} // namespace ns3
