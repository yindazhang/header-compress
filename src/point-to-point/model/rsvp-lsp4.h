#ifndef RSVP_LSP4_H
#define RSVP_LSP4_H

#include "rsvp-object.h"

#include "ns3/ipv4-address.h"

namespace ns3
{

class RsvpLsp4 : public RsvpObject
{

public:
    RsvpLsp4();
    ~RsvpLsp4() override;
 
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

	uint32_t GetExtend();
	void SetExtend(uint32_t extend);
 
  protected:
    Ipv4Address m_address;
	uint16_t m_id;
	uint32_t m_extend;
};


} // namespace ns3

#endif /* RSVP_LSP4_H */