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
#include "mpls-header.h"
#include "rsvp-header.h"

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

    if(proto == 6 || proto == 17 || protocol == 0x8847){
        m_userSize -= packet->GetSize();
        if(m_userSize < 0){
            std::cout << "Error for userSize in Switch " << m_nid << std::endl;  
            std::cout << "Egress size : " << m_userSize << std::endl;
        } 
    }

    packet->AddHeader(ppp);
    return packet;
}

bool 
SwitchNode::IngressPipelineMPLS(Ptr<Packet> packet, Ptr<NetDevice> dev){
    MplsHeader mpls_header;
    packet->RemoveHeader(mpls_header);

    uint8_t ttl = mpls_header.GetTtl();
    if(ttl == 0){
        std::cout << "TTL = 0 for MPLS" << std::endl;
        return false;
    }
    mpls_header.SetTtl(ttl - 1);  

    uint32_t label = mpls_header.GetLabel();
    if(m_mplsroute.find(label) == m_mplsroute.end()){
        std::cout << "Unknown Destination for MPLS Routing in Switch " << m_nid << " for label " << label << std::endl;
        return false;
    }

    mpls_header.SetLabel(m_mplsroute[label].newLabel);
    packet->AddHeader(mpls_header);

    if(m_userSize + packet->GetSize() <= m_userThd){
        m_userSize += packet->GetSize();
        return m_mplsroute[label].dev->Send(packet, m_mplsroute[label].dev->GetBroadcast(), 0x8847);
    }
    return false;
}

void 
SwitchNode::IngressPipelineRSVPPath4(Ptr<Packet> packet, Ptr<NetDevice> dev, FlowV4Id v4Id, Ipv4Header ipv4_header, std::vector<uint32_t> route_vec){
    RsvpHeader rsvpHeader;
    packet->PeekHeader(rsvpHeader);

    auto mp = rsvpHeader.GetObjects();

    auto lsp4 = DynamicCast<RsvpLsp4>(mp[((uint16_t)RsvpObject::Session << 8) | 7]);
    v4Id.m_protocol = lsp4->GetExtend();
    v4Id.m_dstPort = lsp4->GetId();   

    auto timeValue = DynamicCast<RsvpTimeValue>(mp[((uint16_t)RsvpObject::TimeValue << 8) | 1]);
    uint32_t timeout = timeValue->GetPeriod();

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 7]);
    v4Id.m_srcPort = senderTemplate4->GetId();

    if(m_pathState4.find(v4Id) != m_pathState4.end()){
        uint32_t number = m_pathState4[v4Id].label;
        std::cout << "Duplicate Path in switch " << m_nid << " for label " << number << std::endl;
        m_mplsroute.erase(number);
        m_labels[number] = 0;
    }
    
    m_pathState4[v4Id].prev = dev;
    m_pathState4[v4Id].label = -1;
    m_pathState4[v4Id].timeout = timeout;
    m_pathState4[v4Id].time = Simulator::Now().GetNanoSeconds();

    packet->AddHeader(ipv4_header); 
    uint32_t hashValue = hash(v4Id, m_hashSeed);

    uint32_t devId = route_vec[hashValue % route_vec.size()];
    m_devices[devId]->Send(packet, m_devices[devId]->GetBroadcast(), 0x0800);
    Simulator::Schedule(NanoSeconds(m_pathState4[v4Id].timeout * 1000000), &SwitchNode::CreateRsvpTear4, this, v4Id, m_devices[devId]);
}

