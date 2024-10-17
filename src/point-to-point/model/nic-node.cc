#include "nic-node.h"

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
#include "rsvp-header.h"
#include "mpls-header.h"

#include <unordered_set>
#include <unordered_map>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NICNode");

NS_OBJECT_ENSURE_REGISTERED(NICNode);

TypeId
NICNode::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NICNode")
            .SetParent<Node>()
            .SetGroupName("PointToPoint")
            .AddConstructor<NICNode>();
    return tid;
}

NICNode::NICNode() : Node() 
{
}

NICNode::~NICNode()
{
}

uint32_t
NICNode::AddDevice(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    uint32_t index = m_devices.size();
    m_devices.push_back(device);
    device->SetNode(this);
    device->SetIfIndex(index);
    device->SetReceiveCallback(MakeCallback(&NICNode::ReceiveFromDevice, this));
    Simulator::ScheduleWithContext(GetId(), Seconds(0.0), &NetDevice::Initialize, device);
    NotifyDeviceAdded(device);
    return index;
}

bool
NICNode::ReceiveFromDevice(Ptr<NetDevice> device,
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
NICNode::SetECMPHash(uint32_t hashSeed)
{
    m_hashSeed = hashSeed;
}

void
NICNode::SetID(uint32_t id)
{
    m_nid = id;
}

uint32_t
NICNode::GetID()
{
    return m_nid;
}

void
NICNode::AddHostRouteTo(Ipv4Address dest, uint32_t devId)
{
    m_v4route[dest.Get()].push_back(devId);
}

void
NICNode::AddHostRouteTo(Ipv6Address dest, uint32_t devId)
{
    m_v6route[Ipv6ToPair(dest)].push_back(devId);
}

Ptr<Packet>
NICNode::EgressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev){
    PppHeader ppp;
    packet->RemoveHeader(ppp);

    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;
    uint8_t proto = 0;

    if(protocol == 0x0800){
        packet->PeekHeader(ipv4_header);
        proto = ipv4_header.GetProtocol();
    }
    else if(protocol == 0x86DD){
        packet->PeekHeader(ipv6_header);
        proto = ipv6_header.GetNextHeader();
    }

    if(proto == 6 || proto == 17){
        m_userSize -= packet->GetSize();
        if(m_userSize < 0){
            std::cout << "Error for userSize in NIC " << m_nid << std::endl;  
            std::cout << "Egress size : " << m_userSize << std::endl;
        } 
    }

    packet->AddHeader(ppp);
    return packet;
}

bool
NICNode::IngressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev){
    // std::cout << "NIC " << m_nid << " Ingress" << std::endl;
    // std::cout << "Time: " << Simulator::Now().GetNanoSeconds() << std::endl;
    // std::cout << "Start Packet size: " << packet->GetSize() << std::endl;

    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;
    MplsHeader mpls_header;

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
    else if(protocol == 0x8847){
        // TODO: Handle MPLS packets in NIC
        std::cout << "TODO: Handle MPLS packets in NIC" << std::endl;
        return false;
    }
    else{
        std::cout << "Unknown Protocol for IngressPipeline" << std::endl;
        return false;
    }

    if(route_vec.size() <= 0){
        std::cout << "Unknown Destination for Routing in NIC" << std::endl;
        return false;
    }
    if(ttl == 0){
        std::cout << "TTL = 0 in NIC" << std::endl;
        return false;
    }

    bool isUser = (proto == 6 || proto == 17);
    uint16_t src_port = 0, dst_port = 0;
    
    if(isUser){
        UdpHeader udp_header;
        packet->PeekHeader(udp_header);
        src_port = udp_header.GetSourcePort();
        dst_port = udp_header.GetDestinationPort();
    }
    else{
        std::cout << "Unknown Protocol " << int(proto) << std::endl;
        return false;
    }

    Ptr<NetDevice> device;
    if(protocol == 0x0800 && isUser){
        v4Id.m_srcPort = src_port;
        v4Id.m_dstPort = dst_port;   
        packet->AddHeader(ipv4_header);   

        m_v4count[v4Id] += 1;
        // TODO: Dynamic threshold
        if(m_v4count[v4Id] == m_threshold)
            Simulator::Schedule(NanoSeconds(1), &NICNode::CreateRsvpPath4, this, v4Id);

        uint32_t hashValue = hash(v4Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        device = m_devices[devId];
    }
    else if(protocol == 0x86DD && isUser){
        v6Id.m_srcPort = src_port;
        v6Id.m_dstPort = dst_port;   
        packet->AddHeader(ipv6_header); 

        m_v6count[v6Id] += 1;
        // TODO: Dynamic threshold
        if(m_v6count[v6Id] == m_threshold)
            Simulator::Schedule(NanoSeconds(1), &NICNode::CreateRsvpPath6, this, v6Id);

        uint32_t hashValue = hash(v6Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        device = m_devices[devId];  
    }
    else{
        std::cout << "TODO: Handle Other packets" << std::endl;
        return false;
    }

    if(m_userSize + packet->GetSize() <= m_userThd && isUser){
        m_userSize += packet->GetSize();
        return device->Send(packet, device->GetBroadcast(), protocol);
    }

    return false;
}

void 
NICNode::CreateRsvpPath4(FlowV4Id id){
    Ptr<Packet> packet = Create<Packet>();
    Ipv4Header ipHeader;

    ipHeader.SetSource(Ipv4Address(id.m_srcIP));
    ipHeader.SetDestination(Ipv4Address(id.m_dstIP));
    ipHeader.SetProtocol(46);
    ipHeader.SetTtl(64);

    RsvpHeader rsvpHeader;
    rsvpHeader.SetType(RsvpHeader::Path);
    rsvpHeader.SetTtl(64);

    auto lsp4 = DynamicCast<RsvpLsp4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Session << 8) | 7));
    lsp4->SetAddress(Ipv4Address(id.m_dstIP));
    lsp4->SetId(id.m_dstPort);
    lsp4->SetExtend(id.m_protocol);
    rsvpHeader.AppendObject(lsp4);

    auto hop4 = DynamicCast<RsvpHop4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Hop << 8) | 1));
    hop4->SetAddress(Ipv4Address(id.m_dstIP));
    rsvpHeader.AppendObject(hop4);

    auto timeValue = DynamicCast<RsvpTimeValue>(RsvpObject::CreateObject(((uint16_t)RsvpObject::TimeValue << 8) | 1));
    timeValue->SetPeriod(2 * m_timeout);
    rsvpHeader.AppendObject(timeValue);

    auto labelRequest = DynamicCast<RsvpLabelRequest>(RsvpObject::CreateObject(((uint16_t)RsvpObject::LabelRequest << 8) | 1));
    labelRequest->SetId(0x0800);
    rsvpHeader.AppendObject(labelRequest);

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::SenderTemplate << 8) | 7));
    senderTemplate4->SetAddress(Ipv4Address(id.m_srcIP));
    senderTemplate4->SetId(id.m_srcPort);
    rsvpHeader.AppendObject(senderTemplate4);

    auto senderSpec = DynamicCast<RsvpSenderSpec>(RsvpObject::CreateObject(((uint16_t)RsvpObject::SenderSpec << 8) | 2));
    senderSpec->SetMaxSize(1500);
    rsvpHeader.AppendObject(senderSpec);

    ipHeader.SetPayloadSize(rsvpHeader.GetSerializedSize());
    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipHeader);

    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0800);
}

