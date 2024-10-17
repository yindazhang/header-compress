#ifndef RSVP_FILTER_SPEC4_H
#define RSVP_FILTER_SPEC4_H

#include "rsvp-object.h"

#include "ns3/ipv4-address.h"

namespace ns3
{

class RsvpFilterSpec4 : public RsvpObject
{

public:
    RsvpFilterSpec4();
    ~RsvpFilterSpec4() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

    Ipv4Address GetAddress();
	void SetAddress(Ipv4Address address);
	
	uint16_t GetId();
	void SetId(uint16_t id);
 
  protected:
    Ipv4Address m_address;
	uint16_t m_id;
};


} // namespace ns3

#endif /* RSVP_FILTER_SPEC4_H */