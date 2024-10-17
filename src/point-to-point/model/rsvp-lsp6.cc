#include "rsvp-object.h"
#include "rsvp-lsp6.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv6-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpLsp6");

NS_OBJECT_ENSURE_REGISTERED(RsvpLsp6);

RsvpLsp6::RsvpLsp6()
{
	m_id = 0;
    memset(m_extend, 0, 16);
}

RsvpLsp6::~RsvpLsp6()
{
}

TypeId
RsvpLsp6::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpLsp6")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpLsp6>();
    return tid;
}

TypeId
RsvpLsp6::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpLsp6::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpLsp6::GetSerializedSize() const
{
    return 36;
}

void
RsvpLsp6::Serialize(Buffer::Iterator start) const
{
    uint8_t buf[16];
    m_address.GetBytes(buf);
    for(uint32_t i = 0;i < 16;++i)
        start.WriteU8(buf[i]);
    start.WriteHtonU16(0);
    start.WriteHtonU16(m_id);
    for(uint32_t i = 0;i < 16;++i)
        start.WriteU8(m_extend[i]);
}

uint32_t
RsvpLsp6::Deserialize(Buffer::Iterator start)
{
    uint8_t buf[16];
    for(uint32_t i = 0;i < 16;++i)
        buf[i] = start.ReadU8();
    m_address.Set(buf);
    start.ReadNtohU16();
    m_id = start.ReadNtohU16();
    for(uint32_t i = 0;i < 16;++i)
        m_extend[i] = start.ReadU8();
    return GetSerializedSize();
}

uint16_t 
RsvpLsp6::GetKind() const
{
    return ((uint16_t)RsvpObject::Session << 8) | 8;
}

Ipv6Address 
RsvpLsp6::GetAddress()
{
    return m_address;
}
	
void 
RsvpLsp6::SetAddress(Ipv6Address address)
{
    m_address = address;
}

uint16_t 
RsvpLsp6::GetId()
{
    return m_id;
}

void 
RsvpLsp6::SetId(uint16_t id)
{
    m_id = id;
}

void 
RsvpLsp6::GetExtend(uint8_t extend[16])
{
    for(uint32_t i = 0;i < 16;++i)
        extend[i] = m_extend[i];
}
	
void 
RsvpLsp6::SetExtend(uint8_t extend[16])
{
    for(uint32_t i = 0;i < 16;++i)
        m_extend[i] = extend[i];
}

} // namespace ns3
