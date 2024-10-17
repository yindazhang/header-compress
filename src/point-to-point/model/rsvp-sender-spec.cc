#include "rsvp-object.h"
#include "rsvp-sender-spec.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpSenderSpec");

NS_OBJECT_ENSURE_REGISTERED(RsvpSenderSpec);

RsvpSenderSpec::RsvpSenderSpec()
{
    m_tokenRate = 0;
    m_tokenSize = 0;
    m_peakRate = 0;
    m_minUnit = 0;
    m_maxSize = 0;
}

RsvpSenderSpec::~RsvpSenderSpec()
{
}

TypeId
RsvpSenderSpec::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpSenderSpec")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpSenderSpec>();
    return tid;
}

TypeId
RsvpSenderSpec::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpSenderSpec::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpSenderSpec::GetSerializedSize() const
{
    return 32;
}

void
RsvpSenderSpec::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(0);
    start.WriteHtonU16(7);
    start.WriteU8(1);
    start.WriteU8(0);
    start.WriteHtonU16(6);
    start.WriteU8(127);
    start.WriteU8(0);
    start.WriteHtonU16(5);
    start.WriteHtonU32(*reinterpret_cast<const uint32_t*>(&m_tokenRate));
    start.WriteHtonU32(*reinterpret_cast<const uint32_t*>(&m_tokenSize));
    start.WriteHtonU32(*reinterpret_cast<const uint32_t*>(&m_peakRate));
    start.WriteHtonU32(m_minUnit);
    start.WriteHtonU32(m_maxSize);
}

uint32_t
RsvpSenderSpec::Deserialize(Buffer::Iterator start)
{
    start.ReadNtohU32();
    start.ReadNtohU32();
    start.ReadNtohU32();

    uint32_t number;
    number = start.ReadNtohU32();
    m_tokenRate = *reinterpret_cast<float*>(&number);
    number = start.ReadNtohU32();
    m_tokenSize = *reinterpret_cast<float*>(&number);
    number = start.ReadNtohU32();
    m_peakRate = *reinterpret_cast<float*>(&number);

    m_minUnit = start.ReadNtohU32();
    m_maxSize = start.ReadNtohU32();
    return GetSerializedSize();
}

uint16_t 
RsvpSenderSpec::GetKind() const
{
    return ((uint16_t)RsvpObject::FlowSpec << 8) | 2;
}

float 
RsvpSenderSpec::GetTokenRate()
{
    return m_tokenRate;
}

void 
RsvpSenderSpec::SetTokenRate(float tokenRate)
{
    m_tokenRate = tokenRate;
}

float 
RsvpSenderSpec::GetTokenSize()
{
    return m_tokenSize;
}
	
void 
RsvpSenderSpec::SetTokenSize(float tokenSize)
{
    m_tokenSize = tokenSize;
}

float 
RsvpSenderSpec::GetPeakRate()
{
    return m_peakRate;
}

void 
RsvpSenderSpec::SetPeakRate(float peakRate)
{
    m_peakRate = peakRate;
}

uint32_t 
RsvpSenderSpec::GetMinUnit()
{
    return m_minUnit;
}

void 
RsvpSenderSpec::SetMinUnit(uint32_t minUnit)
{
    m_minUnit = minUnit;
}

uint32_t 
RsvpSenderSpec::GetMaxSize()
{
    return m_maxSize;
}

void 
RsvpSenderSpec::SetMaxSize(uint32_t maxSize)
{
    m_maxSize = maxSize;
}

} // namespace ns3