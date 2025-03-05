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
#include "mpls-header.h"
#include "port-header.h"
#include "command-header.h"

#include "compress-ipv4-header.h"
#include "compress-ipv6-header.h"

#include <vector>
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
    m_sample.resize(m_sampleSize);
    m_sampleMpls.resize(m_sampleSize);
    Simulator::Schedule(Seconds(4), &NICNode::CheckEcnCount, this);
}

NICNode::~NICNode()
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
NICNode::CheckEcnCount()
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
    return IngressPipeline(packet, protocol, device);
}

void
NICNode::SetECMPHash(uint32_t hashSeed)
{
    m_hashSeed = hashSeed;
}

void
NICNode::SetSetting(uint32_t setting)
{
    m_setting = setting;
}

void 
NICNode::SetThreshold(uint32_t threshold)
{
    m_threshold = threshold;
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
NICNode::SetNextNode(uint16_t devId, uint16_t nodeId)
{
    m_node[devId] = nodeId;
}

void 
NICNode::SetOutput(std::string output)
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
NICNode::AddHostRouteTo(Ipv4Address dest, uint32_t devId)
{
    m_v4route[dest.Get()].push_back(devId);
}

void
NICNode::AddHostRouteTo(Ipv6Address dest, uint32_t devId)
{
    m_v6route[Ipv6ToPair(dest)].push_back(devId);
}

void
NICNode::AddControlRouteTo(uint16_t id, uint32_t devId)
{
    m_idroute[id].push_back(devId);
}

uint16_t 
NICNode::GetNextDev(FlowV4Id id)
{
    const std::vector<uint32_t>& route_vec = m_v4route[id.m_dstIP];
    if(route_vec.size() == 0){
        std::cout << "Cannot find NextDev for Ipv6" << std::endl;
        return -1;
    }

    uint32_t hashValue = hash(id, m_hashSeed);
    return route_vec[hashValue % route_vec.size()];
}

uint16_t 
NICNode::GetNextDev(FlowV6Id id)
{
    const std::vector<uint32_t>& route_vec = m_v6route[
        std::pair<uint64_t, uint64_t>(id.m_dstIP[0], id.m_dstIP[1])];
    if(route_vec.size() == 0){
        std::cout << "Cannot find NextDev for Ipv6" << std::endl;
        return -1;
    }

    uint32_t hashValue = hash(id, m_hashSeed);
    return route_vec[hashValue % route_vec.size()];
}

uint16_t 
NICNode::GetNextNode(uint16_t devId)
{
    if(m_node.find(devId) == m_node.end()){
        std::cout << "Fail to find dev" << std::endl;
        return -1;
    }
    return m_node[devId];
}

uint64_t 
NICNode::GetUserCount()
{
    return m_userCount;
}
    
void 
NICNode::SetUserCount(uint64_t userCount)
{
    m_userCount = userCount;
}

uint64_t 
NICNode::GetMplsCount()
{
    return m_mplsCount;
}

void 
NICNode::SetMplsCount(uint64_t mplsCount)
{
    m_mplsCount = mplsCount;
}

Ptr<Packet>
NICNode::EgressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev){
    PppHeader ppp;
    packet->RemoveHeader(ppp);

    if(protocol == 0x0800 || protocol == 0x86DD || protocol == 0x8847){
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
NICNode::IngressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev){
    uint8_t ttl = 64;

    FlowV4Id v4Id;
    FlowV6Id v6Id;

    uint32_t devId;

    uint64_t t = Simulator::Now().GetNanoSeconds();

    if(protocol == 0x0800){
        Ipv4Header ipv4_header;
        packet->RemoveHeader(ipv4_header);

        ttl = ipv4_header.GetTtl();

        v4Id.m_srcIP = ipv4_header.GetSource().Get();
        v4Id.m_dstIP = ipv4_header.GetDestination().Get();
        v4Id.m_protocol = ipv4_header.GetProtocol();
        
        PortHeader port_header;
        packet->PeekHeader(port_header);
        v4Id.m_srcPort = port_header.GetSourcePort();
        v4Id.m_dstPort= port_header.GetDestinationPort();

        devId = GetNextDev(v4Id);

        if(dev == m_devices[2])
            m_userCount += 1;

        if(m_setting){
            if(m_compress4.find(v4Id) != m_compress4.end()){
                m_mplsCount += 1;

                packet->RemoveHeader(port_header);
                CompressIpv4Header compressIpv4Header;
                compressIpv4Header.SetIpv4Header(ipv4_header);
                packet->AddHeader(compressIpv4Header);

                MplsHeader mpls_header;
                mpls_header.SetLabel(m_compress4[v4Id]);
                mpls_header.SetExp(MplsHeader::ECN_ECT1);
                mpls_header.SetTtl(64);
                packet->AddHeader(mpls_header);

                uint32_t index = m_compress4[v4Id];
                m_sampleMpls[index] += 1;
                if(m_sampleMpls[index] == m_threshold){
                    m_sampleMpls[index] = 0;
                    if(t - m_v4count[v4Id].second > 1000000){
                        m_v4count[v4Id].first = 0;
                        m_v4count[v4Id].second = t;
                    }
                    m_v4count[v4Id].first += 1;

                    if(m_v4count[v4Id].first == 2){
                        Simulator::Schedule(NanoSeconds(1), &NICNode::GenData4, this, v4Id);
                    }
                }

               if(m_userSize + packet->GetSize() <= m_userThd){
                    m_userSize += packet->GetSize();
                    return m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x8847);
                }
                std::cout << "Drop?" << std::endl;
                return false;
            }
            else if(dev == m_devices[2]){
                uint32_t index = hash(v4Id, 10) % m_sampleSize;
                m_sample[index] += 1;
                if(m_sample[index] == m_threshold){
                    m_sample[index] = 0;
                    if(t - m_v4count[v4Id].second > 1000000){
                        m_v4count[v4Id].first = 0;
                        m_v4count[v4Id].second = t;
                    }
                    m_v4count[v4Id].first += 1;

                    if(m_v4count[v4Id].first == 2){
                        Simulator::Schedule(NanoSeconds(1), &NICNode::GenData4, this, v4Id);
                    }
                }
            }
        }

        ipv4_header.SetTtl(ttl - 1);  
        packet->AddHeader(ipv4_header);         
    }
    else if(protocol == 0x86DD){
        Ipv6Header ipv6_header;
        packet->RemoveHeader(ipv6_header);

        ttl = ipv6_header.GetHopLimit();

        auto src_pair = Ipv6ToPair(ipv6_header.GetSource());
        auto dst_pair = Ipv6ToPair(ipv6_header.GetDestination());

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

        if(dev == m_devices[2])
            m_userCount += 1;

        if(m_setting){
            if(m_compress6.find(v6Id) != m_compress6.end()){
                m_mplsCount += 1;
                
                packet->RemoveHeader(port_header);
                CompressIpv6Header compressIpv6Header;
                compressIpv6Header.SetIpv6Header(ipv6_header);
                packet->AddHeader(compressIpv6Header);

                MplsHeader mpls_header;
                mpls_header.SetLabel(m_compress6[v6Id]);
                mpls_header.SetExp(MplsHeader::ECN_ECT1);
                mpls_header.SetTtl(64);
                packet->AddHeader(mpls_header);

                uint32_t index = m_compress6[v6Id];
                m_sampleMpls[index] += 1;
                if(m_sampleMpls[index] == m_threshold){
                    m_sampleMpls[index] = 0;
                    if(t - m_v6count[v6Id].second > 1000000){
                        m_v6count[v6Id].first = 0;
                        m_v6count[v6Id].second = t;
                    }
                    m_v6count[v6Id].first += 1;

                    if(m_v6count[v6Id].first == 2){
                        Simulator::Schedule(NanoSeconds(1), &NICNode::GenData6, this, v6Id);
                    }
                }

               if(m_userSize + packet->GetSize() <= m_userThd){
                    m_userSize += packet->GetSize();
                    return m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x8847);
                }
                std::cout << "Drop?" << std::endl;
                return false;
            }
            else if(dev == m_devices[2]){
                uint32_t index = hash(v6Id, 10) % m_sampleSize;
                m_sample[index] += 1;
                if(m_sample[index] == m_threshold){
                    m_sample[index] = 0;
                    if(t - m_v6count[v6Id].second > 1000000){
                        m_v6count[v6Id].first = 0;
                        m_v6count[v6Id].second = t;
                    }
                    m_v6count[v6Id].first += 1;

                    if(m_v6count[v6Id].first == 2){
                        Simulator::Schedule(NanoSeconds(1), &NICNode::GenData6, this, v6Id);
                    }
                }
            }
        }

        ipv6_header.SetHopLimit(ttl - 1); 
        packet->AddHeader(ipv6_header); 
    }
    else if(protocol == 0x0170){
        CommandHeader cmd;
        packet->PeekHeader(cmd);

        if(cmd.GetDestinationId() == m_nid){
            switch(cmd.GetType()){
                case CommandHeader::NICUpdateCompress4 :
                    Simulator::Schedule(NanoSeconds(30000), &NICNode::UpdateCompress4, this, cmd);
                    return true;
                case CommandHeader::NICUpdateDecompress4 :
                    Simulator::Schedule(NanoSeconds(30000), &NICNode::UpdateDecompress4, this, cmd);
                    return true;
                case CommandHeader::NICUpdateCompress6 :
                    Simulator::Schedule(NanoSeconds(30000), &NICNode::UpdateCompress6, this, cmd);
                    return true;
                case CommandHeader::NICUpdateDecompress6 :
                    Simulator::Schedule(NanoSeconds(30000), &NICNode::UpdateDecompress6, this, cmd);
                    return true;
                case CommandHeader::NICDeleteCompress4 :
                    Simulator::Schedule(NanoSeconds(30000), &NICNode::DeleteCompress4, this, cmd);
                    return true;
                case CommandHeader::NICDeleteCompress6 :
                    Simulator::Schedule(NanoSeconds(1), &NICNode::DeleteCompress6, this, cmd);
                    return true;
                default : std::cout << "Unknown Type" << std::endl; return true;
            }
            return true;
        }
        else{
            if(m_idroute.find(cmd.GetDestinationId()) == m_idroute.end()){
                std::cout << "Fail to find route for command" << std::endl;
                return false;
            }

            const std::vector<uint32_t>& route_vec = m_idroute[cmd.GetDestinationId()];
            devId = route_vec[rand() % route_vec.size()];
        }
    }
    else if(protocol == 0x8847){
        MplsHeader mpls_header;
        packet->RemoveHeader(mpls_header);

        uint16_t label = mpls_header.GetLabel();
        if(m_decompress4.find(label) != m_decompress4.end()){
            CompressIpv4Header compressIpv4Header;
            packet->RemoveHeader(compressIpv4Header);

            Ipv4Header ipv4_header = compressIpv4Header.GetIpv4Header();
            ipv4_header.SetTtl(mpls_header.GetTtl());
            ipv4_header.SetEcn(Ipv4Header::EcnType(mpls_header.GetExp()));

            FlowV4Id v4Id = m_decompress4[label];
            ipv4_header.SetSource(Ipv4Address(v4Id.m_srcIP));
            ipv4_header.SetDestination(Ipv4Address(v4Id.m_dstIP));
            ipv4_header.SetProtocol(v4Id.m_protocol);

            PortHeader port_header;
            port_header.SetSourcePort(v4Id.m_srcPort);
            port_header.SetDestinationPort(v4Id.m_dstPort);

            packet->AddHeader(port_header);
            packet->AddHeader(ipv4_header);

            if(m_userSize + packet->GetSize() <= m_userThd){
                m_userSize += packet->GetSize();
                return m_devices[2]->Send(packet, m_devices[2]->GetBroadcast(), 0x0800);
            }
        }
        else if(m_decompress6.find(label) != m_decompress6.end()){
            CompressIpv6Header compressIpv6Header;
            packet->RemoveHeader(compressIpv6Header);

            Ipv6Header ipv6_header = compressIpv6Header.GetIpv6Header();
            ipv6_header.SetHopLimit(mpls_header.GetTtl());
            ipv6_header.SetEcn(Ipv6Header::EcnType(mpls_header.GetExp()));

            FlowV6Id v6Id = m_decompress6[label];
            ipv6_header.SetSource(PairToIpv6(
                    std::pair<uint64_t, uint64_t>(v6Id.m_srcIP[0], v6Id.m_srcIP[1])));
            ipv6_header.SetDestination(PairToIpv6(
                    std::pair<uint64_t, uint64_t>(v6Id.m_dstIP[0], v6Id.m_dstIP[1])));
            ipv6_header.SetNextHeader(v6Id.m_protocol);

            
            PortHeader port_header;
            port_header.SetSourcePort(v6Id.m_srcPort);
            port_header.SetDestinationPort(v6Id.m_dstPort);

            packet->AddHeader(port_header);
            packet->AddHeader(ipv6_header);

            if(m_userSize + packet->GetSize() <= m_userThd){
                m_userSize += packet->GetSize();
                return m_devices[2]->Send(packet, m_devices[2]->GetBroadcast(), 0x86DD);
            }
        }
        else{
            std::cout << "Unknown Label for IngressPipeline" << std::endl;
        }
        std::cout << "Drop?" << std::endl;
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
        std::cout << "TTL = 0 in NIC" << std::endl;
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
NICNode::GenData4(FlowV4Id id){
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(0xffff);
    cmd.SetType(CommandHeader::NICData4);
    cmd.SetFlow4Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);
}

