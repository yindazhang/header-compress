#ifndef RSVP_ERROR_SPEC6_H
#define RSVP_ERROR_SPEC6_H

#include "rsvp-object.h"

#include "ns3/ipv6-address.h"

namespace ns3
{

class RsvpErrorSpec6 : public RsvpObject
{

public:
    RsvpErrorSpec6();
    ~RsvpErrorSpec6() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

    Ipv6Address GetAddress();
	void SetAddress(Ipv6Address address);

	uint8_t GetFlag();
	void SetFlag(uint8_t flag);

	uint8_t GetCode();
	void SetCode(uint8_t code);

	uint16_t GetValue();
	void SetValue(uint16_t value);
 
  protected:
    Ipv6Address m_address;
	uint8_t m_flag;
	uint8_t m_code;
	uint16_t m_value;
};


} // namespace ns3

#endif /* RSVP_ERROR_SPEC6_H */