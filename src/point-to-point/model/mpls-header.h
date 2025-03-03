#ifndef MPLS_HEADER_H
#define MPLS_HEADER_H

#include "ns3/header.h"

namespace ns3
{

class MplsHeader : public Header
{
  public:

    MplsHeader();
    ~MplsHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    enum EcnType
    {
        ECN_NotECT = 0x00,
        ECN_ECT1 = 0x01,
        ECN_ECT0 = 0x02,
        ECN_CE = 0x03
    };

    uint16_t GetLabel();
    void SetLabel(uint16_t label);

    uint8_t GetType();
    void SetType(uint8_t type);

    uint8_t GetExp();
    void SetExp(uint8_t exp);

    uint8_t GetTtl();
    void SetTtl(uint8_t ttl);

    uint8_t GetBos();
    void SetBos();
    void ClearBos();

  private:
	  uint16_t m_label;
    uint16_t m_value;
};

} // namespace ns3

#endif /* MPLS_HEADER_H */