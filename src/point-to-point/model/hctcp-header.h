#ifndef HCTCP_HEADER_H
#define HCTCP_HEADER_H

#include "ns3/tcp-header.h"

namespace ns3
{

class HcTcpHeader: public Header
{

public:
    HcTcpHeader();
    ~HcTcpHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    SequenceNumber32 GetSequenceNumber() const;
    void SetSequenceNumber(uint32_t sequenceNumber);

    SequenceNumber32 GetAckNumber() const;
    void SetAckNumber(uint32_t ackNumber);

    uint16_t GetLength() const;
    void SetLength(uint8_t length);

    uint8_t GetFlags() const;
    void SetFlags(uint8_t flags);

    uint16_t GetWindowSize() const;
    void SetWindowSize(uint16_t windowSize);

protected:
    SequenceNumber32 m_sequenceNumber;  
    SequenceNumber32 m_ackNumber;       
    uint8_t m_length;             
    uint8_t m_flags;              
    uint16_t m_windowSize;     
};

} // namespace ns3

#endif /* HCTCP_HEADER_H */