void
NICNode::CreateRsvpPath6(FlowV6Id id){
    Ptr<Packet> packet = Create<Packet>();
    Ipv6Header ipHeader;

    ipHeader.SetSource(PairToIpv6(
        std::pair<uint64_t, uint64_t>(id.m_srcIP[0], id.m_srcIP[1])));
    ipHeader.SetDestination(PairToIpv6(
        std::pair<uint64_t, uint64_t>(id.m_dstIP[0], id.m_dstIP[1])));
    ipHeader.SetNextHeader(46); 
    ipHeader.SetHopLimit(64);

    RsvpHeader rsvpHeader;
    rsvpHeader.SetType(RsvpHeader::Path);
    rsvpHeader.SetTtl(64);

    Ptr<RsvpLsp6> lsp6 = DynamicCast<RsvpLsp6>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Session << 8) | 8));
    lsp6->SetAddress(ipHeader.GetDestination());
    lsp6->SetId(id.m_dstPort);
    uint8_t extend[16] = {0};
    extend[0] = id.m_protocol;
    lsp6->SetExtend(extend);
    rsvpHeader.AppendObject(lsp6);

    auto hop6 = DynamicCast<RsvpHop6>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Hop << 8) | 2));
    hop6->SetAddress(ipHeader.GetDestination());
    rsvpHeader.AppendObject(hop6);

    auto timeValue = DynamicCast<RsvpTimeValue>(RsvpObject::CreateObject(((uint16_t)RsvpObject::TimeValue << 8) | 1));
    timeValue->SetPeriod(2 * m_timeout);
    rsvpHeader.AppendObject(timeValue);

    auto labelRequest = DynamicCast<RsvpLabelRequest>(RsvpObject::CreateObject(((uint16_t)RsvpObject::LabelRequest << 8) | 1));
    labelRequest->SetId(0x0800);
    rsvpHeader.AppendObject(labelRequest);

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(RsvpObject::CreateObject(((uint16_t)RsvpObject::SenderTemplate << 8) | 8));
    senderTemplate6->SetAddress(ipHeader.GetSource());
    senderTemplate6->SetId(id.m_srcPort);
    rsvpHeader.AppendObject(senderTemplate6);

    auto senderSpec = DynamicCast<RsvpSenderSpec>(RsvpObject::CreateObject(((uint16_t)RsvpObject::SenderSpec << 8) | 2));
    senderSpec->SetMaxSize(1500);
    rsvpHeader.AppendObject(senderSpec);

    ipHeader.SetPayloadLength(rsvpHeader.GetSerializedSize());

    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipHeader);

    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x86DD);
}

/* Hash function */
uint32_t
NICNode::rotateLeft(uint32_t x, unsigned char bits)
{
    return (x << bits) | (x >> (32 - bits));
}

uint32_t
NICNode::inhash(const uint8_t* data, uint64_t length, uint32_t seed)
{
    // std::cout << "Seed: " << seed << std::endl;
    // std::cout << data << std::endl;

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
NICNode::hash(const T& data, uint32_t seed){
    return inhash((uint8_t*)&data, sizeof(T), seed);
}

} // namespace ns3
