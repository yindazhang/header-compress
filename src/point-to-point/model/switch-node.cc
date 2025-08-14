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
#include "pfc-header.h"
#include "ppp-header.h"
#include "mpls-header.h"
#include "port-header.h"
#include "vxlan-header.h"
#include "command-header.h"

#include "compress-ip-header.h"

#include "ipv4-tag.h"
#include "ipv6-tag.h"
#include "packet-tag.h"

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
    Simulator::Schedule(Seconds(4), &SwitchNode::CheckEcnCount, this);
}

SwitchNode::~SwitchNode()
{
    std::string out_file = m_output + ".node";
    FILE* fout = fopen(out_file.c_str(), "a");
    fprintf(fout, "%d,%lu,%lu,%lu\n", m_nid, m_drops, m_ecnCount, m_pfcCount);
    fclose(fout);
}

void
SwitchNode::CheckEcnCount()
{
    m_ecnCount = 0;
    for(auto dev : m_devices){
        Ptr<PointToPointNetDevice> p = DynamicCast<PointToPointNetDevice>(dev);
        if(p){
            Ptr<PointToPointQueue> q = DynamicCast<PointToPointQueue>(p->GetQueue());
            if(q){
                m_ecnCount += q->GetEcnCount();
            }
        }
    }
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
    return IngressPipeline(packet, protocol, device);
}

void
SwitchNode::SetECMPHash(uint32_t hashSeed)
{
    m_hashSeed = hashSeed;
}

void
SwitchNode::SetSetting(uint32_t setting)
{
    m_setting = setting;
}

