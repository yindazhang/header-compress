#include "point-to-point-queue.h"

#include "ns3/abort.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include "ns3/socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/ppp-header.h"

#include "point-to-point-net-device.h"

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
                UintegerValue(65),
                MakeUintegerAccessor(&PointToPointQueue::m_ecnThreshold),
                MakeUintegerChecker<uint32_t>());
    return tid;
}

PointToPointQueue::PointToPointQueue()
{
    m_queues.push_back(CreateObject<DropTailQueue<Packet>>());
    m_queues[0]->SetMaxSize(QueueSize("16MiB"));
}

PointToPointQueue::~PointToPointQueue() {}

bool
PointToPointQueue::Enqueue(Ptr<Packet> item)
{
    PppHeader ppp;
    item->RemoveHeader(ppp);

    uint16_t proto = ppp.GetProtocol();

    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;

    if(proto == 0x0021)
        item->RemoveHeader(ipv4_header);
    else if(proto == 0x0057)
        item->RemoveHeader(ipv6_header);
    else
        std::cout << "unknown proto " << int(proto) << std::endl;

    uint32_t priority = 0;
    SocketPriorityTag priorityTag;
    if(item->PeekPacketTag(priorityTag))
        priority = priorityTag.GetPriority();

    switch(priority){
        case 0 : 
            if(m_queues[0]->GetNPackets() > m_ecnThreshold){
                if(proto == 0x0021){
                    if(ipv4_header.GetEcn() == Ipv4Header::ECN_ECT1 || 
                        ipv4_header.GetEcn() == Ipv4Header::ECN_ECT0)
                        ipv4_header.SetEcn(Ipv4Header::ECN_CE);
                }
                else if(proto == 0x0057){
                    if(ipv6_header.GetEcn() == Ipv6Header::ECN_ECT1 || 
                        ipv6_header.GetEcn() == Ipv6Header::ECN_ECT0)
                        ipv6_header.SetEcn(Ipv6Header::ECN_CE);
                }
            }
            break;
        default : std::cout << "Unknown priority for queue" << std::endl; return false;
    }

    if(proto == 0x0021)
        item->AddHeader(ipv4_header);
    else if(proto == 0x0057)
        item->AddHeader(ipv6_header);
    else
        std::cout << "unknown proto " << int(proto) << std::endl;

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
    return m_queues[0]->IsEmpty();
}

uint32_t
PointToPointQueue::GetNBytes() const
{
    return m_queues[0]->GetNBytes();
}

} // namespace ns3