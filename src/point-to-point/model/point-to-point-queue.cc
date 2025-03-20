#include "point-to-point-queue.h"

#include "ns3/abort.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include "ns3/socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/ppp-header.h"

#include "point-to-point-net-device.h"
#include "mpls-header.h"

#include "ipv4-tag.h"
#include "ipv6-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PointToPointQueue");

NS_OBJECT_ENSURE_REGISTERED(PointToPointQueue);

TypeId
PointToPointQueue::GetTypeId()
{
    static TypeId tid = 
        TypeId("ns3::PointToPointQueue")
            .SetParent<Queue<Packet>>()
            .SetGroupName("PointToPoint")
            .AddConstructor<PointToPointQueue>()
            .AddAttribute(
                "ECNThreshold",
                "Threshold for ECN",
                UintegerValue(100000),
                MakeUintegerAccessor(&PointToPointQueue::m_ecnThreshold),
                MakeUintegerChecker<uint32_t>());
    return tid;
}

PointToPointQueue::PointToPointQueue()
{
    m_queues.push_back(CreateObject<DropTailQueue<Packet>>());
    m_queues.push_back(CreateObject<DropTailQueue<Packet>>());
    m_queues[0]->SetMaxSize(QueueSize("16MiB"));
    m_queues[1]->SetMaxSize(QueueSize("16MiB"));
}

PointToPointQueue::~PointToPointQueue() {}

uint64_t 
PointToPointQueue::GetEcnCount()
{
    return m_ecnCount;
}

bool
PointToPointQueue::Enqueue(Ptr<Packet> item)
{
    PppHeader ppp;
    item->RemoveHeader(ppp);

    uint16_t proto = ppp.GetProtocol();

    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;
    MplsHeader mpls_header;

    switch (proto)
    {
    case 0x0021: item->RemoveHeader(ipv4_header); break; // IPv4
    case 0x0057: item->RemoveHeader(ipv6_header); break; // IPv6
    case 0x0281: item->RemoveHeader(mpls_header); break; // MPLS       
    default: break;
    }

    int priority = (proto != 0x0170);
    if(m_queues[priority]->GetNBytes() > m_ecnThreshold){
        switch (proto)
        {
            case 0x0021: 
                if(ipv4_header.GetEcn() == Ipv4Header::ECN_ECT1 || 
                    ipv4_header.GetEcn() == Ipv4Header::ECN_ECT0){
                        m_ecnCount += 1;
                        ipv4_header.SetEcn(Ipv4Header::ECN_CE);
                    }
                break;
            case 0x0057: 
                if(ipv6_header.GetEcn() == Ipv6Header::ECN_ECT1 || 
                    ipv6_header.GetEcn() == Ipv6Header::ECN_ECT0){
                        m_ecnCount += 1;
                        ipv6_header.SetEcn(Ipv6Header::ECN_CE);  
                    }                   
                break; 
            case 0x0281:
                if(mpls_header.GetExp() == MplsHeader::ECN_ECT1 || 
                    mpls_header.GetExp() == MplsHeader::ECN_ECT0){
                        m_ecnCount += 1;
                        mpls_header.SetExp(MplsHeader::ECN_CE);
                    }
                break;    
            default: break;
        }
    }

    switch (proto)
    {
    case 0x0021: item->AddHeader(ipv4_header); break; // IPv4
    case 0x0057: item->AddHeader(ipv6_header); break; // IPv6
    case 0x0281: item->AddHeader(mpls_header); break; // MPLS        
    default: break;
    }

    if(proto == 0x0171 && m_queues[priority]->GetNBytes() > m_ecnThreshold){
        Ipv4Tag ipv4Tag;
        Ipv6Tag ipv6Tag;

        if (item->PeekPacketTag(ipv4Tag)){
            ipv4_header = ipv4Tag.GetHeader();

            if(ipv4_header.GetEcn() == Ipv4Header::ECN_ECT1 || 
                ipv4_header.GetEcn() == Ipv4Header::ECN_ECT0){
                m_ecnCount += 1;
                ipv4_header.SetEcn(Ipv4Header::ECN_CE);

                ipv4Tag.SetHeader(ipv4_header);
                item->ReplacePacketTag(ipv4Tag); 
            }
        }
        else if(item->PeekPacketTag(ipv6Tag)){
            ipv6_header = ipv6Tag.GetHeader();

            if(ipv6_header.GetEcn() == Ipv6Header::ECN_ECT1 || 
                ipv6_header.GetEcn() == Ipv6Header::ECN_ECT0){
                m_ecnCount += 1;
                ipv6_header.SetEcn(Ipv6Header::ECN_CE);
                
                ipv6Tag.SetHeader(ipv6_header);
                item->ReplacePacketTag(ipv6Tag); 
            } 
        }
        else{
            std::cout << "Fail to find tag" << std::endl;
        }
    }

    item->AddHeader(ppp);
    bool ret = m_queues[priority]->Enqueue(item);
    if(!ret){
        std::cout << "Error in buffer " << priority << std::endl;
        std::cout << "Buffer size " << m_queues[priority]->GetNBytes() << std::endl;
    }

    return ret;
}

Ptr<Packet>
PointToPointQueue::Dequeue()
{
    Ptr<Packet> ret = nullptr;
    for(auto queue : m_queues){
        ret = queue->Dequeue();
        if(ret != nullptr)
            return ret;
    }
    return nullptr;
}

Ptr<Packet>
PointToPointQueue::Remove()
{
    std::cout << "Remove in PointToPointQueue is not implemented now." << std::endl;
    return nullptr;
}

Ptr<const Packet>
PointToPointQueue::Peek() const
{
    std::cout << "Peek in PointToPointQueue is not implemented now." << std::endl;
    return nullptr;
}

bool 
PointToPointQueue::IsEmpty() const
{
    return (m_queues[0]->IsEmpty() && m_queues[1]->IsEmpty());
}

uint32_t
PointToPointQueue::GetNBytes() const
{
    return (m_queues[0]->GetNBytes() + m_queues[1]->GetNBytes());
}

} // namespace ns3