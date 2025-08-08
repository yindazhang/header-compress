#ifndef PFC_HEADER_H
#define PFC_HEADER_H

#include "ns3/header.h"

namespace ns3
{

class PfcHeader : public Header
{
  public:

    PfcHeader();
    ~PfcHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    void SetPause(uint8_t id);
    void SetResume(uint8_t id);

    uint16_t GetPause(uint8_t id);

  private:
	  uint16_t m_opcode;
    uint16_t m_mask;
    uint16_t m_pause[4];
};

} // namespace ns3

#endif /* PFC_HEADER_H */