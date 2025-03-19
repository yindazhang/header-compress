#ifndef IPV4_TAG_H
#define IPV4_TAG_H

#include "ns3/tag.h"
#include "ns3/ipv4-header.h"

namespace ns3
{

class Ipv4Tag : public Tag
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


    void SetHeader(Ipv4Header header);
    Ipv4Header GetHeader() const;

    void Print(std::ostream& os) const override;

  private:
    Ipv4Header m_header; 
};

} // namespace ns3

#endif /* IPV4_TAG_H */