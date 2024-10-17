#include "rsvp-object.h"
#include "rsvp-error-spec6.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv6-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpErrorSpec6");

NS_OBJECT_ENSURE_REGISTERED(RsvpErrorSpec6);

RsvpErrorSpec6::RsvpErrorSpec6()
{
    m_flag = 0;
	m_code = 0;
	m_value = 0;
}

RsvpErrorSpec6::~RsvpErrorSpec6()
{
}

TypeId
RsvpErrorSpec6::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpErrorSpec6")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpErrorSpec6>();
    return tid;
}

TypeId
RsvpErrorSpec6::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpErrorSpec6::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpErrorSpec6::GetSerializedSize() const
{
    return 20;
}

void
RsvpErrorSpec6::Serialize(Buffer::Iterator start) const
{
    uint8_t buf[16];
    m_address.GetBytes(buf);
    for(uint32_t i = 0;i < 8;++i)
        start.WriteU8(buf[i]);
    
    start.WriteU8(m_flag);
    start.WriteU8(m_code);
    start.WriteHtonU16(m_value);
}

uint32_t
RsvpErrorSpec6::Deserialize(Buffer::Iterator start)
{
    uint8_t buf[16];
    for(uint32_t i = 0;i < 8;++i)
        buf[i] = start.ReadU8();
    m_address.Set(buf);

    m_flag = start.ReadU8();
    m_code = start.ReadU8();
    m_value = start.ReadNtohU16();
    return GetSerializedSize();
}


uint16_t 
RsvpErrorSpec6::GetKind() const
{
    return ((uint16_t)RsvpObject::ErrorSpec << 8) | 2;
}

Ipv6Address
RsvpErrorSpec6::GetAddress()
{
    return m_address;
}
	
void 
RsvpErrorSpec6::SetAddress(Ipv6Address address)
{
    m_address = address;
}

uint8_t 
RsvpErrorSpec6::GetFlag()
{
    return m_flag;
}

void 
RsvpErrorSpec6::SetFlag(uint8_t flag)
{
    m_flag = flag;
}

uint8_t 
RsvpErrorSpec6::GetCode()
{
    return m_code;
}
	
void 
RsvpErrorSpec6::SetCode(uint8_t code)
{
    m_code = code;
}

uint16_t 
RsvpErrorSpec6::GetValue()
{
    return m_value;
}

void 
RsvpErrorSpec6::SetValue(uint16_t value)
{
    m_value = value;
}

} // namespace ns3

