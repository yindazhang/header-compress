#include "switch-node.h"

#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/node-list.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/global-value.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/socket.h"

#include "point-to-point-net-device.h"
#include "point-to-point-queue.h"
#include "ppp-header.h"

#include <unordered_set>
#include <unordered_map>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SwitchNode");

NS_OBJECT_ENSURE_REGISTERED(SwitchNode);

TypeId
SwitchNode::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SwitchNode")
            .SetParent<Node>()
            .SetGroupName("PointToPoint")
            .AddConstructor<SwitchNode>();
    return tid;
}

SwitchNode::SwitchNode() : Node() 
{
}

SwitchNode::~SwitchNode()
{
}

uint32_t
SwitchNode::AddDevice(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    uint32_t index = m_devices.size();
    m_devices.push_back(device);
    device->SetNode(this);
    device->SetIfIndex(index);
    device->SetReceiveCallback(MakeCallback(&SwitchNode::ReceiveFromDevice, this));
    Simulator::ScheduleWithContext(GetId(), Seconds(0.0), &NetDevice::Initialize, device);
    NotifyDeviceAdded(device);
    return index;
}

bool
SwitchNode::ReceiveFromDevice(Ptr<NetDevice> device,
                                  Ptr<const Packet> p,
                                  uint16_t protocol,
                                  const Address& from)
{
    Ptr<Packet> packet = p->Copy();

    uint32_t priority = 0;
    SocketPriorityTag priorityTag;
    if(packet->PeekPacketTag(priorityTag))
        priority = priorityTag.GetPriority();

    return IngressPipeline(packet, priority, protocol, device);
}

void
SwitchNode::SetECMPHash(uint32_t hashSeed)
{
    m_hashSeed = hashSeed;
}

void
SwitchNode::SetID(uint32_t id)
{
    m_nid = id;
}

uint32_t
SwitchNode::GetID()
{
    return m_nid;
}

void
SwitchNode::AddHostRouteTo(Ipv4Address dest, uint32_t devId)
{
    m_v4route[dest.Get()].push_back(devId);
}

void
SwitchNode::AddHostRouteTo(Ipv6Address dest, uint32_t devId)
{
    m_v6route[Ipv6ToPair(dest)].push_back(devId);
}

Ptr<Packet>
SwitchNode::EgressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev){
    PppHeader ppp;
    packet->RemoveHeader(ppp);

    m_userSize -= packet->GetSize();
    if(m_userSize < 0){
        std::cout << "Error for userSize in Switch " << m_nid << std::endl;  
        std::cout << "Egress size : " << m_userSize << std::endl;  
    }   

    packet->AddHeader(ppp);
    return packet;
}

