#include "rohc-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RohcHeader");

NS_OBJECT_ENSURE_REGISTERED(RohcHeader);

RohcHeader::RohcHeader()
{
    m_type = 0;
    m_profile = 0;
    m_cid = 0;
}

RohcHeader::~RohcHeader()
{
}

TypeId
RohcHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RohcHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RohcHeader>();
    return tid;
}

TypeId
RohcHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RohcHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
RohcHeader::GetSerializedSize() const
{
    if(m_type == 0)
    {
        return 3; 
    }
    else
    {
        return 4;
    }
}

void
RohcHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_type);
    if (m_type == 0)
    {
        start.WriteHtonU16(m_cid);
    }
    else
    {
        start.WriteU8(m_profile);
        start.WriteHtonU16(m_cid);
    }
}

uint32_t
RohcHeader::Deserialize(Buffer::Iterator start)
{
    m_type = start.ReadU8();
    if(m_type == 0)
    {
        m_cid = start.ReadNtohU16();
    }
    else
    {
        m_profile = start.ReadU8();
        m_cid = start.ReadNtohU16();
    }
    return GetSerializedSize();
}

uint8_t 
RohcHeader::GetType()
{
    return m_type;
}

void 
RohcHeader::SetType(uint8_t type)
{
    m_type = type; 
}

uint8_t
RohcHeader::GetProfile()
{
    return m_profile;
}

void 
RohcHeader::SetProfile(uint8_t profile)
{
    m_profile = profile;
}

uint16_t
RohcHeader::GetCid()
{
    return m_cid;
}

void
RohcHeader::SetCid(uint16_t cid)
{
    m_cid = cid;
}

} // namespace ns3
