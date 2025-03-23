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
#include "port-header.h"
#include "command-header.h"

#include "ipv4-tag.h"
#include "ipv6-tag.h"
#include "port-tag.h"

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
    std::string out_file;
    FILE* fout;

    out_file = m_output + ".drop";
    fout = fopen(out_file.c_str(), "a");
    fprintf(fout, "%d,%lld\n", m_nid, m_drops);
    fclose(fout);

    out_file = m_output + ".ecn";
    fout = fopen(out_file.c_str(), "a");
    fprintf(fout, "%d,%lld\n", m_nid, m_ecnCount);
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

    std::string out_file;
    FILE* fout;

    out_file = m_output + ".drop";
    fout = fopen(out_file.c_str(), "w");
    fclose(fout);

    out_file = m_output + ".ecn";
    fout = fopen(out_file.c_str(), "w");
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
        return -1;
    }

    uint32_t hashValue = 0;
    if(route_vec.size() > 1)
        hashValue = hash(id, m_hashSeed);
    return route_vec[hashValue % route_vec.size()];
}

uint16_t 
SwitchNode::GetNextDev(FlowV6Id id)
{
    const std::vector<uint32_t>& route_vec = m_v6route[
        std::pair<uint64_t, uint64_t>(id.m_dstIP[0], id.m_dstIP[1])];
    if(route_vec.size() == 0){
        std::cout << "Cannot find NextDev for Ipv6" << std::endl;
        return -1;
    }

    uint32_t hashValue = 0;
    if(route_vec.size() > 1)
        hashValue = hash(id, m_hashSeed);
    return route_vec[hashValue % route_vec.size()];
}

uint16_t 
SwitchNode::GetNextNode(uint16_t devId)
{
    if(m_node.find(devId) == m_node.end()){
        std::cout << "Fail to find dev" << std::endl;
        return -1;
    }
    return m_node[devId];
}

