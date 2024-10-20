#include "compress-tcp-header.h"
#include "rsvp-object.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CompressTcpHeader");

NS_OBJECT_ENSURE_REGISTERED(CompressTcpHeader);

CompressTcpHeader::CompressTcpHeader()
{
}

CompressTcpHeader::~CompressTcpHeader()
{
}

TypeId
CompressTcpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CompressTcpHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<CompressTcpHeader>();
    return tid;
}

TypeId
CompressTcpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CompressTcpHeader::Print(std::ostream& os) const
{
    return;
}

uint32_t
CompressTcpHeader::GetSerializedSize() const
{
    return m_length + 14;
}

void
CompressTcpHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_sequenceNumber.GetValue());
    start.WriteHtonU32(m_ackNumber.GetValue());
    start.WriteU8(m_length);
    start.WriteU8(m_flags);
    start.WriteHtonU16(m_windowSize);
    start.WriteHtonU16(m_urgentPointer);

    for(auto op = m_options.begin(); op != m_options.end(); ++op){
        uint32_t len = (*op)->GetSerializedSize();
        (*op)->Serialize(start);
        start.Next(len);
    }
}

uint32_t
CompressTcpHeader::Deserialize(Buffer::Iterator start)
{
    m_sequenceNumber = start.ReadNtohU32();
    m_ackNumber = start.ReadNtohU32();
    m_length = start.ReadU8();
    m_flags = start.ReadU8();
    m_windowSize = start.ReadNtohU16();
    m_urgentPointer = start.ReadNtohU16();

    m_options.clear();
    int32_t optionLen = m_length;
    while(optionLen > 0){
        uint8_t kind = start.PeekU8();
        Ptr<TcpOption> op;
        int32_t optionSize;
        if (TcpOption::IsKindKnown(kind))
            op = TcpOption::CreateOption(kind);
        else{
            op = TcpOption::CreateOption(TcpOption::UNKNOWN);
            std::cout << "Option kind " << static_cast<int>(kind) << " unknown, skipping." << std::endl;
        }

        optionSize = op->Deserialize(start);
        if (optionLen >= optionSize){
            optionLen -= optionSize;
            start.Next(optionSize);
            m_options.emplace_back(op);
        }
        else{
            std::cout << "Option exceeds TCP option space; option discarded" << std::endl;
            break;
        }
        if (op->GetKind() == TcpOption::END && optionLen != 0)
            std::cout << "Error in Compress TCP Header" << std::endl;
    }

    return GetSerializedSize();
}

TcpHeader 
CompressTcpHeader::GetTcpHeader()
{
    TcpHeader header;
    header.SetSequenceNumber(m_sequenceNumber);
    header.SetAckNumber(m_ackNumber);
    header.SetFlags(m_flags);
    header.SetUrgentPointer(m_urgentPointer);
    for(auto i = m_options.begin();i != m_options.end();++i)
        header.AppendOption(*i);
    return header;
}
    
void 
CompressTcpHeader::SetTcpHeader(TcpHeader header)
{
    m_sequenceNumber = header.GetSequenceNumber();
    m_ackNumber = header.GetAckNumber();
    m_flags = header.GetFlags();
    m_windowSize = header.GetWindowSize();
    m_urgentPointer = header.GetUrgentPointer();

    m_length = 0;
    m_options.clear();
    auto options = header.GetOptionList();
    for(auto i = options.begin();i != options.end();++i){
        m_options.push_back(*i);
        m_length += (*i)->GetSerializedSize();
    }
}

} // namespace ns3
