#ifndef RSVP_LABEL_H
#define RSVP_LABEL_H

#include "rsvp-object.h"

namespace ns3
{

class RsvpLabel : public RsvpObject
{

public:
    RsvpLabel();
    ~RsvpLabel() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

	uint32_t GetLabel();
	void SetLabel(uint32_t label);
 
  protected:
	uint32_t m_label;
};


} // namespace ns3

#endif /* RSVP_LABEL_H */