#ifndef ROHC_HCTCP_HEADER_H
#define ROHC_HCTCP_HEADER_H

#include "ns3/hctcp-header.h"

namespace ns3
{

class RohcHcTcpHeader: public Header
{

public:
    RohcHcTcpHeader();
    ~RohcHcTcpHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    void SetHeader(const HcTcpHeader& a, const HcTcpHeader& b);
    HcTcpHeader GetHeader(const HcTcpHeader& a);

protected:
    uint8_t m_diff;  
    SequenceNumber32 m_sequenceNumber;  
    SequenceNumber32 m_ackNumber;       
    uint8_t m_length;             
    uint8_t m_flags;              
    uint16_t m_windowSize;     
};

} // namespace ns3

#endif /* ROHC_HCTCP_HEADER_H */