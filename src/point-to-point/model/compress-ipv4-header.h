#ifndef COMPRESS_IPV4_HEADER_H
#define COMPRESS_IPV4_HEADER_H

#include "ns3/ipv4-header.h"

namespace ns3
{

class CompressIpv4Header: public Header
{

public:
    CompressIpv4Header();
    ~CompressIpv4Header() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    Ipv4Header GetIpv4Header();
    void SetIpv4Header(Ipv4Header header);
 
protected:
    uint16_t m_payloadSize;
    uint16_t m_identification;
};

} // namespace ns3

#endif /* COMPRESS_IPV4_HEADER_H */