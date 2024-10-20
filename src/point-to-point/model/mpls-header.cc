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
    start.WriteHtonU32(m_value);
}

uint32_t
MplsHeader::Deserialize(Buffer::Iterator start)
{
    m_value = start.ReadNtohU32();
    return GetSerializedSize();
}

uint32_t 
MplsHeader::GetLabel()
{
    return m_value >> 12;
}
    
void
MplsHeader::SetLabel(uint32_t label)
{
    m_value = (m_value & 0x00000fff) | (label << 12);
}

uint8_t 
MplsHeader::GetExp()
{
    return (m_value >> 9) & 0x7;
}

void 
MplsHeader::SetExp(uint8_t exp)
{
    m_value = (m_value & 0xfffff1ff) | (uint16_t(exp) << 9);
}

uint8_t 
MplsHeader::GetTtl()
{
    return m_value;
}
    
void 
MplsHeader::SetTtl(uint8_t ttl)
{
    m_value = (m_value & 0xffffff00) | ttl;
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
    m_value = m_value & 0xfffffeff;
}

} // namespace ns3