void 
SwitchNode::IngressPipelineRSVPPath6(Ptr<Packet> packet, Ptr<NetDevice> dev, FlowV6Id v6Id, Ipv6Header ipv6_header, std::vector<uint32_t> route_vec){
    RsvpHeader rsvpHeader;
    packet->PeekHeader(rsvpHeader);

    auto mp = rsvpHeader.GetObjects();

    auto lsp6 = DynamicCast<RsvpLsp6>(mp[((uint16_t)RsvpObject::Session << 8) | 8]);
    uint8_t extend[16];
    lsp6->GetExtend(extend);
    v6Id.m_protocol = extend[0];
    v6Id.m_dstPort = lsp6->GetId();   

    auto timeValue = DynamicCast<RsvpTimeValue>(mp[((uint16_t)RsvpObject::TimeValue << 8) | 1]);
    uint32_t timeout = timeValue->GetPeriod();

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 8]);
    v6Id.m_srcPort = senderTemplate6->GetId();

    if(m_pathState6.find(v6Id) != m_pathState6.end()){
        uint32_t number = m_pathState6[v6Id].label;
        std::cout << "Duplicate Path in switch " << m_nid << " for label " << number << std::endl;
        m_mplsroute.erase(number);
        m_labels[number] = 0;
    }

    // std::cout << "Receive Path in Switch " << m_nid << std::endl;

    m_pathState6[v6Id].prev = dev;
    m_pathState6[v6Id].label = -1;
    m_pathState6[v6Id].timeout = timeout;
    m_pathState6[v6Id].time = Simulator::Now().GetNanoSeconds();
    
    packet->AddHeader(ipv6_header); 
    uint32_t hashValue = hash(v6Id, m_hashSeed);
     
    uint32_t devId = route_vec[hashValue % route_vec.size()];
    m_devices[devId]->Send(packet, m_devices[devId]->GetBroadcast(), 0x86DD);
    Simulator::Schedule(NanoSeconds(m_pathState6[v6Id].timeout * 1000000), &SwitchNode::CreateRsvpTear6, this, v6Id, m_devices[devId]);
}

void 
SwitchNode::IngressPipelineRSVPTear4(Ptr<Packet> packet, FlowV4Id v4Id, Ipv4Header ipv4_header, std::vector<uint32_t> route_vec){
    RsvpHeader rsvpHeader;
    packet->PeekHeader(rsvpHeader);

    auto mp = rsvpHeader.GetObjects();

    auto lsp4 = DynamicCast<RsvpLsp4>(mp[((uint16_t)RsvpObject::Session << 8) | 7]);
    v4Id.m_protocol = lsp4->GetExtend();
    v4Id.m_dstPort = lsp4->GetId();   

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 7]);
    v4Id.m_srcPort = senderTemplate4->GetId();

    if(m_pathState4.find(v4Id) == m_pathState4.end()) return;

    uint32_t number = m_pathState4[v4Id].label;
    m_pathState4.erase(v4Id);
    m_mplsroute.erase(number);
    m_labels[number] = 0;

    packet->AddHeader(ipv4_header); 
    uint32_t hashValue = hash(v4Id, m_hashSeed);

    uint32_t devId = route_vec[hashValue % route_vec.size()];
    m_devices[devId]->Send(packet, m_devices[devId]->GetBroadcast(), 0x0800);
}


void 
SwitchNode::IngressPipelineRSVPTear6(Ptr<Packet> packet, FlowV6Id v6Id, Ipv6Header ipv6_header, std::vector<uint32_t> route_vec){
    // std::cout << "Receive Tear in Switch " << m_nid << std::endl;

    RsvpHeader rsvpHeader;
    packet->PeekHeader(rsvpHeader);

    auto mp = rsvpHeader.GetObjects();

    auto lsp6 = DynamicCast<RsvpLsp6>(mp[((uint16_t)RsvpObject::Session << 8) | 8]);
    uint8_t extend[16];
    lsp6->GetExtend(extend);
    v6Id.m_protocol = extend[0];
    v6Id.m_dstPort = lsp6->GetId();   

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 8]);
    v6Id.m_srcPort = senderTemplate6->GetId();

    if(m_pathState6.find(v6Id) == m_pathState6.end()) return;
    
    uint32_t number = m_pathState6[v6Id].label;
    m_pathState6.erase(v6Id);
    m_mplsroute.erase(number);
    m_labels[number] = 0;
    
    packet->AddHeader(ipv6_header); 
    uint32_t hashValue = hash(v6Id, m_hashSeed);
     
    uint32_t devId = route_vec[hashValue % route_vec.size()];
    m_devices[devId]->Send(packet, m_devices[devId]->GetBroadcast(), 0x86DD);
}

