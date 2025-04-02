#include "schc-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SchcHeader");

NS_OBJECT_ENSURE_REGISTERED(SchcHeader);

SchcHeader::SchcHeader()
{
}

SchcHeader::~SchcHeader()
{
}

TypeId
SchcHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SchcHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<SchcHeader>();
    return tid;
}

TypeId
SchcHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SchcHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
SchcHeader::GetSerializedSize() const
{
    switch(m_type){
        case SchcType::UpdateCompress4 : return 16;
        case SchcType::UpdateCompress6 : return 40;
        default : std::cout << "Unknown Type" << std::endl; return -1;
    }
    return -1;
}

void
SchcHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_type);
    switch(m_type){
        case SchcType::UpdateCompress4 : 
            start.WriteHtonU16(m_label); 
            start.WriteHtonU32(m_flow4Id.m_srcIP);
            start.WriteHtonU32(m_flow4Id.m_dstIP);
            start.WriteHtonU16(m_flow4Id.m_srcPort);
            start.WriteHtonU16(m_flow4Id.m_dstPort);
            start.WriteU8(m_flow4Id.m_protocol); 
            break;
        case SchcType::UpdateCompress6 : 
            start.WriteHtonU16(m_label); 
            start.WriteHtonU64(m_flow6Id.m_srcIP[0]);
            start.WriteHtonU64(m_flow6Id.m_srcIP[1]);
            start.WriteHtonU64(m_flow6Id.m_dstIP[0]);
            start.WriteHtonU64(m_flow6Id.m_dstIP[1]);
            start.WriteHtonU16(m_flow6Id.m_srcPort);
            start.WriteHtonU16(m_flow6Id.m_dstPort);
            start.WriteU8(m_flow6Id.m_protocol);
            break;
        default : std::cout << "Unknown Type" << std::endl; break;
    }
}

uint32_t
SchcHeader::Deserialize(Buffer::Iterator start)
{
    m_type = start.ReadU8();
    switch(m_type){
        case SchcType::UpdateCompress4 : 
            m_label = start.ReadNtohU16();
            m_flow4Id.m_srcIP  = start.ReadNtohU32();
            m_flow4Id.m_dstIP  = start.ReadNtohU32();
            m_flow4Id.m_srcPort = start.ReadNtohU16();
            m_flow4Id.m_dstPort = start.ReadNtohU16();
            m_flow4Id.m_protocol = start.ReadU8();
            break;
        case SchcType::UpdateCompress6 : 
            m_label = start.ReadNtohU16();
            m_flow6Id.m_srcIP[0]  = start.ReadNtohU64();
            m_flow6Id.m_srcIP[1]  = start.ReadNtohU64();
            m_flow6Id.m_dstIP[0]  = start.ReadNtohU64();
            m_flow6Id.m_dstIP[1]  = start.ReadNtohU64();
            m_flow6Id.m_srcPort = start.ReadNtohU16();
            m_flow6Id.m_dstPort = start.ReadNtohU16();
            m_flow6Id.m_protocol = start.ReadU8();
            break;
        default : std::cout << "Unknown Type" << std::endl; break;
    }

    return GetSerializedSize();
}

uint8_t 
SchcHeader::GetType()
{
    return m_type;
}

void 
SchcHeader::SetType(uint8_t type)
{
    m_type = type;
}

uint16_t 
SchcHeader::GetLabel()
{
    return m_label;
}

void 
SchcHeader::SetLabel(uint16_t label)
{
    m_label = label;
}

FlowV4Id 
SchcHeader::GetFlow4Id()
{
    return m_flow4Id;
}
    
void 
SchcHeader::SetFlow4Id(FlowV4Id flowId)
{
    m_flow4Id = flowId;
}

FlowV6Id 
SchcHeader::GetFlow6Id()
{
    return m_flow6Id;
}
    
void 
SchcHeader::SetFlow6Id(FlowV6Id flowId)
{
    m_flow6Id = flowId;
}

} // namespace ns3
