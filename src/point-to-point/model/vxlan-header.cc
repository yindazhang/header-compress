#include "vxlan-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VxlanHeader");

NS_OBJECT_ENSURE_REGISTERED(VxlanHeader);

VxlanHeader::VxlanHeader()
{
    m_flag = 0;
    m_vni = 0;
}

VxlanHeader::~VxlanHeader()
{
}

TypeId
VxlanHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::VxlanHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<VxlanHeader>();
    return tid;
}

TypeId
VxlanHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
VxlanHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
VxlanHeader::GetSerializedSize() const
{
    return 8;
}

void
VxlanHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_flag);
    start.WriteHtonU32(m_vni);
}

uint32_t
VxlanHeader::Deserialize(Buffer::Iterator start)
{
    m_flag = start.ReadNtohU32();
    m_vni = start.ReadNtohU32();
    return GetSerializedSize();
}

uint8_t 
VxlanHeader::GetFlag()
{
    return m_flag;
}

void 
VxlanHeader::SetFlag(uint8_t flag)
{
    m_flag = flag;
}

uint32_t 
VxlanHeader::GetVni()
{
    return m_vni;
}
    
void 
VxlanHeader::SetVni(uint32_t vni)
{
    m_vni = vni;
}

} // namespace ns3
