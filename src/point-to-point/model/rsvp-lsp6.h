#ifndef RSVP_LSP6_H
#define RSVP_LSP6_H

#include "rsvp-object.h"

#include "ns3/ipv6-address.h"

namespace ns3
{

class RsvpLsp6 : public RsvpObject
{

public:
    RsvpLsp6();
    ~RsvpLsp6() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

    Ipv6Address GetAddress();
	void SetAddress(Ipv6Address address);
	
	uint16_t GetId();
	void SetId(uint16_t id);

	void GetExtend(uint8_t extend[16]);
	void SetExtend(uint8_t extend[16]);
 
  protected:
    Ipv6Address m_address;
	uint16_t m_id;
  uint8_t m_extend[16];
};


} // namespace ns3

#endif /* RSVP_LSP6_H */