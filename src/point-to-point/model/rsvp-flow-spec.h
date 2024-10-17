#ifndef RSVP_FLOW_SPEC_H
#define RSVP_FLOW_SPEC_H

#include "rsvp-object.h"

namespace ns3
{

class RsvpFlowSpec : public RsvpObject
{

public:
    RsvpFlowSpec();
    ~RsvpFlowSpec() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

	float GetTokenRate();
	void SetTokenRate(float tokenRate);

    float GetTokenSize();
	void SetTokenSize(float tokenSize);

    float GetPeakRate();
	void SetPeakRate(float peakRate);

    uint32_t GetMinUnit();
	void SetMinUnit(uint32_t minUnit);

    uint32_t GetMaxSize();
	void SetMaxSize(uint32_t maxSize);
 
  protected:
	float m_tokenRate;
    float m_tokenSize;
    float m_peakRate;
    uint32_t m_minUnit;
    uint32_t m_maxSize;
};


} // namespace ns3

#endif /* RSVP_FLOW_SPEC_H */