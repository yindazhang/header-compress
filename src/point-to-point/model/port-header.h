#ifndef PORT_HEADER_H
#define PORT_HEADER_H

#include "ns3/header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

namespace ns3
{

class PortHeader: public Header
{

public:
    PortHeader();
    PortHeader(const PortHeader& header);

    ~PortHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    uint16_t GetSourcePort() const;
    void SetSourcePort(uint16_t port);

    uint16_t GetDestinationPort() const;
    void SetDestinationPort(uint16_t port);
 
protected:
    uint16_t m_sourcePort;      
    uint16_t m_destinationPort; 
};

} // namespace ns3

#endif /* PORT_HEADER_H */