#ifndef RSVP_HOP6_H
#define RSVP_HOP6_H

#include "rsvp-object.h"

#include "ns3/ipv6-address.h"

namespace ns3
{

class RsvpHop6 : public RsvpObject
{

public:
    RsvpHop6();
    ~RsvpHop6() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

    Ipv6Address GetAddress();
	void SetAddress(Ipv6Address address);
	
	uint32_t GetInterface();
	void SetInterface(uint32_t interface);
 
  protected:
    Ipv6Address m_address;
	uint32_t m_interface;
};


} // namespace ns3

#endif /* RSVP_HOP6_H */