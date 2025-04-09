#ifndef VXLAN_HEADER_H
#define VXLAN_HEADER_H

#include "ns3/header.h"

namespace ns3
{

class VxlanHeader : public Header
{
  public:

    VxlanHeader();
    ~VxlanHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    uint8_t GetFlag();
    void SetFlag(uint8_t flag);

    uint32_t GetVni();
    void SetVni(uint32_t vni);

  private:
	uint32_t m_flag;
    uint32_t m_vni;
};

} // namespace ns3

#endif /* VXLAN_HEADER_H */