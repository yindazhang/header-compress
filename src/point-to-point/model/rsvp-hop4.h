#ifndef RSVP_HOP4_H
#define RSVP_HOP4_H

#include "rsvp-object.h"

#include "ns3/ipv4-address.h"

namespace ns3
{

class RsvpHop4 : public RsvpObject
{

public:
    RsvpHop4();
    ~RsvpHop4() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

    Ipv4Address GetAddress();
	void SetAddress(Ipv4Address address);
	
	uint32_t GetInterface();
	void SetInterface(uint32_t interface);
 
  protected:
    Ipv4Address m_address;
	uint32_t m_interface;
};


} // namespace ns3

#endif /* RSVP_HOP4_H */