void 
SwitchNode::IngressPipelineRSVPResv4(Ptr<Packet> packet, Ptr<NetDevice> dev, FlowV4Id v4Id, Ipv4Header ipv4_header){
    RsvpHeader rsvpHeader;
    packet->PeekHeader(rsvpHeader);

    auto mp = rsvpHeader.GetObjects();

    std::swap(v4Id.m_srcIP, v4Id.m_dstIP);

    auto lsp4 = DynamicCast<RsvpLsp4>(mp[((uint16_t)RsvpObject::Session << 8) | 7]);
    v4Id.m_protocol = lsp4->GetExtend();
    v4Id.m_dstPort = lsp4->GetId();   

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 7]);
    v4Id.m_srcPort = senderTemplate4->GetId();

    if(m_pathState4.find(v4Id) == m_pathState4.end()){
        std::cout << "Cannot find path state in Switch " << m_nid << std::endl;
        return;
    }
    else if(m_pathState4[v4Id].label != -1){
        std::cout << "Already assign label in switch " << m_nid << std::endl;
        return;
    }

    uint32_t number = GetLabel();
    if(number == -1){
        std::cout << "No entry available" << std::endl;
        return;
    }
    m_labels[number] = 1;

    auto label = DynamicCast<RsvpLabel>(mp[(((uint16_t)RsvpObject::Label << 8) | 1)]);
                
    m_pathState4[v4Id].label = number;

    m_mplsroute[number].newLabel = label->GetLabel();
    m_mplsroute[number].dev = dev;

    if(m_mplsroute.size() > m_maxroute){
        m_maxroute = m_mplsroute.size();
        if(m_maxroute % 10 == 0)
            std::cout << m_maxroute << " routes in switch " << m_nid << std::endl;
    }

    label->SetLabel(number);
    packet->RemoveHeader(rsvpHeader);
    rsvpHeader.AppendObject(label);
    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipv4_header); 

    m_pathState4[v4Id].prev->Send(packet, m_pathState4[v4Id].prev->GetBroadcast(), 0x0800);
}

void 
SwitchNode::IngressPipelineRSVPResv6(Ptr<Packet> packet, Ptr<NetDevice> dev, FlowV6Id v6Id, Ipv6Header ipv6_header){
    // std::cout << "Receive Resv in Switch " << m_nid << std::endl;

    RsvpHeader rsvpHeader;
    packet->PeekHeader(rsvpHeader);

    auto mp = rsvpHeader.GetObjects();

    std::swap(v6Id.m_srcIP, v6Id.m_dstIP);

    auto lsp6 = DynamicCast<RsvpLsp6>(mp[((uint16_t)RsvpObject::Session << 8) | 8]);
    uint8_t extend[16];
    lsp6->GetExtend(extend);
    v6Id.m_protocol = extend[0];
    v6Id.m_dstPort = lsp6->GetId();   

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 8]);
    v6Id.m_srcPort = senderTemplate6->GetId();

    if(m_pathState6.find(v6Id) == m_pathState6.end()){
        std::cout << "Cannot find path state in Switch " << m_nid << std::endl;
        return;
    }
    else if(m_pathState6[v6Id].label != -1){
        std::cout << "Already assign label in switch " << m_nid << std::endl;
        return;
    }

    uint32_t number = GetLabel();
    if(number == -1){
        std::cout << "No entry available" << std::endl;
        return;
    }
    m_labels[number] = 1;

    auto label = DynamicCast<RsvpLabel>(mp[(((uint16_t)RsvpObject::Label << 8) | 1)]);
    m_pathState6[v6Id].label = number;

    m_mplsroute[number].newLabel = label->GetLabel();
    m_mplsroute[number].dev = dev;

    if(m_mplsroute.size() > m_maxroute){
        m_maxroute = m_mplsroute.size();
        if(m_maxroute % 10 == 0)
            std::cout << m_maxroute << " routes in switch " << m_nid << std::endl;
    }

    label->SetLabel(number);
    packet->RemoveHeader(rsvpHeader);
    rsvpHeader.AppendObject(label);
    packet->AddHeader(rsvpHeader); 
    packet->AddHeader(ipv6_header);

    m_pathState6[v6Id].prev->Send(packet, m_pathState6[v6Id].prev->GetBroadcast(), 0x86DD);
}

