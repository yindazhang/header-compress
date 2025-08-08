#include "pfc-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PfcHeader");

NS_OBJECT_ENSURE_REGISTERED(PfcHeader);

PfcHeader::PfcHeader()
{
    m_opcode = 0;
    m_mask = 0;
    for (int i = 0; i < 4; ++i)
        m_pause[i] = 0;
}

PfcHeader::~PfcHeader()
{
}

TypeId
PfcHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PfcHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<PfcHeader>();
    return tid;
}

TypeId
PfcHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
PfcHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
PfcHeader::GetSerializedSize() const
{
    return 12;
}

void
PfcHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_opcode);
    start.WriteHtonU16(m_mask);
    for (int i = 0; i < 4; ++i)
        start.WriteHtonU16(m_pause[i]);
}

uint32_t
PfcHeader::Deserialize(Buffer::Iterator start)
{
    m_opcode = start.ReadNtohU16();
    m_mask = start.ReadNtohU16();
    for (int i = 0; i < 4; ++i)
        m_pause[i] = start.ReadNtohU16();
    return GetSerializedSize();
}

void
PfcHeader::SetPause(uint8_t id)
{
    m_opcode = 0x0101;
    m_mask |= (1 << id);
    m_pause[id] = 1;
}

void
PfcHeader::SetResume(uint8_t id)
{
    m_opcode = 0x0101;
    m_mask |= (1 << id);
    m_pause[id] = 0;
}

uint16_t
PfcHeader::GetPause(uint8_t id)
{
    return m_pause[id];
}

} // namespace ns3
