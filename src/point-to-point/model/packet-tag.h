#ifndef PACKET_TAG_H
#define PACKET_TAG_H

#include "ns3/tag.h"
#include "ns3/node.h"

namespace ns3
{

class PacketTag : public Tag
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;

    void SetSize(uint32_t size);
    uint32_t GetSize();

    void SetNetDevice(Ptr<NetDevice> device);
    Ptr<NetDevice> GetNetDevice();

    void Print(std::ostream& os) const override;

  private:
    uint32_t m_size;
    Ptr<NetDevice> m_device;
};

} // namespace ns3

#endif /* PACKET_TAG_H */

