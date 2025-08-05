#ifndef COMPRESS_IP_HEADER_H
#define COMPRESS_IP_HEADER_H

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

namespace ns3
{

class CompressIpHeader: public Header
{

public:
    CompressIpHeader();
    ~CompressIpHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    Ipv4Header GetIpv4Header();
    void SetIpv4Header(Ipv4Header header);

    Ipv6Header GetIpv6Header();
    void SetIpv6Header(Ipv6Header header);
 
protected:
    uint16_t m_payloadSize;
};

} // namespace ns3

#endif /* COMPRESS_IPV4_HEADER_H */