void 
NICNode::GenData6(FlowV6Id id){
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(0xffff);
    cmd.SetType(CommandHeader::NICData6);
    cmd.SetFlow6Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);
}

void
NICNode::UpdateCompress4(CommandHeader cmd)
{
    // std::cout << "Compress with label " << cmd.GetLabel() << " in " << m_nid << std::endl;
    m_compress4[cmd.GetFlow4Id()] = cmd.GetLabel();
}


void
NICNode::UpdateDecompress4(CommandHeader cmd)
{
    // std::cout << "Decompress with label " << cmd.GetLabel() << " in " << m_nid << std::endl;
    m_decompress4[cmd.GetLabel()] = cmd.GetFlow4Id();
}

void
NICNode::UpdateCompress6(CommandHeader cmd)
{
    // std::cout << "Compress with label " << cmd.GetLabel() << " in " << m_nid << std::endl;
    m_compress6[cmd.GetFlow6Id()] = cmd.GetLabel();
}

void
NICNode::UpdateDecompress6(CommandHeader cmd)
{
    // std::cout << "Decompress with label " << cmd.GetLabel() << " in " << m_nid << std::endl;
    m_decompress6[cmd.GetLabel()] = cmd.GetFlow6Id();
}

void 
NICNode::DeleteCompress4(CommandHeader cmd)
{
    // std::cout << "Delete compress " << m_nid << std::endl;
    m_compress4.erase(cmd.GetFlow4Id());
}

void 
NICNode::DeleteCompress6(CommandHeader cmd)
{
    // std::cout << "Delete compress " << m_nid << std::endl;
    m_compress6.erase(cmd.GetFlow6Id());
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