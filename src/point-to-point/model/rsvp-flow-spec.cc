#include "rsvp-object.h"
#include "rsvp-flow-spec.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpFlowSpec");

NS_OBJECT_ENSURE_REGISTERED(RsvpFlowSpec);

RsvpFlowSpec::RsvpFlowSpec()
{
    m_tokenRate = 0;
    m_tokenSize = 0;
    m_peakRate = 0;
    m_minUnit = 0;
    m_maxSize = 0;
}

RsvpFlowSpec::~RsvpFlowSpec()
{
}

TypeId
RsvpFlowSpec::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpFlowSpec")
                            .SetParent<RsvpObject>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<RsvpFlowSpec>();
    return tid;
}

TypeId
RsvpFlowSpec::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RsvpFlowSpec::Print(std::ostream& os) const
{
    return;
}

uint32_t
RsvpFlowSpec::GetSerializedSize() const
{
    return 32;
}

void
RsvpFlowSpec::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(0);
    start.WriteHtonU16(7);
    start.WriteU8(5);
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
RsvpFlowSpec::Deserialize(Buffer::Iterator start)
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
RsvpFlowSpec::GetKind() const
{
    return ((uint16_t)RsvpObject::FlowSpec << 8) | 2;
}

float 
RsvpFlowSpec::GetTokenRate()
{
    return m_tokenRate;
}

void 
RsvpFlowSpec::SetTokenRate(float tokenRate)
{
    m_tokenRate = tokenRate;
}

float 
RsvpFlowSpec::GetTokenSize()
{
    return m_tokenSize;
}
	
void 
RsvpFlowSpec::SetTokenSize(float tokenSize)
{
    m_tokenSize = tokenSize;
}

float 
RsvpFlowSpec::GetPeakRate()
{
    return m_peakRate;
}

void 
RsvpFlowSpec::SetPeakRate(float peakRate)
{
    m_peakRate = peakRate;
}

uint32_t 
RsvpFlowSpec::GetMinUnit()
{
    return m_minUnit;
}

void 
RsvpFlowSpec::SetMinUnit(uint32_t minUnit)
{
    m_minUnit = minUnit;
}

uint32_t 
RsvpFlowSpec::GetMaxSize()
{
    return m_maxSize;
}

void 
RsvpFlowSpec::SetMaxSize(uint32_t maxSize)
{
    m_maxSize = maxSize;
}

} // namespace ns3