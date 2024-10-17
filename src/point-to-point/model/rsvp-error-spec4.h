#ifndef RSVP_ERROR_SPEC4_H
#define RSVP_ERROR_SPEC4_H

#include "rsvp-object.h"

#include "ns3/ipv4-address.h"

namespace ns3
{

class RsvpErrorSpec4 : public RsvpObject
{

public:
    RsvpErrorSpec4();
    ~RsvpErrorSpec4() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

    Ipv4Address GetAddress();
	void SetAddress(Ipv4Address address);

	uint8_t GetFlag();
	void SetFlag(uint8_t flag);

	uint8_t GetCode();
	void SetCode(uint8_t code);

	uint16_t GetValue();
	void SetValue(uint16_t value);
 
  protected:
    Ipv4Address m_address;
	uint8_t m_flag;
	uint8_t m_code;
	uint16_t m_value;
};


} // namespace ns3

#endif /* RSVP_ERROR_SPEC4_H */