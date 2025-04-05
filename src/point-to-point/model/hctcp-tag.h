#ifndef HCTCP_TAG_H
#define HCTCP_TAG_H

#include "ns3/tag.h"
#include "hctcp-header.h"

namespace ns3
{

class HcTcpTag : public Tag
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

    void SetHeader(const HcTcpHeader& header);
    HcTcpHeader GetHeader();

    void Print(std::ostream& os) const override;

  private:
    HcTcpHeader m_header; 
};

} // namespace ns3

#endif /* HCTCP_TAG_H */