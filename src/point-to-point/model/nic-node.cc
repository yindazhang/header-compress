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

#include "compress-ipv4-header.h"
#include "compress-ipv6-header.h"
#include "compress-tcp-header.h"

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
NICNode::SetSettig(uint32_t setting)
{
    m_setting = setting;
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

    if(proto == 6 || proto == 17 || protocol == 0x8847){
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
    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;
    MplsHeader mpls_header;

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
        MplsHeader mpls_header;
        packet->RemoveHeader(mpls_header);
        uint32_t label = mpls_header.GetLabel();
        if(m_decompress.find(label) == m_decompress.end()){
            std::cout << "Cannot find label " << label << " in NIC " << m_nid << std::endl;
            return false;
        }
        
        protocol = m_decompress[label].protocol;
        if(protocol == 0x0800){
            CompressIpv4Header compressIpv4Header;
            packet->RemoveHeader(compressIpv4Header);
            ipv4_header = compressIpv4Header.GetIpv4Header();
            ipv4_header.SetTtl(mpls_header.GetTtl());
            ipv4_header.SetEcn(Ipv4Header::EcnType(mpls_header.GetExp()));

            FlowV4Id v4Id = m_decompress[label].v4Id;
            ipv4_header.SetSource(Ipv4Address(v4Id.m_srcIP));
            ipv4_header.SetDestination(Ipv4Address(v4Id.m_dstIP));
            ipv4_header.SetProtocol(v4Id.m_protocol);
            if(v4Id.m_protocol == 6){
                TcpHeader tcpHeader;
                CompressTcpHeader compressTcpHeader;
                packet->RemoveHeader(compressTcpHeader);
                tcpHeader = compressTcpHeader.GetTcpHeader();
                tcpHeader.SetSourcePort(v4Id.m_srcPort);
                tcpHeader.SetDestinationPort(v4Id.m_dstPort);
                packet->AddHeader(tcpHeader);
            }
            else if(v4Id.m_protocol == 17){
                UdpHeader udpHeader;
                udpHeader.SetSourcePort(v4Id.m_srcPort);
                udpHeader.SetDestinationPort(v4Id.m_dstPort);
                packet->AddHeader(udpHeader);
            }
            else{
                std::cout << "Unknown protocol " << v4Id.m_protocol << " in NIC " << m_nid << std::endl;
                return false;
            }
            packet->AddHeader(ipv4_header);
        }
        else if(protocol == 0x86DD){
            CompressIpv6Header compressIpv6Header;
            packet->RemoveHeader(compressIpv6Header);
            ipv6_header = compressIpv6Header.GetIpv6Header();
            ipv6_header.SetHopLimit(mpls_header.GetTtl());
            ipv6_header.SetEcn(Ipv6Header::EcnType(mpls_header.GetExp()));

            FlowV6Id v6Id = m_decompress[label].v6Id;
            ipv6_header.SetSource(PairToIpv6(
                std::pair<uint64_t, uint64_t>(v6Id.m_srcIP[0], v6Id.m_srcIP[1])));
            ipv6_header.SetDestination(PairToIpv6(
                std::pair<uint64_t, uint64_t>(v6Id.m_dstIP[0], v6Id.m_dstIP[1])));
            ipv6_header.SetNextHeader(v6Id.m_protocol);
            if(v6Id.m_protocol == 6){
                TcpHeader tcpHeader;
                CompressTcpHeader compressTcpHeader;
                packet->RemoveHeader(compressTcpHeader);
                tcpHeader = compressTcpHeader.GetTcpHeader();
                tcpHeader.SetSourcePort(v6Id.m_srcPort);
                tcpHeader.SetDestinationPort(v6Id.m_dstPort);
                packet->AddHeader(tcpHeader);
            }
            else if(v6Id.m_protocol == 17){
                UdpHeader udpHeader;
                udpHeader.SetSourcePort(v6Id.m_srcPort);
                udpHeader.SetDestinationPort(v6Id.m_dstPort);
                packet->AddHeader(udpHeader);
            }
            else{
                std::cout << "Unknown protocol " << v6Id.m_protocol << " in NIC " << m_nid << std::endl;
                return false;
            }
            packet->AddHeader(ipv6_header);
        }

        if(m_userSize + packet->GetSize() <= m_userThd){
            m_userSize += packet->GetSize();
            return m_devices[2]->Send(packet, m_devices[2]->GetBroadcast(), protocol);
        }
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

    Ptr<NetDevice> device;
    if(protocol == 0x0800 && isUser){
        v4Id.m_srcPort = src_port;
        v4Id.m_dstPort = dst_port;    

        // TODO: Dynamic threshold
        if(m_setting){
            m_v4count[v4Id] += 1;
            if(m_compress4.find(v4Id) != m_compress4.end()){
                if(proto == 6){
                    TcpHeader tcpHeader;
                    packet->RemoveHeader(tcpHeader);
                    CompressTcpHeader compressTcpHeader;
                    compressTcpHeader.SetTcpHeader(tcpHeader);
                    packet->AddHeader(compressTcpHeader);
                }
                else{
                    UdpHeader udpHeader;
                    packet->RemoveHeader(udpHeader);
                }
                CompressIpv4Header compressIpv4Header;
                compressIpv4Header.SetIpv4Header(ipv4_header);
                packet->AddHeader(compressIpv4Header);

                mpls_header.SetLabel(m_compress4[v4Id].label);
                mpls_header.SetExp(MplsHeader::ECN_ECT1);
                mpls_header.SetTtl(64);
                packet->AddHeader(mpls_header);

                if(m_userSize + packet->GetSize() <= m_userThd){
                    m_userSize += packet->GetSize();
                    return m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x8847);
                }
                return false;
            }
            else if(m_v4count[v4Id] == m_threshold && dev == m_devices[2])
                Simulator::Schedule(NanoSeconds(1), &NICNode::CreateRsvpPath4, this, v4Id);
        }

        packet->AddHeader(ipv4_header);
        uint32_t hashValue = hash(v4Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        device = m_devices[devId];
    }
    else if(protocol == 0x86DD && isUser){
        v6Id.m_srcPort = src_port;
        v6Id.m_dstPort = dst_port;   

        // TODO: Dynamic threshold
        if(m_setting){
            m_v6count[v6Id] += 1;
            if(m_compress6.find(v6Id) != m_compress6.end()){
                if(proto == 6){
                    TcpHeader tcpHeader;
                    packet->RemoveHeader(tcpHeader);
                    CompressTcpHeader compressTcpHeader;
                    compressTcpHeader.SetTcpHeader(tcpHeader);
                    packet->AddHeader(compressTcpHeader);
                }
                else{
                    UdpHeader udpHeader;
                    packet->RemoveHeader(udpHeader);
                }
                CompressIpv6Header compressIpv6Header;
                compressIpv6Header.SetIpv6Header(ipv6_header);
                packet->AddHeader(compressIpv6Header);

                mpls_header.SetLabel(m_compress6[v6Id].label);
                mpls_header.SetExp(MplsHeader::ECN_ECT1);
                mpls_header.SetTtl(64);
                packet->AddHeader(mpls_header);

                if(m_userSize + packet->GetSize() <= m_userThd){
                    m_userSize += packet->GetSize();
                    return m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x8847);
                }
                return false;
            }
            else if(m_v6count[v6Id] == m_threshold && dev == m_devices[2])
                Simulator::Schedule(NanoSeconds(1), &NICNode::CreateRsvpPath6, this, v6Id);
        }

        packet->AddHeader(ipv6_header);
        uint32_t hashValue = hash(v6Id, m_hashSeed);
        uint32_t devId = route_vec[hashValue % route_vec.size()];
        device = m_devices[devId];  
    }
    else if(proto == 46){
        RsvpHeader rsvpHeader;
        packet->PeekHeader(rsvpHeader);

        if(rsvpHeader.GetType() == RsvpHeader::Path){
            if(protocol == 0x0800)
                return CreateRsvpResv4(v4Id, rsvpHeader);
            else if(protocol == 0x86DD)
                return CreateRsvpResv6(v6Id, rsvpHeader);
        }
        else if(rsvpHeader.GetType() == RsvpHeader::Resv){
            auto mp = rsvpHeader.GetObjects();
            if(protocol == 0x0800){
                auto tmp = v4Id.m_srcIP;
                v4Id.m_srcIP = v4Id.m_dstIP;
                v4Id.m_dstIP = tmp;

                auto lsp4 = DynamicCast<RsvpLsp4>(mp[((uint16_t)RsvpObject::Session << 8) | 7]);
                v4Id.m_protocol = lsp4->GetExtend();
                v4Id.m_dstPort = lsp4->GetId();   

                auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 7]);
                v4Id.m_srcPort = senderTemplate4->GetId();

                if(m_compress4.find(v4Id) != m_compress4.end()){
                    std::cout << "Already assign label in NIC " << m_nid << std::endl;
                    return false;
                }

                auto label = DynamicCast<RsvpLabel>(mp[(((uint16_t)RsvpObject::Label << 8) | 1)]);
                m_compress4[v4Id].label = label->GetLabel();
            }
            else if(protocol == 0x86DD){
                uint64_t tmp[2] = {v6Id.m_srcIP[0], v6Id.m_srcIP[1]};
                v6Id.m_srcIP[0] = v6Id.m_dstIP[0];
                v6Id.m_srcIP[1] = v6Id.m_dstIP[1];
                v6Id.m_dstIP[0] = tmp[0];
                v6Id.m_dstIP[1] = tmp[1];

                auto lsp6 = DynamicCast<RsvpLsp6>(mp[((uint16_t)RsvpObject::Session << 8) | 8]);
                uint8_t extend[16];
                lsp6->GetExtend(extend);
                v6Id.m_protocol = extend[0];
                v6Id.m_dstPort = lsp6->GetId();   

                auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 8]);
                v6Id.m_srcPort = senderTemplate6->GetId();

                if(m_compress6.find(v6Id) != m_compress6.end()){
                    std::cout << "Already assign label in NIC " << m_nid << std::endl;
                    return false;
                }

                auto label = DynamicCast<RsvpLabel>(mp[(((uint16_t)RsvpObject::Label << 8) | 1)]);
                m_compress6[v6Id].label = label->GetLabel();
            }
            return false;
        }
        else{
            std::cout << "Unknown type " << int(rsvpHeader.GetType()) << " in RSVP" << std::endl;
        }
        return false;
    }
    else{
        std::cout << "TODO: Handle Other packets in NIC" << std::endl;
        return false;
    }

    if(m_userSize + packet->GetSize() <= m_userThd && isUser){
        m_userSize += packet->GetSize();
        return device->Send(packet, device->GetBroadcast(), protocol);
    }

    
    if(isUser){
        std::cout << "User packet drop in NIC " << m_nid << std::endl;
        if(device == m_devices[1])
            std::cout << "To server traffic" << std::endl;
        if(device == m_devices[2])
            std::cout << "From server traffic" << std::endl;
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

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(RsvpObject::CreateObject(((uint16_t)RsvpObject::FilterSpec << 8) | 7));
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

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(RsvpObject::CreateObject(((uint16_t)RsvpObject::FilterSpec << 8) | 8));
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

bool
NICNode::CreateRsvpResv4(FlowV4Id id, RsvpHeader pathHeader){
    auto mp = pathHeader.GetObjects();

    auto lsp4 = DynamicCast<RsvpLsp4>(mp[((uint16_t)RsvpObject::Session << 8) | 7]);
    id.m_protocol = lsp4->GetExtend();
    id.m_dstPort = lsp4->GetId();   

    auto senderTemplate4 = DynamicCast<RsvpFilterSpec4>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 7]);
    id.m_srcPort = senderTemplate4->GetId();

    if(m_pathState4.find(id) != m_pathState4.end()){
        std::cout << "Path already exists in NIC " << m_nid << std::endl;
        return false;
    }

    uint32_t number = GetLabel();
    if(number == -1){
        std::cout << "No entry available" << std::endl;
        return false;
    }
    m_pathState4[id] = number;
    m_labels[number] = 1;
    
    if(m_decompress.find(number) != m_decompress.end())
        std::cout << "Find decompress in NIC " << m_nid << std::endl;
    m_decompress[number].protocol = 0x0800; 
    m_decompress[number].v4Id = id; 

    Ptr<Packet> packet = Create<Packet>();
    Ipv4Header ipHeader;

    ipHeader.SetSource(Ipv4Address(id.m_dstIP));
    ipHeader.SetDestination(Ipv4Address(id.m_srcIP));
    ipHeader.SetProtocol(46);
    ipHeader.SetTtl(64);

    RsvpHeader rsvpHeader;
    rsvpHeader.SetType(RsvpHeader::Resv);
    rsvpHeader.SetTtl(64);

    rsvpHeader.AppendObject(lsp4);

    auto hop4 = DynamicCast<RsvpHop4>(mp[((uint16_t)RsvpObject::Hop << 8) | 1]);
    rsvpHeader.AppendObject(hop4);

    auto timeValue = DynamicCast<RsvpTimeValue>(mp[((uint16_t)RsvpObject::TimeValue << 8) | 1]);
    rsvpHeader.AppendObject(timeValue);

    auto style = DynamicCast<RsvpStyle>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Style << 8) | 1));
    style->SetOption(RsvpStyle::FixedFilter);
    rsvpHeader.AppendObject(style);

    auto flowSpec = DynamicCast<RsvpFlowSpec>(RsvpObject::CreateObject(((uint16_t)RsvpObject::FlowSpec << 8) | 2));
    flowSpec->SetMaxSize(1500);
    rsvpHeader.AppendObject(flowSpec);

    rsvpHeader.AppendObject(senderTemplate4);

    auto label = DynamicCast<RsvpLabel>(RsvpObject::CreateObject((((uint16_t)RsvpObject::Label << 8) | 1)));
    label->SetLabel(number);
    rsvpHeader.AppendObject(label);

    ipHeader.SetPayloadSize(rsvpHeader.GetSerializedSize());
    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipHeader);

    return m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0800);
}

