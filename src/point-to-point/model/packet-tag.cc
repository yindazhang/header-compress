#include "packet-tag.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/tag.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketTag");

NS_OBJECT_ENSURE_REGISTERED(PacketTag);


TypeId
PacketTag::GetTypeId()
{
    static TypeId tid = TypeId("PacketTag")
                            .SetParent<Tag>()
                            .AddConstructor<PacketTag>();
    return tid;
}

TypeId
PacketTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
PacketTag::GetSerializedSize() const
{
    return 4;
}

void
PacketTag::Serialize(TagBuffer i) const
{
    i.WriteU32(m_size);
    i.WriteU64(reinterpret_cast<uint64_t>(GetPointer(m_device)));
}

void
PacketTag::Deserialize(TagBuffer i)
{
    m_size = i.ReadU32();
    m_device = (NetDevice*)(i.ReadU64());
}

void 
PacketTag::SetSize(uint32_t size)
{
    m_size = size;
}

uint32_t 
PacketTag::GetSize()
{
    return m_size;
}

void 
PacketTag::SetNetDevice(Ptr<NetDevice> device)
{
    m_device = device;
}

Ptr<NetDevice> 
PacketTag::GetNetDevice()
{
    return m_device;
}

void
PacketTag::Print(std::ostream& os) const
{
    return;
}

} // namespace ns3

