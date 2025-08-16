#include "point-to-point-queue.h"

#include "ns3/abort.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include "ns3/socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/ppp-header.h"
#include "ns3/tcp-header.h"

#include "point-to-point-net-device.h"
#include "mpls-header.h"

#include "ipv4-tag.h"
#include "ipv6-tag.h"
#include "packet-tag.h"

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
    for(uint32_t i = 0; i < 4; ++i){
        m_queues.push_back(CreateObject<DropTailQueue<Packet>>());
        m_queues[i]->SetMaxSize(QueueSize("16MiB"));
    }
    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0));
    m_random->SetAttribute("Max", DoubleValue(1));
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
    PortHeader port_header;

    switch (proto)
    {
    case 0x0021: item->RemoveHeader(ipv4_header); break; // IPv4
    case 0x0057: item->RemoveHeader(ipv6_header); break; // IPv6
    case 0x0281: item->RemoveHeader(mpls_header); break; // MPLS
    default: break;
    }

    int priority = 0;
    SocketPriorityTag socketPriorityTag;
    if(item->PeekPacketTag(socketPriorityTag))
        priority = socketPriorityTag.GetPriority();

    if(priority == 2 && SetEcn()){
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

    if(proto == 0x0171 && priority == 2 && SetEcn()){
        Ipv4Tag ipv4Tag;
        Ipv6Tag ipv6Tag;

        if (item->PeekPacketTag(ipv4Tag)){
            ipv4Tag.GetHeader(ipv4_header, port_header);

            if(ipv4_header.GetEcn() == Ipv4Header::ECN_ECT1 ||
                ipv4_header.GetEcn() == Ipv4Header::ECN_ECT0){
                m_ecnCount += 1;
                ipv4_header.SetEcn(Ipv4Header::ECN_CE);

                ipv4Tag.SetHeader(ipv4_header, port_header);
                item->ReplacePacketTag(ipv4Tag);
            }
        }
        else if(item->PeekPacketTag(ipv6Tag)){
            ipv6Tag.GetHeader(ipv6_header, port_header);

            if(ipv6_header.GetEcn() == Ipv6Header::ECN_ECT1 ||
                ipv6_header.GetEcn() == Ipv6Header::ECN_ECT0){
                m_ecnCount += 1;
                ipv6_header.SetEcn(Ipv6Header::ECN_CE);

                ipv6Tag.SetHeader(ipv6_header, port_header);
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

    PacketTag packetTag;
    if(item->PeekPacketTag(packetTag))
        m_ecnSize += packetTag.GetSize();

    return ret;
}

Ptr<Packet> 
PointToPointQueue::Dequeue(bool pause)
{
    if(!pause)
        return this->Dequeue();

    for(uint32_t i = 0; i < 2; ++i){
        Ptr<Packet> ret = m_queues[i]->Dequeue();
        if(ret != nullptr){
            PacketTag packetTag;
            if(ret->PeekPacketTag(packetTag))
                m_ecnSize -= packetTag.GetSize();
            return ret;
        }
    }
    return nullptr;
}

Ptr<Packet>
PointToPointQueue::Dequeue()
{
    for(auto queue : m_queues){
        Ptr<Packet> ret = queue->Dequeue();
        if(ret != nullptr){
            PacketTag packetTag;
            if(ret->PeekPacketTag(packetTag))
                m_ecnSize -= packetTag.GetSize();
            return ret;
        }
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
    for(uint32_t i = 0; i < 4; ++i)
        if(!m_queues[i]->IsEmpty())
            return false;
    return true;
}

uint32_t
PointToPointQueue::GetNBytes() const
{
    uint32_t totalBytes = 0;
    for (const auto& queue : m_queues)
        totalBytes += queue->GetNBytes();
    return totalBytes;
}

bool
PointToPointQueue::SetEcn()
{
    if(m_ecnSize > m_ecnThreshold)
        return true;
    if(m_ecnSize < 50000)
        return false;

    double prob = (m_ecnSize - 50000) / (m_ecnThreshold - 50000) * 0.01;
    return m_random->GetValue() < prob;
}

} // namespace ns3