bool
SwitchNode::IngressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev){
    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;

    FlowV4Id v4Id;
    FlowV6Id v6Id;

    uint8_t proto = 0, ttl = 0;
    std::vector<uint32_t> route_vec;

    if(protocol == 0x0800){
        packet->RemoveHeader(ipv4_header);

        proto = ipv4_header.GetProtocol();
        ttl = ipv4_header.GetTtl();

        v4Id.m_srcIP = ipv4_header.GetSource().Get();
        v4Id.m_dstIP = ipv4_header.GetDestination().Get();
        v4Id.m_protocol = proto;
        
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

        route_vec = m_v6route[dst_pair];

        ipv6_header.SetHopLimit(ttl - 1);  
    }
    else if(protocol == 0x8847){
        return IngressPipelineMPLS(packet, dev);
    }
    else{
        std::cout << "Unknown Protocol for IngressPipeline" << std::endl;
        return false;
    }

    if(route_vec.size() <= 0){
        std::cout << "Unknown Destination for Routing in Switch " << m_nid << std::endl;
        return false;
    }
    if(ttl == 0){
        std::cout << "TTL = 0 for IP in Switch" << std::endl;
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


    Ptr<NetDevice> device;
    if(protocol == 0x0800 && isUser){
        v4Id.m_srcPort = src_port;
        v4Id.m_dstPort = dst_port;   
        packet->AddHeader(ipv4_header);   

        uint32_t hashValue = hash(v4Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        device = m_devices[devId];
    }
    else if(protocol == 0x86DD && isUser){
        v6Id.m_srcPort = src_port;
        v6Id.m_dstPort = dst_port;   
        packet->AddHeader(ipv6_header); 

        uint32_t hashValue = hash(v6Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        device = m_devices[devId];
    }
    else if(proto == 46){
        RsvpHeader rsvpHeader;
        packet->PeekHeader(rsvpHeader);

        switch(rsvpHeader.GetType()){
            case RsvpHeader::Path:
                if(protocol == 0x0800)
                    Simulator::Schedule(NanoSeconds(m_delay), &SwitchNode::IngressPipelineRSVPPath4, this, packet, dev, v4Id, ipv4_header, route_vec);
                else if(protocol == 0x86DD)
                    Simulator::Schedule(NanoSeconds(m_delay), &SwitchNode::IngressPipelineRSVPPath6, this, packet, dev, v6Id, ipv6_header, route_vec);
                return false;
            case RsvpHeader::Resv:
                if(protocol == 0x0800)
                    Simulator::Schedule(NanoSeconds(m_delay), &SwitchNode::IngressPipelineRSVPResv4, this, packet, dev, v4Id, ipv4_header);
                else if(protocol == 0x86DD)
                    Simulator::Schedule(NanoSeconds(m_delay), &SwitchNode::IngressPipelineRSVPResv6, this, packet, dev, v6Id, ipv6_header);
                return false;
            case RsvpHeader::PathTear:
                if(protocol == 0x0800)
                    Simulator::Schedule(NanoSeconds(m_delay), &SwitchNode::IngressPipelineRSVPTear4, this, packet, v4Id, ipv4_header, route_vec);
                else if(protocol == 0x86DD)
                    Simulator::Schedule(NanoSeconds(m_delay), &SwitchNode::IngressPipelineRSVPTear6, this, packet, v6Id, ipv6_header, route_vec);
                return false;
            default:
                std::cout << "Unknown type " << int(rsvpHeader.GetType()) << " in RSVP" << std::endl;
                return false;
        }
    }
    else{
        std::cout << "Unknown Protocol " << int(proto) << std::endl;
        return false;
    }

    if(m_userSize + packet->GetSize() <= m_userThd && isUser){
        m_userSize += packet->GetSize();
        return device->Send(packet, device->GetBroadcast(), protocol);
    }

    if(isUser){
        m_drops += 1;
        if(m_drops % 100 == 0)
            std::cout << "User packet drop 100 in Switch " << m_nid << std::endl;
    }
    return false;
}


void 
SwitchNode::CreateRsvpTear4(FlowV4Id id, Ptr<NetDevice> dev){
    if(m_pathState4.find(id) == m_pathState4.end()) return;

    if(Simulator::Now().GetNanoSeconds() - m_pathState4[id].time 
            < m_pathState4[id].timeout * 1000000)
        return;

    uint32_t number = m_pathState4[id].label;
    m_pathState4.erase(id);
    m_mplsroute.erase(number);
    m_labels[number] = 0;

    Ptr<Packet> packet = Create<Packet>();
    Ipv4Header ipHeader;

    ipHeader.SetSource(Ipv4Address(id.m_srcIP));
    ipHeader.SetDestination(Ipv4Address(id.m_dstIP));
    ipHeader.SetProtocol(46);
    ipHeader.SetTtl(64);

    RsvpHeader rsvpHeader;
    rsvpHeader.SetType(RsvpHeader::PathTear);
    rsvpHeader.SetTtl(64);

    auto lsp4 = DynamicCast<RsvpLsp4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Session << 8) | 7));
    lsp4->SetAddress(Ipv4Address(id.m_dstIP));
    lsp4->SetId(id.m_dstPort);
    lsp4->SetExtend(id.m_protocol);
    rsvpHeader.AppendObject(lsp4);

    auto hop4 = DynamicCast<RsvpHop4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Hop << 8) | 1));
    hop4->SetAddress(Ipv4Address(id.m_dstIP));
    rsvpHeader.AppendObject(hop4);

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::FilterSpec << 8) | 7));
    senderTemplate4->SetAddress(Ipv4Address(id.m_srcIP));
    senderTemplate4->SetId(id.m_srcPort);
    rsvpHeader.AppendObject(senderTemplate4);

    ipHeader.SetPayloadSize(rsvpHeader.GetSerializedSize());
    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipHeader);

    dev->Send(packet, dev->GetBroadcast(), 0x0800);
}

