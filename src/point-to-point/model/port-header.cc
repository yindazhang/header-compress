#include "port-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PortHeader");

NS_OBJECT_ENSURE_REGISTERED(PortHeader);

PortHeader::PortHeader()
{
}

PortHeader::PortHeader(const PortHeader& header)
{
    m_sourcePort = header.GetSourcePort();
    m_destinationPort = header.GetDestinationPort();
}

PortHeader::~PortHeader()
{
}

TypeId
PortHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PortHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<PortHeader>();
    return tid;
}

TypeId
PortHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
PortHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
PortHeader::GetSerializedSize() const
{
    return 4;
}

void
PortHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_sourcePort);
    start.WriteHtonU16(m_destinationPort);
}

uint32_t
PortHeader::Deserialize(Buffer::Iterator start)
{
    m_sourcePort = start.ReadNtohU16();
    m_destinationPort = start.ReadNtohU16();

    return GetSerializedSize();
}

uint16_t 
PortHeader::GetSourcePort() const
{
    return m_sourcePort;
}

void 
PortHeader::SetSourcePort(uint16_t port)
{
    m_sourcePort = port;
}

uint16_t 
PortHeader::GetDestinationPort() const
{
    return m_destinationPort;
}
    
void 
PortHeader::SetDestinationPort(uint16_t port)
{
    m_destinationPort = port;
}

} // namespace ns3
