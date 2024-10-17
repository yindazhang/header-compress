#include "rsvp-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpHeader");

NS_OBJECT_ENSURE_REGISTERED(RsvpHeader);

RsvpHeader::RsvpHeader()
{
    m_type = 0;
    m_ttl = 64;
    m_length = 8;
}

RsvpHeader::~RsvpHeader()
{
}

TypeId
RsvpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpHeader>();
    return tid;
}

TypeId
RsvpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpHeader::GetSerializedSize() const
{
    return m_length;
}

void
RsvpHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(0x10);
    start.WriteU8(m_type);
    start.WriteHtonU16(0);
    start.WriteU8(m_ttl);
    start.WriteU8(0);
    start.WriteHtonU16(m_length);

    for(auto ob = m_objects.begin();ob != m_objects.end();++ob){
        uint16_t length = ob->second->GetSerializedSize() + 4;
        uint8_t cclass = ob->first >> 8;
        uint8_t type = ob->first;

        start.WriteHtonU16(length);
        start.WriteU8(cclass);
        start.WriteU8(type);

        ob->second->Serialize(start);
        start.Next(ob->second->GetSerializedSize());
    }
}

uint32_t
RsvpHeader::Deserialize(Buffer::Iterator start)
{
    start.ReadU8();
    m_type = start.ReadU8();
    start.ReadNtohU16();
    m_ttl = start.ReadU8();
    start.ReadU8();
    m_length = start.ReadNtohU16();

    m_objects.clear();
    uint32_t offset = 8;
    while(offset < m_length)
    {
        uint16_t length = start.ReadNtohU16();
        uint16_t cclass = start.ReadU8();
        uint16_t type = start.ReadU8();
        uint16_t kind = (cclass << 8) | type;

        if(m_objects.find(kind) == m_objects.end())
            std::cout << "Duplicate kind " << kind << " in Deserialize" << std::endl;

        Ptr<RsvpObject> ob = RsvpObject::CreateObject(kind);
        uint32_t objectSize = ob->Deserialize(start);
        start.Next(objectSize);
        if(length != objectSize + 4)
            std::cout << "Length " << length << " does not match objectSize " 
                    << objectSize << std::endl;
        offset += length;
        m_objects[kind] = ob;
    }

    return GetSerializedSize();
}

uint8_t 
RsvpHeader::GetType()
{
    return m_type;
}
    
void
RsvpHeader::SetType(uint8_t type)
{
    m_type = type;
}

uint8_t 
RsvpHeader::GetTtl()
{
    return m_ttl;
}
    
void 
RsvpHeader::SetTtl(uint8_t ttl)
{
    m_ttl = ttl;
}

uint16_t 
RsvpHeader::GetLength()
{
    return m_length;
}

std::map<uint16_t, Ptr<RsvpObject>>&
RsvpHeader::GetObjects()
{
    return m_objects;
}

void 
RsvpHeader::AppendObject(Ptr<RsvpObject> object)
{
    uint16_t kind = object->GetKind();
    if(m_objects.find(kind) != m_objects.end())
        std::cout << "Duplicate kind " << kind << " in AppendObject" << std::endl;
    else
        m_length += (object->GetSerializedSize() + 4);
    m_objects[kind] = object;
}

} // namespace ns3
