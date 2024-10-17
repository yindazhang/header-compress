#ifndef RSVP_LABEL_REQUEST_H
#define RSVP_LABEL_REQUEST_H

#include "rsvp-object.h"

namespace ns3
{

class RsvpLabelRequest : public RsvpObject
{

public:
    RsvpLabelRequest();
    ~RsvpLabelRequest() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

	uint16_t GetId();
	void SetId(uint16_t id);
 
  protected:
	uint16_t m_id;
};


} // namespace ns3

#endif /* RSVP_LABEL_REQUEST_H */