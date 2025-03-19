#ifndef IPV6_TAG_H
#define IPV6_TAG_H

#include "ns3/tag.h"
#include "ns3/ipv6-header.h"

namespace ns3
{

class Ipv6Tag : public Tag
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


    void SetHeader(Ipv6Header header);
    Ipv6Header GetHeader() const;

    void Print(std::ostream& os) const override;

  private:
    Ipv6Header m_header; 
};

} // namespace ns3

#endif /* IPV6_TAG_H */