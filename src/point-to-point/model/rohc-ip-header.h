#ifndef ROHC_IP_HEADER_H
#define ROHC_IP_HEADER_H

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

namespace ns3
{

class RohcIpHeader: public Header
{

public:
    RohcIpHeader();
    ~RohcIpHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    Ipv4Header GetIpv4Header();
    void GetIpv4Header(Ipv4Header& header);
    void SetIpv4Header(Ipv4Header header);

    Ipv6Header GetIpv6Header();
    void GetIpv6Header(Ipv6Header& header);
    void SetIpv6Header(Ipv6Header header);
 
protected:
    uint8_t m_ttl;
    uint8_t m_ecn;
    uint16_t m_payloadSize;
};

} // namespace ns3

#endif /* ROHC_IPV4_HEADER_H */