void
SwitchNode::CreateRsvpTear6(FlowV6Id id, Ptr<NetDevice> dev){
    if(m_pathState6.find(id) == m_pathState6.end()) return;

    if(Simulator::Now().GetNanoSeconds() - m_pathState6[id].time 
            < m_pathState6[id].timeout * 1000000)
        return;

    uint32_t number = m_pathState6[id].label;
    m_pathState6.erase(id);
    m_mplsroute.erase(number);
    m_labels[number] = 0;

    Ptr<Packet> packet = Create<Packet>();
    Ipv6Header ipHeader;

    ipHeader.SetSource(PairToIpv6(
        std::pair<uint64_t, uint64_t>(id.m_srcIP[0], id.m_srcIP[1])));
    ipHeader.SetDestination(PairToIpv6(
        std::pair<uint64_t, uint64_t>(id.m_dstIP[0], id.m_dstIP[1])));
    ipHeader.SetNextHeader(46); 
    ipHeader.SetHopLimit(64);

    RsvpHeader rsvpHeader;
    rsvpHeader.SetType(RsvpHeader::PathTear);
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

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(RsvpObject::CreateObject(((uint16_t)RsvpObject::FilterSpec << 8) | 8));
    senderTemplate6->SetAddress(ipHeader.GetSource());
    senderTemplate6->SetId(id.m_srcPort);
    rsvpHeader.AppendObject(senderTemplate6);

    ipHeader.SetPayloadLength(rsvpHeader.GetSerializedSize());
    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipHeader);

    dev->Send(packet, dev->GetBroadcast(), 0x86DD);
}

uint32_t 
SwitchNode::GetLabel(){
    for(uint32_t i = m_labelMin;i != m_labelMax;++i){
        if(m_labels[i] == 0)
            return i;
    }
    return -1;
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
