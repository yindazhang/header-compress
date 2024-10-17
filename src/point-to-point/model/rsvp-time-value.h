#ifndef RSVP_TIME_VALUE_H
#define RSVP_TIME_VALUE_H

#include "rsvp-object.h"

namespace ns3
{

class RsvpTimeValue : public RsvpObject
{

public:
    RsvpTimeValue();
    ~RsvpTimeValue() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

	uint32_t GetPeriod();
	void SetPeriod(uint32_t period);
 
  protected:
	uint32_t m_period;
};


} // namespace ns3

#endif /* RSVP_TIME_VALUE_H */