bool
NICNode::CreateRsvpResv6(FlowV6Id id, RsvpHeader pathHeader){
    auto mp = pathHeader.GetObjects();

    auto lsp6 = DynamicCast<RsvpLsp6>(mp[((uint16_t)RsvpObject::Session << 8) | 8]);
    uint8_t extend[16];
    lsp6->GetExtend(extend);            
    id.m_protocol = extend[0];
    id.m_dstPort = lsp6->GetId();   

    auto senderTemplate6 = DynamicCast<RsvpFilterSpec6>(mp[((uint16_t)RsvpObject::FilterSpec << 8) | 8]);
    id.m_srcPort = senderTemplate6->GetId();

    if(m_pathState6.find(id) != m_pathState6.end()){
        std::cout << "Path already exists in NIC " << m_nid << std::endl;
        return false;
    }

    uint32_t number = GetLabel();
    if(number == -1){
        std::cout << "No entry available" << std::endl;
        return false;
    }
    m_pathState6[id] = number;
    m_labels[number] = 1;

    if(m_decompress.find(number) != m_decompress.end())
        std::cout << "Find decompress in NIC " << m_nid << std::endl;
    m_decompress[number].protocol = 0x86DD; 
    m_decompress[number].v6Id = id; 

    Ptr<Packet> packet = Create<Packet>();
    Ipv6Header ipHeader;

    ipHeader.SetSource(PairToIpv6(
        std::pair<uint64_t, uint64_t>(id.m_dstIP[0], id.m_dstIP[1])));
    ipHeader.SetDestination(PairToIpv6(
        std::pair<uint64_t, uint64_t>(id.m_srcIP[0], id.m_srcIP[1])));
    ipHeader.SetNextHeader(46); 
    ipHeader.SetHopLimit(64);

    RsvpHeader rsvpHeader;
    rsvpHeader.SetType(RsvpHeader::Resv);
    rsvpHeader.SetTtl(64);

    rsvpHeader.AppendObject(lsp6);

    auto hop6 = DynamicCast<RsvpHop6>(mp[((uint16_t)RsvpObject::Hop << 8) | 2]);
    rsvpHeader.AppendObject(hop6);

    auto timeValue = DynamicCast<RsvpTimeValue>(mp[((uint16_t)RsvpObject::TimeValue << 8) | 1]);
    rsvpHeader.AppendObject(timeValue);

    auto style = DynamicCast<RsvpStyle>(RsvpObject::CreateObject(((uint16_t)RsvpObject::Style << 8) | 1));
    style->SetOption(RsvpStyle::FixedFilter);
    rsvpHeader.AppendObject(style);

    auto flowSpec = DynamicCast<RsvpFlowSpec>(RsvpObject::CreateObject(((uint16_t)RsvpObject::FlowSpec << 8) | 2));
    flowSpec->SetMaxSize(1500);
    rsvpHeader.AppendObject(flowSpec);

    rsvpHeader.AppendObject(senderTemplate6);

    auto label = DynamicCast<RsvpLabel>(RsvpObject::CreateObject((((uint16_t)RsvpObject::Label << 8) | 1)));
    label->SetLabel(number);
    rsvpHeader.AppendObject(label);

    ipHeader.SetPayloadLength(rsvpHeader.GetSerializedSize());
    packet->AddHeader(rsvpHeader);
    packet->AddHeader(ipHeader);

    return m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x86DD);
}

uint32_t 
NICNode::GetLabel(){
    for(uint32_t i = m_labelMin;i != m_labelMax;++i){
        if(m_labels[i] == 0)
            return i;
    }
    return -1;
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