Ptr<Packet>
SwitchNode::EgressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev){
    PppHeader ppp;
    packet->RemoveHeader(ppp);

    // std::cout << packet->GetSize() << std::endl;
    if(protocol == 0x0800 || protocol == 0x86DD || protocol == 0x8847 || protocol == 0x0171){
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
SwitchNode::IngressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev){
    uint8_t ttl = 64;
    
    uint32_t devId;

    if(protocol == 0x0800){
        Ipv4Header ipv4_header;
        packet->RemoveHeader(ipv4_header);

        ttl = ipv4_header.GetTtl();

        FlowV4Id v4Id;
        v4Id.m_srcIP = ipv4_header.GetSource().Get();
        v4Id.m_dstIP = ipv4_header.GetDestination().Get();
        v4Id.m_protocol = ipv4_header.GetProtocol();
        
        PortHeader port_header;
        packet->PeekHeader(port_header);
        v4Id.m_srcPort = port_header.GetSourcePort();
        v4Id.m_dstPort= port_header.GetDestinationPort();

        devId = GetNextDev(v4Id);

        ipv4_header.SetTtl(ttl - 1);  
        packet->AddHeader(ipv4_header);       
    }
    else if(protocol == 0x86DD){
        Ipv6Header ipv6_header;
        packet->RemoveHeader(ipv6_header);

        ttl = ipv6_header.GetHopLimit();

        auto src_pair = Ipv6ToPair(ipv6_header.GetSource());
        auto dst_pair = Ipv6ToPair(ipv6_header.GetDestination());

        FlowV6Id v6Id;
        v6Id.m_srcIP[0] = src_pair.first;
        v6Id.m_srcIP[1] = src_pair.second;
        v6Id.m_dstIP[0] = dst_pair.first;
        v6Id.m_dstIP[1] = dst_pair.second;
        v6Id.m_protocol = ipv6_header.GetNextHeader();

        PortHeader port_header;
        packet->PeekHeader(port_header);
        v6Id.m_srcPort = port_header.GetSourcePort();
        v6Id.m_dstPort= port_header.GetDestinationPort();

        devId = GetNextDev(v6Id);

        ipv6_header.SetHopLimit(ttl - 1); 
        packet->AddHeader(ipv6_header); 
    }
    else if(protocol == 0x0170){
        CommandHeader cmd;
        packet->PeekHeader(cmd);

        if(cmd.GetDestinationId() == m_nid){
            switch(cmd.GetType()){
                case CommandHeader::SwitchUpdate :
                    Simulator::Schedule(NanoSeconds(30000), &SwitchNode::UpdateMplsRoute, this, cmd);
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
    else if(protocol == 0x0171){
        Ipv4Tag ipv4Tag;
        Ipv6Tag ipv6Tag;
        PortTag portTag;

        if (!packet->PeekPacketTag(portTag)){
            std::cout << "Fail to find port tag" << std::endl;
            return false;
        }

        PortHeader port_header = portTag.GetHeader();

        if (packet->PeekPacketTag(ipv4Tag)){
            Ipv4Header ipv4_header = ipv4Tag.GetHeader();

            FlowV4Id v4Id;
            v4Id.m_srcIP = ipv4_header.GetSource().Get();
            v4Id.m_dstIP = ipv4_header.GetDestination().Get();
            v4Id.m_protocol = ipv4_header.GetProtocol();
            v4Id.m_srcPort = port_header.GetSourcePort();
            v4Id.m_dstPort= port_header.GetDestinationPort();

            devId = GetNextDev(v4Id);  
        }
        else if(packet->PeekPacketTag(ipv6Tag)){
            Ipv6Header ipv6_header = ipv6Tag.GetHeader();

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

        if(m_userSize + packet->GetSize() <= m_userThd){
            m_userSize += packet->GetSize();
            return m_devices[m_mplsroute[label].second]->Send(packet, m_devices[m_mplsroute[label].second]->GetBroadcast(), 0x8847);
        }
        m_drops += 1;
        return false;
    }
    else{
        std::cout << "Unknown Protocol for IngressPipeline" << std::endl;
        return false;
    }


    if(devId == -1){
        std::cout << "Fail to get next dev" << std::endl;
        return false;
    }
    if(ttl == 0){
        std::cout << "TTL = 0 for IP in Switch" << std::endl;
        return false;
    }

    Ptr<NetDevice> device = m_devices[devId];
    
    if(protocol == 0x0170){
        return device->Send(packet, device->GetBroadcast(), protocol);
        std::cout << "Drop of command message" << std::endl;
    }
    else if(m_userSize + packet->GetSize() <= m_userThd){
        m_userSize += packet->GetSize();
        return device->Send(packet, device->GetBroadcast(), protocol);
    }

    m_drops += 1;
    if(m_drops % 100 == 0)
        std::cout << "User packet drop 100 in Switch " << m_nid << std::endl;

    return false;
}

void
SwitchNode::UpdateMplsRoute(CommandHeader cmd)
{
    // std::cout << "Update Label " << cmd.GetLabel() << " with new label " << cmd.GetNewLabel() << " and port " << (uint32_t)cmd.GetPort() 
    //    << " in " << m_nid << std::endl;
    m_mplsroute[cmd.GetLabel()] = std::pair<uint16_t, uint16_t>(cmd.GetNewLabel(), cmd.GetPort());
}

/* Hash function */
uint32_t
SwitchNode::rotateLeft(uint32_t x, unsigned char bits)
{
    return (x << bits) | (x >> (32 - bits));
}

uint32_t 
SwitchNode::hash(FlowV4Id id, uint32_t seed){
    uint32_t result = prime[seed];

    result = rotateLeft(result + id.m_srcPort * Prime[2], 17) * Prime[3];
    result = rotateLeft(result + id.m_dstPort * Prime[4], 11) * Prime[0];
    result = rotateLeft(result + id.m_protocol * Prime[1], 17) * Prime[2];
    result = rotateLeft(result + ((id.m_srcIP >> 8) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((id.m_srcIP >> 16) & 0xff) * Prime[0], 17) * Prime[4];
    result = rotateLeft(result + ((id.m_dstIP >> 8) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((id.m_dstIP >> 16) & 0xff) * Prime[0], 17) * Prime[4];

    // std::cout << seed << " " << result << std::endl;
    return result;
}

uint32_t 
SwitchNode::hash(FlowV6Id id, uint32_t seed){
    uint32_t result = prime[seed];

    result = rotateLeft(result + id.m_srcPort * Prime[2], 17) * Prime[3];
    result = rotateLeft(result + id.m_dstPort * Prime[4], 11) * Prime[0];
    result = rotateLeft(result + id.m_protocol * Prime[1], 17) * Prime[2];
    result = rotateLeft(result + ((id.m_srcIP[0] >> 40) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((id.m_srcIP[0] >> 24) & 0xff) * Prime[0], 17) * Prime[4];
    result = rotateLeft(result + ((id.m_dstIP[0] >> 40) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((id.m_dstIP[0] >> 24) & 0xff) * Prime[0], 17) * Prime[4];

    // std::cout << seed << " " << result << std::endl;
    return result;
}


} // namespace ns3