void
SwitchNode::SetPFC(uint32_t pfc)
{
    m_pfc = pfc;
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
SwitchNode::SetNextNode(uint16_t devId, uint16_t nodeId)
{
    m_node[devId] = nodeId;
}

void
SwitchNode::SetOutput(std::string output)
{
    m_output = output;
    std::string out_file = m_output + ".node";
    FILE* fout = fopen(out_file.c_str(), "w");
    fclose(fout);
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

void
SwitchNode::AddControlRouteTo(uint16_t id, uint32_t devId)
{
    m_idroute[id].push_back(devId);
}


uint16_t
SwitchNode::GetNextDev(FlowV4Id id)
{
    const std::vector<uint32_t>& route_vec = m_v4route[id.m_dstIP];
    if(route_vec.size() == 0){
        std::cout << "Cannot find NextDev for Ipv6" << std::endl;
        return 0xffff;
    }

    uint32_t hashValue = 0;
    if(route_vec.size() > 1)
        hashValue = id.hash(m_hashSeed);
    return route_vec[hashValue % route_vec.size()];
}

uint16_t
SwitchNode::GetNextDev(FlowV6Id id)
{
    const std::vector<uint32_t>& route_vec = m_v6route[
        std::pair<uint64_t, uint64_t>(id.m_dstIP[0], id.m_dstIP[1])];
    if(route_vec.size() == 0){
        std::cout << "Cannot find NextDev for Ipv6" << std::endl;
        return 0xffff;
    }

    uint32_t hashValue = 0;
    if(route_vec.size() > 1)
        hashValue = id.hash(m_hashSeed);
    return route_vec[hashValue % route_vec.size()];
}

uint16_t
SwitchNode::GetNextNode(uint16_t devId)
{
    if(m_node.find(devId) == m_node.end()){
        std::cout << "Fail to find dev" << std::endl;
        return 0xffff;
    }
    return m_node[devId];
}

void
SwitchNode::MarkNicDevice(Ptr<NetDevice> device)
{
    m_nicDevices.insert(device);
}

Ptr<Packet>
SwitchNode::EgressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev){
    PppHeader ppp;
    packet->RemoveHeader(ppp);

    if(m_setting == 3 && (protocol == 0x0800 || protocol == 0x86DD)){
        if(m_rohcCom.find(dev) == m_rohcCom.end()){
            m_rohcCom[dev] = CreateObject<RohcCompressor>();
        }
        protocol = m_rohcCom[dev]->Process(packet, protocol);
        ppp.SetProtocol(PointToPointNetDevice::EtherToPpp(protocol));
    }

    if(protocol != 0x0170 && protocol != 0x8808){
        PacketTag packetTag;
        if(!packet->PeekPacketTag(packetTag))
            std::cerr << "Fail to find packetTag" << std::endl;
        m_userSize -= packetTag.GetSize();
        m_ingressSize[packetTag.GetNetDevice()] -= packetTag.GetSize();
        if(m_userSize < 0){
            std::cout << "Error for userSize in Switch " << m_nid << std::endl;
            std::cout << "Egress size : " << m_userSize << std::endl;
        }
        if(m_ingressSize[packetTag.GetNetDevice()] < 0){
            std::cout << "Error for ingressSize in Switch " << m_nid << std::endl;
            std::cout << "Egress size : " << m_ingressSize[packetTag.GetNetDevice()] << std::endl;
        }
        if(m_pause[packetTag.GetNetDevice()]){
            if(m_ingressSize[packetTag.GetNetDevice()] < m_resumeNicThd || (m_nicDevices.find(packetTag.GetNetDevice()) == m_nicDevices.end() && m_ingressSize[packetTag.GetNetDevice()] < m_resumeThd)){
                m_pause[packetTag.GetNetDevice()] = false;
                Simulator::Schedule(NanoSeconds(1), &SwitchNode::SendPFC, this, packetTag.GetNetDevice(), false);
            }
        }
    }

    packet->AddHeader(ppp);
    return packet;
}

bool
SwitchNode::IngressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev){
    if(protocol != 0x0170){
        if(m_userSize + packet->GetSize() > m_userThd){
            m_drops += 1;
            // if(m_drops % 100 == 0)
            std::cout << "User packet drop in Switch " << m_nid << std::endl;
            return false;
        }
        else{
            PacketTag packetTag;
            packetTag.SetSize(packet->GetSize());
            packetTag.SetNetDevice(dev);
            packet->ReplacePacketTag(packetTag);
            m_userSize += packet->GetSize();
            m_ingressSize[dev] += packet->GetSize();

            if(!m_pause[dev] && m_pfc){
                if(m_ingressSize[dev] > m_pfcThd || (m_nicDevices.find(dev) != m_nicDevices.end() && m_ingressSize[dev] > m_pfcNicThd)){
                    m_pfcCount += 1;
                    m_pause[dev] = true;
                    Simulator::Schedule(NanoSeconds(1), &SwitchNode::SendPFC, this, dev, true);
                }
            }
        }
    }

    if(protocol == 0x0172){
        if(m_rohcDecom.find(dev) == m_rohcDecom.end()){
            m_rohcDecom[dev] = CreateObject<RohcDecompressor>();
        }
        protocol = m_rohcDecom[dev]->Process(packet);
    }

    uint8_t ttl = 64;
    uint32_t devId;

    if(protocol == 0x0800){
        auto v4Id = getFlowV4Id(packet);
        devId = GetNextDev(v4Id);

        Ipv4Header ipv4_header;
        packet->RemoveHeader(ipv4_header);
        ttl = ipv4_header.GetTtl();
        ipv4_header.SetTtl(ttl - 1);
        packet->AddHeader(ipv4_header);
    }
    else if(protocol == 0x86DD){
        auto v6Id = getFlowV6Id(packet);
        devId = GetNextDev(v6Id);

        Ipv6Header ipv6_header;
        packet->RemoveHeader(ipv6_header);
        ttl = ipv6_header.GetHopLimit();
        ipv6_header.SetHopLimit(ttl - 1);
        packet->AddHeader(ipv6_header);
    }
    else if(protocol == 0x0170){
        CommandHeader cmd;
        packet->PeekHeader(cmd);

        if(cmd.GetDestinationId() == m_nid){
            switch(cmd.GetType()){
                case CommandHeader::SwitchUpdate :
                    Simulator::Schedule(NanoSeconds(COMMAND_DELAY), &SwitchNode::UpdateMplsRoute, this, cmd);
                    return true;
                default : std::cout << "Unknown Type" << std::endl; return true;
            }
            return true;
        }
        else{
            if(m_idroute.find(cmd.GetDestinationId()) == m_idroute.end()){
                std::cout << "Fail to find route for command dst " << cmd.GetDestinationId() << " in " << m_nid << std::endl;
                return false;
            }

            const std::vector<uint32_t>& route_vec = m_idroute[cmd.GetDestinationId()];
            devId = route_vec[rand() % route_vec.size()];
        }
    }
    else if(protocol == 0x8847){
        MplsHeader mpls_header;
        packet->RemoveHeader(mpls_header);

        uint8_t ttl = mpls_header.GetTtl();
        if(ttl == 0){
            std::cout << "TTL = 0 for MPLS" << std::endl;
            return false;
        }
        mpls_header.SetTtl(ttl - 1);

        uint16_t label = mpls_header.GetLabel();
        if(m_mplsroute.find(label) == m_mplsroute.end()){
            std::cout << "Unknown Destination for MPLS Routing in Switch " << m_nid << " for label " << label << std::endl;
            return false;
        }

        mpls_header.SetLabel(m_mplsroute[label].first);
        packet->AddHeader(mpls_header);

        if(!m_devices[m_mplsroute[label].second]->Send(packet, m_devices[m_mplsroute[label].second]->GetBroadcast(), 0x8847)){
            std::cout << "Fail to send packet for MPLS in SwitchNode" << std::endl;
            return false;
        }
        return true;
    }
    else if(protocol == 0x0171){
        Ipv4Tag ipv4Tag;
        Ipv6Tag ipv6Tag;

        Ipv4Header ipv4_header;
        Ipv6Header ipv6_header;
        PortHeader port_header;

        if (packet->PeekPacketTag(ipv4Tag)){
            ipv4Tag.GetHeader(ipv4_header, port_header);

            FlowV4Id v4Id;
            v4Id.m_srcIP = ipv4_header.GetSource().Get();
            v4Id.m_dstIP = ipv4_header.GetDestination().Get();
            v4Id.m_protocol = ipv4_header.GetProtocol();
            v4Id.m_srcPort = port_header.GetSourcePort();
            v4Id.m_dstPort= port_header.GetDestinationPort();

            devId = GetNextDev(v4Id);
        }
        else if(packet->PeekPacketTag(ipv6Tag)){
            ipv6Tag.GetHeader(ipv6_header, port_header);

            auto src_pair = Ipv6ToPair(ipv6_header.GetSource());
            auto dst_pair = Ipv6ToPair(ipv6_header.GetDestination());

            FlowV6Id v6Id;
            v6Id.m_srcIP[0] = src_pair.first;
            v6Id.m_srcIP[1] = src_pair.second;
            v6Id.m_dstIP[0] = dst_pair.first;
            v6Id.m_dstIP[1] = dst_pair.second;
            v6Id.m_protocol = ipv6_header.GetNextHeader();
            v6Id.m_srcPort = port_header.GetSourcePort();
            v6Id.m_dstPort= port_header.GetDestinationPort();

            devId = GetNextDev(v6Id);
        }
        else{
            std::cout << "Fail to find tag" << std::endl;
            return false;
        }
    }
    else{
        std::cout << "Unknown Protocol for IngressPipeline" << std::endl;
        return false;
    }


    if(devId == 0xffff){
        std::cout << "Fail to get next dev" << std::endl;
        return false;
    }
    if(ttl == 0){
        std::cout << "TTL = 0 for IP in Switch" << std::endl;
        return false;
    }

    Ptr<NetDevice> device = m_devices[devId];
    if(!device->Send(packet, device->GetBroadcast(), protocol)){
        std::cout << "Fail to send packet in SwitchNode" << std::endl;
        return false;
    }
    return true;
}

void
SwitchNode::UpdateMplsRoute(CommandHeader cmd)
{
    m_mplsroute[cmd.GetLabel()] = std::pair<uint16_t, uint16_t>(cmd.GetNewLabel(), cmd.GetPort());
}

void 
SwitchNode::SendPFC(Ptr<NetDevice> dev, bool pause)
{
    Ptr<Packet> packet = Create<Packet>();
    PfcHeader pfc_header;
    if(pause) pfc_header.SetPause(2);
    else pfc_header.SetResume(2);
    packet->AddHeader(pfc_header);

    SocketPriorityTag tag;
    tag.SetPriority(0);
    packet->ReplacePacketTag(tag);
    if(!dev->Send(packet, dev->GetBroadcast(), 0x8808))
        std::cout << "Drop of PFC" << std::endl;
}

} // namespace ns3

