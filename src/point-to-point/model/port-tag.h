#ifndef PORT_TAG_H
#define PORT_TAG_H

#include "ns3/tag.h"
#include "ns3/port-header.h"

namespace ns3
{

class PortTag : public Tag
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


    void SetHeader(const PortHeader& header);
    PortHeader GetHeader();

    void Print(std::ostream& os) const override;

  private:
    PortHeader m_header; 
};

} // namespace ns3

#endif /* PORT_TAG_H */