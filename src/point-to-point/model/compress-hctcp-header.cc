#include "compress-hctcp-header.h"
#include "hctcp-header.h"

#include "ns3/tcp-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CompressHcTcpHeader");

NS_OBJECT_ENSURE_REGISTERED(CompressHcTcpHeader);

CompressHcTcpHeader::CompressHcTcpHeader()
{
}

CompressHcTcpHeader::~CompressHcTcpHeader()
{
}

TypeId
CompressHcTcpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CompressHcTcpHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<CompressHcTcpHeader>();
    return tid;
}

TypeId
CompressHcTcpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CompressHcTcpHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
CompressHcTcpHeader::GetSerializedSize() const
{
    uint32_t ret = 1;
    if(m_diff & 1) ret += 4;
    if(m_diff & 2) ret += 4;
    if(m_diff & 4) ret += 1;
    if(m_diff & 8) ret += 1;
    if(m_diff & 16) ret += 2;
    return ret;
}

void
CompressHcTcpHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_diff);
    if(m_diff & 1) start.WriteHtonU32(m_sequenceNumber.GetValue());
    if(m_diff & 2) start.WriteHtonU32(m_ackNumber.GetValue());
    if(m_diff & 4) start.WriteU8(m_length);
    if(m_diff & 8) start.WriteU8(m_flags);
    if(m_diff & 16) start.WriteHtonU16(m_windowSize);
}

uint32_t
CompressHcTcpHeader::Deserialize(Buffer::Iterator start)
{
    m_diff = start.ReadU8();
    if(m_diff & 1) m_sequenceNumber = start.ReadNtohU32();
    if(m_diff & 2) m_ackNumber = start.ReadNtohU32();
    if(m_diff & 4) m_length = start.ReadU8();
    if(m_diff & 8) m_flags = start.ReadU8();
    if(m_diff & 16) m_windowSize = start.ReadNtohU16();
    return GetSerializedSize();
}

void 
CompressHcTcpHeader::SetHeader(const HcTcpHeader& a, const HcTcpHeader& b)
{
    m_diff = 0;
    if(a.GetSequenceNumber() != b.GetSequenceNumber()){
        m_diff |= 1;
        m_sequenceNumber = b.GetSequenceNumber();
    }
    if(a.GetAckNumber() != b.GetAckNumber()){
        m_diff |= 2;
        m_ackNumber = b.GetAckNumber();
    }
    if(a.GetLength() != b.GetLength()){
        m_diff |= 4;
        m_length = b.GetLength();
    }
    if(a.GetFlags() != b.GetFlags()){
        m_diff |= 8;
        m_flags = b.GetFlags();
    }
    if(a.GetWindowSize() != b.GetWindowSize()){
        m_diff |= 16;
        m_windowSize = b.GetWindowSize();
    }
}
    
HcTcpHeader 
CompressHcTcpHeader::GetHeader(const HcTcpHeader& a)
{
    HcTcpHeader ret;

    if(m_diff & 1) ret.SetSequenceNumber(m_sequenceNumber.GetValue());
    else ret.SetSequenceNumber(a.GetSequenceNumber().GetValue());

    if(m_diff & 2) ret.SetAckNumber(m_ackNumber.GetValue());
    else ret.SetAckNumber(a.GetAckNumber().GetValue());

    if(m_diff & 4) ret.SetLength(m_length);
    else ret.SetLength(a.GetLength());

    if(m_diff & 8) ret.SetFlags(m_flags);
    else ret.SetFlags(a.GetFlags());

    if(m_diff & 16) ret.SetWindowSize(m_windowSize);
    else ret.SetWindowSize(a.GetWindowSize());

    return ret;
}

} // namespace ns3
