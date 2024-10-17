#ifndef RSVP_STYLE_H
#define RSVP_STYLE_H

#include "rsvp-object.h"

namespace ns3
{

class RsvpStyle : public RsvpObject
{

public:
    RsvpStyle();
    ~RsvpStyle() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
 
    uint16_t GetKind() const override;

	uint32_t GetOption();
	void SetOption(uint32_t option);

    enum Option{
       WildcardFilter = 0b10001,
	   FixedFilter = 0b01010,
	   SharedExplicit = 0b10010
    };
 
  protected:
	uint32_t m_option;
};


} // namespace ns3

#endif /* RSVP_STYLE_H */