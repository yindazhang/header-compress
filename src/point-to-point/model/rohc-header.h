#ifndef ROHC_HEADER_H
#define ROHC_HEADER_H

#include "ns3/header.h"

namespace ns3
{

class RohcHeader : public Header
{
  public:

    RohcHeader();
    ~RohcHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    uint8_t GetType();
    void SetType(uint8_t type);

    uint8_t GetProfile();
    void SetProfile(uint8_t profile);

    uint16_t GetCid();
    void SetCid(uint16_t cid);

  private:
	  uint8_t m_type;
    uint8_t m_profile;
    uint16_t m_cid;
};

} // namespace ns3

#endif /* ROHC_HEADER_H */