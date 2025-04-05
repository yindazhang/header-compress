#include "hctcp-header.h"
#include "ns3/tcp-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HcTcpHeader");

NS_OBJECT_ENSURE_REGISTERED(HcTcpHeader);

HcTcpHeader::HcTcpHeader()
{
}

HcTcpHeader::~HcTcpHeader()
{
}

TypeId
HcTcpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HcTcpHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<HcTcpHeader>();
    return tid;
}

TypeId
HcTcpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
HcTcpHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
HcTcpHeader::GetSerializedSize() const
{
    return 16;
}

void
HcTcpHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_sourcePort);
    start.WriteHtonU16(m_destinationPort);
    start.WriteHtonU32(m_sequenceNumber.GetValue());
    start.WriteHtonU32(m_ackNumber.GetValue());
    start.WriteHtonU16(GetLength() << 12 | m_flags);
    start.WriteHtonU16(m_windowSize);
}

uint32_t
HcTcpHeader::Deserialize(Buffer::Iterator start)
{
    m_sourcePort = start.ReadNtohU16();
    m_destinationPort = start.ReadNtohU16();
    m_sequenceNumber = start.ReadNtohU32();
    m_ackNumber = start.ReadNtohU32();
    uint16_t field = start.ReadNtohU16();
    m_flags = field & 0xff;
    m_length = field >> 12;
    m_windowSize = start.ReadNtohU16();
    return GetSerializedSize();
}

uint16_t 
HcTcpHeader::GetSourcePort() const
{
    return m_sourcePort;
}
    
void 
HcTcpHeader::SetSourcePort(uint16_t port)
{
    m_sourcePort = port;
}

uint16_t 
HcTcpHeader::GetDestinationPort() const
{
    return m_destinationPort;
}
    
void 
HcTcpHeader::SetDestinationPort(uint16_t port)
{
    m_destinationPort = port;
}

SequenceNumber32 
HcTcpHeader::GetSequenceNumber() const
{
    return m_sequenceNumber;
}
    
void 
HcTcpHeader::SetSequenceNumber(uint32_t sequenceNumber)
{
    m_sequenceNumber = sequenceNumber;
}

SequenceNumber32 
HcTcpHeader::GetAckNumber() const
{
    return m_ackNumber;
}
    
void 
HcTcpHeader::SetAckNumber(uint32_t ackNumber)
{
    m_ackNumber = ackNumber;
}

uint16_t 
HcTcpHeader::GetLength() const
{
    return m_length;
}
    
void 
HcTcpHeader::SetLength(uint8_t length)
{
    m_length = length;
}

uint8_t 
HcTcpHeader::GetFlags() const
{
    return m_flags;
}
    
void 
HcTcpHeader::SetFlags(uint8_t flags)
{
    m_flags = flags;
}

uint16_t 
HcTcpHeader::GetWindowSize() const
{
    return m_windowSize;
}
    
void 
HcTcpHeader::SetWindowSize(uint16_t windowSize)
{
    m_windowSize = windowSize;
}

} // namespace ns3
