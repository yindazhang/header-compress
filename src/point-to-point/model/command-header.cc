#include "command-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CommandHeader");

NS_OBJECT_ENSURE_REGISTERED(CommandHeader);

CommandHeader::CommandHeader()
{
}

CommandHeader::~CommandHeader()
{
}

TypeId
CommandHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CommandHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<CommandHeader>();
    return tid;
}

TypeId
CommandHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CommandHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
CommandHeader::GetSerializedSize() const
{
    switch(m_type){
        case CmdType::SwitchUpdate : return 10;
        case CmdType::NICDeleteCompress4 :
        case CmdType::NICData4 : return 18;
        case CmdType::NICDeleteCompress6 :
        case CmdType::NICData6 : return 42;
        case CmdType::NICUpdateCompress4 : 
        case CmdType::NICUpdateDecompress4 : return 20;
        case CmdType::NICUpdateCompress6 : 
        case CmdType::NICUpdateDecompress6 : return 44;
        default : std::cout << "Unknown Type" << std::endl; return -1;
    }
    return -1;
}

void
CommandHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_srcId);
    start.WriteHtonU16(m_dstId);
    start.WriteU8(m_type);
    switch(m_type){
        case CmdType::SwitchUpdate :
            start.WriteHtonU16(m_label); 
            start.WriteHtonU16(m_newLabel);
            start.WriteU8(m_port);
            break;
        case CmdType::NICDeleteCompress4 : 
        case CmdType::NICData4 :
            start.WriteHtonU32(m_flow4Id.m_srcIP);
            start.WriteHtonU32(m_flow4Id.m_dstIP);
            start.WriteHtonU16(m_flow4Id.m_srcPort);
            start.WriteHtonU16(m_flow4Id.m_dstPort);
            start.WriteU8(m_flow4Id.m_protocol); 
            break;
        case CmdType::NICDeleteCompress6 : 
        case CmdType::NICData6 :
            start.WriteHtonU64(m_flow6Id.m_srcIP[0]);
            start.WriteHtonU64(m_flow6Id.m_srcIP[1]);
            start.WriteHtonU64(m_flow6Id.m_dstIP[0]);
            start.WriteHtonU64(m_flow6Id.m_dstIP[1]);
            start.WriteHtonU16(m_flow6Id.m_srcPort);
            start.WriteHtonU16(m_flow6Id.m_dstPort);
            start.WriteU8(m_flow6Id.m_protocol);
            break;
        case CmdType::NICUpdateCompress4 : 
        case CmdType::NICUpdateDecompress4 :
            start.WriteHtonU16(m_label); 
            start.WriteHtonU32(m_flow4Id.m_srcIP);
            start.WriteHtonU32(m_flow4Id.m_dstIP);
            start.WriteHtonU16(m_flow4Id.m_srcPort);
            start.WriteHtonU16(m_flow4Id.m_dstPort);
            start.WriteU8(m_flow4Id.m_protocol); 
            break;
        case CmdType::NICUpdateCompress6 : 
        case CmdType::NICUpdateDecompress6 : 
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
CommandHeader::Deserialize(Buffer::Iterator start)
{
    m_srcId = start.ReadNtohU16();
    m_dstId = start.ReadNtohU16();
    m_type = start.ReadU8();
    switch(m_type){
        case CmdType::SwitchUpdate :
            m_label = start.ReadNtohU16();
            m_newLabel = start.ReadNtohU16();
            m_port = start.ReadU8();
            break;
        case CmdType::NICDeleteCompress4 :
        case CmdType::NICData4 :
            m_flow4Id.m_srcIP  = start.ReadNtohU32();
            m_flow4Id.m_dstIP  = start.ReadNtohU32();
            m_flow4Id.m_srcPort = start.ReadNtohU16();
            m_flow4Id.m_dstPort = start.ReadNtohU16();
            m_flow4Id.m_protocol = start.ReadU8();
            break;
        case CmdType::NICDeleteCompress6 :
        case CmdType::NICData6 :
            m_flow6Id.m_srcIP[0]  = start.ReadNtohU64();
            m_flow6Id.m_srcIP[1]  = start.ReadNtohU64();
            m_flow6Id.m_dstIP[0]  = start.ReadNtohU64();
            m_flow6Id.m_dstIP[1]  = start.ReadNtohU64();
            m_flow6Id.m_srcPort = start.ReadNtohU16();
            m_flow6Id.m_dstPort = start.ReadNtohU16();
            m_flow6Id.m_protocol = start.ReadU8();
            break;
        case CmdType::NICUpdateCompress4 : 
        case CmdType::NICUpdateDecompress4 :
            m_label = start.ReadNtohU16();
            m_flow4Id.m_srcIP  = start.ReadNtohU32();
            m_flow4Id.m_dstIP  = start.ReadNtohU32();
            m_flow4Id.m_srcPort = start.ReadNtohU16();
            m_flow4Id.m_dstPort = start.ReadNtohU16();
            m_flow4Id.m_protocol = start.ReadU8();
            break;
        case CmdType::NICUpdateCompress6 : 
        case CmdType::NICUpdateDecompress6 : 
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

uint16_t 
CommandHeader::GetSourceId()
{
    return m_srcId;
}
    
void 
CommandHeader::SetSourceId(uint16_t id)
{
    m_srcId = id;
}

uint16_t 
CommandHeader::GetDestinationId()
{
    return m_dstId;
}

void 
CommandHeader::SetDestinationId(uint16_t id)
{
    m_dstId = id;
}

uint8_t 
CommandHeader::GetType()
{
    return m_type;
}

void 
CommandHeader::SetType(uint8_t type)
{
    m_type = type;
}

uint16_t 
CommandHeader::GetLabel()
{
    return m_label;
}

void 
CommandHeader::SetLabel(uint16_t label)
{
    m_label = label;
}

uint16_t 
CommandHeader::GetNewLabel()
{
    return m_newLabel;
}

void 
CommandHeader::SetNewLabel(uint16_t label)
{
    m_newLabel = label;
}

uint8_t 
CommandHeader::GetPort()
{
    return m_port;
}

void 
CommandHeader::SetPort(uint8_t port)
{
    m_port = port;
}


FlowV4Id 
CommandHeader::GetFlow4Id()
{
    return m_flow4Id;
}
    
void 
CommandHeader::SetFlow4Id(FlowV4Id flowId)
{
    m_flow4Id = flowId;
}

FlowV6Id 
CommandHeader::GetFlow6Id()
{
    return m_flow6Id;
}
    
void 
CommandHeader::SetFlow6Id(FlowV6Id flowId)
{
    m_flow6Id = flowId;
}

} // namespace ns3
