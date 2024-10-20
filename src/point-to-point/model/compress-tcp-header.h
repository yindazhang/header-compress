#ifndef COMPRESS_TCP_HEADER_H
#define COMPRESS_TCP_HEADER_H


#include "ns3/tcp-header.h"

#include "rsvp-object.h"

namespace ns3
{

class CompressTcpHeader: public Header
{

public:
    typedef std::list<Ptr<const TcpOption>> TcpOptionList; 
    
    CompressTcpHeader();
    ~CompressTcpHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    TcpHeader GetTcpHeader();
    void SetTcpHeader(TcpHeader header);
 
protected:
    SequenceNumber32 m_sequenceNumber; 
    SequenceNumber32 m_ackNumber; 

    uint16_t m_length;             
    uint16_t m_flags;                   
    uint16_t m_windowSize;             
    uint16_t m_urgentPointer;  

    TcpOptionList m_options;
};

} // namespace ns3

#endif /* COMPRESS_TCP_HEADER_H */