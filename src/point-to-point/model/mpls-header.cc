#include "mpls-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MplsHeader");

NS_OBJECT_ENSURE_REGISTERED(MplsHeader);

MplsHeader::MplsHeader()
{
    m_label = 0;
    m_value = 0;
}

MplsHeader::~MplsHeader()
{
}

TypeId
MplsHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MplsHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<MplsHeader>();
    return tid;
}

TypeId
MplsHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MplsHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
MplsHeader::GetSerializedSize() const
{
    return 4;
}

void
MplsHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_label);
    start.WriteHtonU16(m_value);
}

uint32_t
MplsHeader::Deserialize(Buffer::Iterator start)
{
    m_label = start.ReadNtohU16();
    m_value = start.ReadNtohU16();
    return GetSerializedSize();
}

uint16_t 
MplsHeader::GetLabel()
{
    return m_label;
}
    
void
MplsHeader::SetLabel(uint16_t label)
{
    m_label = label;
}

uint8_t 
MplsHeader::GetType()
{
    return (m_value >> 12) & 0xf;
}

void 
MplsHeader::SetType(uint8_t type)
{
    m_value = (m_value & 0xfff) | (uint16_t(type) << 12);
}

uint8_t 
MplsHeader::GetExp()
{
    return (m_value >> 9) & 0x7;
}

void 
MplsHeader::SetExp(uint8_t exp)
{
    m_value = (m_value & 0xf1ff) | (uint16_t(exp) << 9);
}

uint8_t 
MplsHeader::GetTtl()
{
    return m_value;
}
    
void 
MplsHeader::SetTtl(uint8_t ttl)
{
    m_value = (m_value & 0xff00) | ttl;
}

uint8_t 
MplsHeader::GetBos()
{
    return (m_value >> 8) & 0x1;
}
    
void 
MplsHeader::SetBos()
{
    m_value = m_value | 0x100;
}
    
void 
MplsHeader::ClearBos()
{
    m_value = m_value & 0xfeff;
}

} // namespace ns3