bool
SwitchNode::IngressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev){
    // std::cout << "Switch " << m_nid << " Ingress" << std::endl;
    // std::cout << "Time: " << Simulator::Now().GetNanoSeconds() << std::endl;
    // std::cout << "Start Packet size: " << packet->GetSize() << std::endl;

    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;

    FlowV4Id v4Id;
    FlowV6Id v6Id;

    // memset(&v4Id, 0, sizeof(FlowV4Id));
    // memset(&v6Id, 0, sizeof(FlowV6Id));

    uint8_t proto = 0, ttl = 0;
    std::vector<uint32_t> route_vec;

    if(protocol == 0x0800){
        packet->RemoveHeader(ipv4_header);

        proto = ipv4_header.GetProtocol();
        ttl = ipv4_header.GetTtl();

        v4Id.m_srcIP = ipv4_header.GetSource().Get();
        v4Id.m_dstIP = ipv4_header.GetDestination().Get();
        v4Id.m_protocol = proto;

        // print_v4addr(ipv4_header.GetDestination());
        
        route_vec = m_v4route[v4Id.m_dstIP];

        ipv4_header.SetTtl(ttl - 1);        
    }
    else if(protocol == 0x86DD){
        packet->RemoveHeader(ipv6_header);

        proto = ipv6_header.GetNextHeader();
        ttl = ipv6_header.GetHopLimit();

        auto src_pair = Ipv6ToPair(ipv6_header.GetSource());
        auto dst_pair = Ipv6ToPair(ipv6_header.GetDestination());

        v6Id.m_srcIP[0] = src_pair.first;
        v6Id.m_srcIP[1] = src_pair.second;
        v6Id.m_dstIP[0] = dst_pair.first;
        v6Id.m_dstIP[1] = dst_pair.second;
        v6Id.m_protocol = proto;

        // print_v6addr(ipv6_header.GetDestination());

        route_vec = m_v6route[dst_pair];

        ipv6_header.SetHopLimit(ttl - 1);  
    }
    else{
        std::cout << "Unknown Protocol for IngressPipeline" << std::endl;
        return false;
    }

    if(route_vec.size() <= 0){
        std::cout << "Unknown Destination for Routing in Switch" << std::endl;
        return false;
    }
    if(ttl == 0)
        return false;
    if(proto != 6 && proto != 17){
        std::cout << "Unknown Protocol " << int(proto) << std::endl;
        return false;
    }

    // print_v4addr(ipv4_header.GetDestination());
    // std::cout << route_vec.size() << std::endl;
    
    uint16_t src_port = 0, dst_port = 0;

    if(proto == 6){
        TcpHeader tcp_header;
        packet->RemoveHeader(tcp_header);
        src_port = tcp_header.GetSourcePort();
        dst_port = tcp_header.GetDestinationPort();
        packet->AddHeader(tcp_header);
    }
    else if(proto == 17){
        UdpHeader udp_header;
        packet->RemoveHeader(udp_header);
        src_port = udp_header.GetSourcePort();
        dst_port = udp_header.GetDestinationPort();
        packet->AddHeader(udp_header);
    }
    else{
        std::cout << "Not TCP/UDP" << std::endl;
        return false;
    }


    if(protocol == 0x0800){
        v4Id.m_srcPort = src_port;
        v4Id.m_dstPort = dst_port;   
        packet->AddHeader(ipv4_header);   

        uint32_t hashValue = hash(v4Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        Ptr<NetDevice> dev = m_devices[devId];

        if(m_userSize + packet->GetSize() <= m_userThd){
            m_userSize += packet->GetSize();
            return dev->Send(packet, dev->GetBroadcast(), 0x0800);
        }
    }
    else if(protocol == 0x86DD){
        v6Id.m_srcPort = src_port;
        v6Id.m_dstPort = dst_port;   
        packet->AddHeader(ipv6_header); 

        uint32_t hashValue = hash(v6Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        Ptr<NetDevice> dev = m_devices[devId];  

        if(m_userSize + packet->GetSize() <= m_userThd){
            m_userSize += packet->GetSize();
            return dev->Send(packet, dev->GetBroadcast(), 0x86DD);
        }
    }
    else{
        std::cout << "Unknown Protocol for IngressPipeline" << std::endl;
        return false;
    }

    return false;
}

/* Hash function */
uint32_t
SwitchNode::rotateLeft(uint32_t x, unsigned char bits)
{
    return (x << bits) | (x >> (32 - bits));
}

uint32_t
SwitchNode::inhash(const uint8_t* data, uint64_t length, uint32_t seed)
{
    seed = prime[seed];
    uint32_t state[4] = {seed + Prime[0] + Prime[1],
                        seed + Prime[1], seed, seed - Prime[0]};
    uint32_t result = length + state[2] + Prime[4];

    // point beyond last byte
    const uint8_t* stop = data + length;

    // at least 4 bytes left ? => eat 4 bytes per step
    for (; data + 4 <= stop; data += 4)
        result = rotateLeft(result + *(uint32_t*)data * Prime[2], 17) * Prime[3];

    // take care of remaining 0..3 bytes, eat 1 byte per step
    while (data != stop)
        result = rotateLeft(result + (*data++) * Prime[4], 11) * Prime[0];

    // mix bits
    result ^= result >> 15;
    result *= Prime[1];
    result ^= result >> 13;
    result *= Prime[2];
    result ^= result >> 16;
    return result;
}

template<typename T>
uint32_t
SwitchNode::hash(const T& data, uint32_t seed){
    return inhash((uint8_t*)&data, sizeof(T), seed);
}

} // namespace ns3
