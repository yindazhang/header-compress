#include "rsvp-object.h"
#include "rsvp-error-spec4.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "ns3/ipv4-address.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpErrorSpec4");

NS_OBJECT_ENSURE_REGISTERED(RsvpErrorSpec4);

RsvpErrorSpec4::RsvpErrorSpec4()
{
    m_flag = 0;
	m_code = 0;
	m_value = 0;
}

RsvpErrorSpec4::~RsvpErrorSpec4()
{
}

TypeId
RsvpErrorSpec4::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpErrorSpec4")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpErrorSpec4>();
    return tid;
}

TypeId
RsvpErrorSpec4::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpErrorSpec4::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpErrorSpec4::GetSerializedSize() const
{
    return 8;
}

void
RsvpErrorSpec4::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_address.Get());
    start.WriteU8(m_flag);
    start.WriteU8(m_code);
    start.WriteHtonU16(m_value);
}

uint32_t
RsvpErrorSpec4::Deserialize(Buffer::Iterator start)
{
    m_address.Set(start.ReadNtohU32());
    m_flag = start.ReadU8();
    m_code = start.ReadU8();
    m_value = start.ReadNtohU16();
    return GetSerializedSize();
}

uint16_t 
RsvpErrorSpec4::GetKind() const
{
    return ((uint16_t)RsvpObject::ErrorSpec << 8) | 1;
}

Ipv4Address
RsvpErrorSpec4::GetAddress()
{
    return m_address;
}
	
void 
RsvpErrorSpec4::SetAddress(Ipv4Address address)
{
    m_address.Set(address.Get());
}

uint8_t 
RsvpErrorSpec4::GetFlag()
{
    return m_flag;
}

void 
RsvpErrorSpec4::SetFlag(uint8_t flag)
{
    m_flag = flag;
}

uint8_t 
RsvpErrorSpec4::GetCode()
{
    return m_code;
}
	
void 
RsvpErrorSpec4::SetCode(uint8_t code)
{
    m_code = code;
}

uint16_t 
RsvpErrorSpec4::GetValue()
{
    return m_value;
}

void 
RsvpErrorSpec4::SetValue(uint16_t value)
{
    m_value = value;
}

} // namespace ns3
