#ifndef COMPRESS_IPV6_HEADER_H
#define COMPRESS_IPV6_HEADER_H

#include "ns3/ipv6-header.h"

namespace ns3
{

class CompressIpv6Header: public Header
{

public:
    CompressIpv6Header();
    ~CompressIpv6Header() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    Ipv6Header GetIpv6Header();
    void SetIpv6Header(Ipv6Header header);
 
protected:
    uint16_t m_payloadSize;
    uint32_t m_identification; 
};

} // namespace ns3

#endif /* COMPRESS_IPV6_HEADER_H */