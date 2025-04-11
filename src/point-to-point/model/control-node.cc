#include "control-node.h"

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

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ControlNode");

NS_OBJECT_ENSURE_REGISTERED(ControlNode);


TypeId
ControlNode::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ControlNode")
            .SetParent<Node>()
            .SetGroupName("PointToPoint")
            .AddConstructor<ControlNode>();
    return tid;
}

ControlNode::ControlNode() : Node() {
}

ControlNode::~ControlNode(){
    fclose(fout);
}

uint32_t
ControlNode::AddDevice(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    uint32_t index = m_devices.size();
    m_devices.push_back(device);
    device->SetNode(this);
    device->SetIfIndex(index);
    device->SetReceiveCallback(MakeCallback(&ControlNode::ReceiveFromDevice, this));
    Simulator::ScheduleWithContext(GetId(), Seconds(0.0), &NetDevice::Initialize, device);
    Simulator::Schedule(NanoSeconds(1000000), &ControlNode::ClearFlow, this);
    NotifyDeviceAdded(device);
    return index;
}

bool
ControlNode::ReceiveFromDevice(Ptr<NetDevice> device,
                                  Ptr<const Packet> p,
                                  uint16_t protocol,
                                  const Address& from)
{
    Ptr<Packet> packet = p->Copy();
    CommandHeader cmd;
    packet->RemoveHeader(cmd);

    switch(cmd.GetType()){
        case CommandHeader::NICData4 :
            return ProcessNICData4(cmd);
        case CommandHeader::NICData6 :
            return ProcessNICData6(cmd);
        default : std::cout << "Unknown Type" << std::endl; return true;
    }
}

void
ControlNode::SetID(uint32_t id)
{
    m_nid = id;
}

uint32_t
ControlNode::GetID()
{
    return m_nid;
}

void 
ControlNode::SetLabelSize(uint32_t labelsize)
{
    m_labelSize = labelsize;
}

void 
ControlNode::SetTopology(uint32_t K, 
    uint32_t NUM_BLOCK,
    uint32_t RATIO,
    std::vector<Ptr<NICNode>> nics,
    std::vector<Ptr<SwitchNode>> edges,
    std::vector<Ptr<SwitchNode>> aggs,
    std::vector<Ptr<SwitchNode>> cores)
{
    m_K = K;
    m_NUM_BLOCK = NUM_BLOCK;
    m_RATIO = RATIO;
    m_nics = nics;
    m_edges = edges;
    m_aggs = aggs;
    m_cores = cores;

    if(m_nics.size() != K * K * NUM_BLOCK * RATIO - 1)
        std::cout << "Number of NICs Error" << std::endl;
    if(m_edges.size() != K * NUM_BLOCK)
        std::cout << "Number of Edges Error" << std::endl;
    if(m_aggs.size() != K * NUM_BLOCK)
        std::cout << "Number of Aggs Error" << std::endl;
    if(m_cores.size() != K * K)
        std::cout << "Number of Cores Error" << std::endl;
}

void 
ControlNode::SetOutput(std::string output)
{
    m_output = output;

    std::string out_file = m_output + ".collector";
    fout = fopen(out_file.c_str(), "w");
}

Ptr<Node> 
ControlNode::GetNode(uint16_t id)
{
    if(id < 2000)
        return m_nics[id - 1000];
    else if(id < 3000)
        return  m_edges[id - 2000];
    else if(id < 4000)
        return m_aggs[id - 3000];
    else
        return m_cores[id - 4000];
}

bool
ControlNode::ProcessNICData4(CommandHeader cmd)
{
    m_data += 1;

    FlowV4Id id = cmd.GetFlow4Id();

    if(m_v4count.find(id) != m_v4count.end()){
        m_v4count[id] = Simulator::Now().GetNanoSeconds();
        return true;
    }

    uint16_t srcId = m_K * m_RATIO * ((id.m_srcIP >> 16) & 0xff) + ((id.m_srcIP >> 8) & 0xff) + 1000;
    uint16_t dstId = m_K * m_RATIO * ((id.m_dstIP >> 16) & 0xff) + ((id.m_dstIP >> 8) & 0xff) + 1000;

    std::vector<std::pair<uint16_t, uint16_t>> vec;
    std::vector<uint32_t> devVec;

    uint16_t tmpId = srcId;
    uint16_t label = 0;
    while(tmpId != dstId){
        uint16_t devId;
        if(tmpId != srcId)
            label = AllocateLabel(GetNode(tmpId));
        vec.emplace_back(tmpId, label);
        if(label == 1)
            return false;

        if(tmpId < 2000){
            devId = m_nics[tmpId - 1000]->GetNextDev(id);
            tmpId = m_nics[tmpId - 1000]->GetNextNode(devId);
        }
        else if(tmpId < 3000){
            devId = m_edges[tmpId - 2000]->GetNextDev(id);
            tmpId = m_edges[tmpId - 2000]->GetNextNode(devId);
        }
        else if(tmpId < 4000){
            devId = m_aggs[tmpId - 3000]->GetNextDev(id);
            tmpId = m_aggs[tmpId - 3000]->GetNextNode(devId);
        }
        else{
            devId = m_cores[tmpId - 4000]->GetNextDev(id);
            tmpId = m_cores[tmpId - 4000]->GetNextNode(devId);
        }

        devVec.push_back(devId);
    }

    label = AllocateLabel(GetNode(dstId));
    vec.emplace_back(dstId, label);
    if(label == 1)
        return false;

    m_v4count[id] = Simulator::Now().GetNanoSeconds();

    Simulator::Schedule(NanoSeconds(1), &ControlNode::GenNICUpdateDecompress4, this, id, vec.back());
    for(int i = vec.size() - 2;i >= 1;--i)
        Simulator::Schedule(NanoSeconds(1), &ControlNode::GenSwitchUpdate, this, vec[i], vec[i + 1].second, devVec[i]);
    Simulator::Schedule(NanoSeconds(10000), &ControlNode::GenNICUpdateCompress4, this, id, vec[0].first, vec[1].second);

    for(uint32_t i = 0;i < vec.size();++i){
        Ptr<Node> ptr = GetNode(vec[i].first);
        m_label4[ptr][vec[i].second] = id;
        m_flow4[ptr][id] = vec[i].second;
    }

    m_update += 1;

    return true;
}

bool
ControlNode::ProcessNICData6(CommandHeader cmd)
{
    m_data += 1;

    FlowV6Id id = cmd.GetFlow6Id();

    if(m_v6count.find(id) != m_v6count.end()){
        m_v6count[id] = Simulator::Now().GetNanoSeconds();
        return true;
    }

    uint16_t srcId = m_K * m_RATIO * ((id.m_srcIP[0] >> 24) & 0xffff) + ((id.m_srcIP[0] >> 40) & 0xffff) + 1000;
    uint16_t dstId = m_K * m_RATIO * ((id.m_dstIP[0] >> 24) & 0xffff) + ((id.m_dstIP[0] >> 40) & 0xffff) + 1000;

    std::vector<std::pair<uint16_t, uint16_t>> vec;
    std::vector<uint32_t> devVec;

    uint16_t tmpId = srcId;
    uint16_t label = 0;
    while(tmpId != dstId){
        uint16_t devId;
        if(tmpId != srcId)
            label = AllocateLabel(GetNode(tmpId));
        vec.emplace_back(tmpId, label);
        if(label == 1){
            return false;
        }

        if(tmpId < 2000){
            devId = m_nics[tmpId - 1000]->GetNextDev(id);
            tmpId = m_nics[tmpId - 1000]->GetNextNode(devId);
        }
        else if(tmpId < 3000){
            devId = m_edges[tmpId - 2000]->GetNextDev(id);
            tmpId = m_edges[tmpId - 2000]->GetNextNode(devId);
        }
        else if(tmpId < 4000){
            devId = m_aggs[tmpId - 3000]->GetNextDev(id);
            tmpId = m_aggs[tmpId - 3000]->GetNextNode(devId);
        }
        else{
            devId = m_cores[tmpId - 4000]->GetNextDev(id);
            tmpId = m_cores[tmpId - 4000]->GetNextNode(devId);
        }

        devVec.push_back(devId);
    }

    label = AllocateLabel(GetNode(dstId));
    vec.emplace_back(dstId, label);
    if(label == 1){
        return false;
    }

    m_v6count[id] = Simulator::Now().GetNanoSeconds();

    Simulator::Schedule(NanoSeconds(1), &ControlNode::GenNICUpdateDecompress6, this, id, vec.back());
    for(int i = vec.size() - 2;i >= 1;--i)
        Simulator::Schedule(NanoSeconds(1), &ControlNode::GenSwitchUpdate, this, vec[i], vec[i + 1].second, devVec[i]);
    Simulator::Schedule(NanoSeconds(10000), &ControlNode::GenNICUpdateCompress6, this, id, vec[0].first, vec[1].second);

    for(uint32_t i = 0;i < vec.size();++i){
        Ptr<Node> ptr = GetNode(vec[i].first);
        m_label6[ptr][vec[i].second] = id;
        m_flow6[ptr][id] = vec[i].second;
    }

    m_update += 1;

    return true;
}

uint16_t 
ControlNode::AllocateLabel(Ptr<Node> node)
{
    std::unordered_map<uint16_t, FlowV4Id>& mp4 = m_label4[node];
    std::unordered_map<uint16_t, FlowV6Id>& mp6 = m_label6[node];

    if(mp4.size() + mp6.size() >= m_labelSize){
        std::cout << "Label full" << std::endl;
        return 1;
    }

    for(uint32_t i = 0;i < 10;++i){
        uint32_t number = rand() % 61703 + 1025;
        if(mp4.find(number) == mp4.end() && mp6.find(number) == mp6.end())
            return number;
    }

    std::cout << "Cannot find label" << std::endl;
    return 1;
}

void 
ControlNode::GenNICUpdateDecompress4(FlowV4Id id, std::pair<uint16_t, uint16_t> mp)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(mp.first);
    cmd.SetType(CommandHeader::NICUpdateDecompress4);
    cmd.SetLabel(mp.second);
    cmd.SetFlow4Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send decompress to " << mp.first << " with label " << mp.second << std::endl;
}

void 
ControlNode::GenNICUpdateDecompress6(FlowV6Id id, std::pair<uint16_t, uint16_t> mp)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(mp.first);
    cmd.SetType(CommandHeader::NICUpdateDecompress6);
    cmd.SetLabel(mp.second);
    cmd.SetFlow6Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send decompress to " << mp.first << " with label " << mp.second << std::endl;
}

void 
ControlNode::GenNICUpdateCompress4(FlowV4Id id, uint16_t nodeId, uint16_t label)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(nodeId);
    cmd.SetType(CommandHeader::NICUpdateCompress4);
    cmd.SetLabel(label);
    cmd.SetFlow4Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send compress to " << nodeId << " with label " << label << std::endl;
}

void 
ControlNode::GenNICUpdateCompress6(FlowV6Id id, uint16_t nodeId, uint16_t label)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(nodeId);
    cmd.SetType(CommandHeader::NICUpdateCompress6);
    cmd.SetLabel(label);
    cmd.SetFlow6Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send compress to " << nodeId << " with label " << label << std::endl;
}

void 
ControlNode::GenNICDeleteCompress4(uint16_t nodeId, FlowV4Id id)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(nodeId);
    cmd.SetType(CommandHeader::NICDeleteCompress4);
    cmd.SetFlow4Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send delete to " << nodeId << std::endl;
}

void 
ControlNode::GenNICDeleteCompress6(uint16_t nodeId, FlowV6Id id)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(nodeId);
    cmd.SetType(CommandHeader::NICDeleteCompress6);
    cmd.SetFlow6Id(id);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send delete to " << nodeId << std::endl;
}

void 
ControlNode::GenSwitchUpdate(std::pair<uint16_t, uint16_t> mp, uint16_t newLabel, uint32_t devId)
{
    Ptr<Packet> packet = Create<Packet>();
    CommandHeader cmd;

    cmd.SetSourceId(m_nid);
    cmd.SetDestinationId(mp.first);
    cmd.SetType(CommandHeader::SwitchUpdate);
    cmd.SetLabel(mp.second);
    cmd.SetNewLabel(newLabel);
    cmd.SetPort(devId);

    packet->AddHeader(cmd);
    m_devices[1]->Send(packet, m_devices[1]->GetBroadcast(), 0x0170);

    // std::cout << "Send switchupdate to " << mp.first << " with label " << mp.second <<  " new label " << newLabel << " dev " << devId << std::endl;
}

void 
ControlNode::ClearNode(Ptr<Node> node)
{
    std::map<FlowV4Id, uint16_t>& mp4 = m_flow4[node];
    std::map<FlowV6Id, uint16_t>& mp6 = m_flow6[node];

    uint64_t minimum = UINT64_MAX;
    for(auto it = mp4.begin();it != mp4.end();++it){
        if(m_delete4.find(it->first) != m_delete4.end())
            continue;
        if(m_v4count[it->first] < minimum)
            minimum = m_v4count[it->first];
    }
    for(auto it = mp6.begin();it != mp6.end();++it){
        if(m_delete6.find(it->first) != m_delete6.end())
            continue;
        if(m_v6count[it->first] < minimum)
            minimum = m_v6count[it->first];
    }

    if(Simulator::Now().GetNanoSeconds() - minimum < 2000000)
        return;

    std::cout << "Delete flow " << (Simulator::Now().GetNanoSeconds() - minimum) / 1000000 << " ms ago" << std::endl; 

    for(auto it = mp4.begin();it != mp4.end();++it){
        if(m_delete4.find(it->first) != m_delete4.end())
            continue;
        if(m_v4count[it->first] - minimum < 1000000){
            minimum = m_v4count[it->first];
            FlowV4Id id = it->first;

            uint16_t srcId = m_K * m_RATIO * ((id.m_srcIP >> 16) & 0xff) + ((id.m_srcIP >> 8) & 0xff) + 1000;
            uint16_t dstId = m_K * m_RATIO * ((id.m_dstIP >> 16) & 0xff) + ((id.m_dstIP >> 8) & 0xff) + 1000;

            uint16_t tmpId = srcId;
            std::map<FlowV4Id, std::vector<Ptr<Node>>> mp;

            while(tmpId != dstId){
                uint16_t devId;
                Ptr<Node> tmpNode;
                if(tmpId < 2000){
                    tmpNode = m_nics[tmpId - 1000];
                    devId = m_nics[tmpId - 1000]->GetNextDev(id);
                    tmpId = m_nics[tmpId - 1000]->GetNextNode(devId);
                }
                else if(tmpId < 3000){
                    tmpNode = m_edges[tmpId - 2000];
                    devId = m_edges[tmpId - 2000]->GetNextDev(id);
                    tmpId = m_edges[tmpId - 2000]->GetNextNode(devId);
                }
                else if(tmpId < 4000){
                    tmpNode =  m_aggs[tmpId - 3000];
                    devId = m_aggs[tmpId - 3000]->GetNextDev(id);
                    tmpId = m_aggs[tmpId - 3000]->GetNextNode(devId);
                }
                else{
                    tmpNode = m_cores[tmpId - 4000];
                    devId = m_cores[tmpId - 4000]->GetNextDev(id);
                    tmpId = m_cores[tmpId - 4000]->GetNextNode(devId);
                }

                mp[id].push_back(tmpNode);
            }

            mp[id].push_back(GetNode(dstId));
            m_delete4.insert(id);
            m_delete += 1;

            Simulator::Schedule(NanoSeconds(1), &ControlNode::GenNICDeleteCompress4, this, srcId, id);
            Simulator::Schedule(NanoSeconds(50000), &ControlNode::EraseFlow4, this, mp);
        }
    }
    for(auto it = mp6.begin();it != mp6.end();++it){
        if(m_delete6.find(it->first) != m_delete6.end())
            continue;
        if(m_v6count[it->first] - minimum < 1000000){
            minimum = m_v6count[it->first];
            FlowV6Id id = it->first;

            uint16_t srcId = m_K * m_RATIO * ((id.m_srcIP[0] >> 24) & 0xffff) + ((id.m_srcIP[0] >> 40) & 0xffff) + 1000;
            uint16_t dstId = m_K * m_RATIO * ((id.m_dstIP[0] >> 24) & 0xffff) + ((id.m_dstIP[0] >> 40) & 0xffff) + 1000;

            uint16_t tmpId = srcId;
            std::map<FlowV6Id, std::vector<Ptr<Node>>> mp;

            while(tmpId != dstId){
                uint16_t devId;
                Ptr<Node> tmpNode;
                if(tmpId < 2000){
                    tmpNode = m_nics[tmpId - 1000];
                    devId = m_nics[tmpId - 1000]->GetNextDev(id);
                    tmpId = m_nics[tmpId - 1000]->GetNextNode(devId);
                }
                else if(tmpId < 3000){
                    tmpNode = m_edges[tmpId - 2000];
                    devId = m_edges[tmpId - 2000]->GetNextDev(id);
                    tmpId = m_edges[tmpId - 2000]->GetNextNode(devId);
                }
                else if(tmpId < 4000){
                    tmpNode =  m_aggs[tmpId - 3000];
                    devId = m_aggs[tmpId - 3000]->GetNextDev(id);
                    tmpId = m_aggs[tmpId - 3000]->GetNextNode(devId);
                }
                else{
                    tmpNode = m_cores[tmpId - 4000];
                    devId = m_cores[tmpId - 4000]->GetNextDev(id);
                    tmpId = m_cores[tmpId - 4000]->GetNextNode(devId);
                }

                mp[id].push_back(tmpNode);
            }

            mp[id].push_back(GetNode(dstId));
            m_delete6.insert(id);
            m_delete += 1;
            
            Simulator::Schedule(NanoSeconds(1), &ControlNode::GenNICDeleteCompress6, this, srcId, id);
            Simulator::Schedule(NanoSeconds(50000), &ControlNode::EraseFlow6, this, mp);
        }
    }
}

void 
ControlNode::EraseFlow4(const std::map<FlowV4Id, std::vector<Ptr<Node>>>&  mp)
{
    for(auto it = mp.begin();it != mp.end();++it){
        for(auto ptr : it->second){
            m_label4[ptr].erase(m_flow4[ptr][it->first]);
            m_flow4[ptr].erase(it->first);
        }
        m_v4count.erase(it->first);
        m_delete4.erase(it->first);
    }
}
    
void 
ControlNode::EraseFlow6(const std::map<FlowV6Id, std::vector<Ptr<Node>>>&  mp)
{
    for(auto it = mp.begin();it != mp.end();++it){
        for(auto ptr : it->second){
            m_label6[ptr].erase(m_flow6[ptr][it->first]);
            m_flow6[ptr].erase(it->first);
        }
        m_v6count.erase(it->first);
        m_delete6.erase(it->first);
    }
}

void 
ControlNode::ClearFlow()
{
    for(auto node : m_nics)
        if(m_flow4[node].size() + m_flow6[node].size() > 0.8 * m_labelSize)
            ClearNode(node);

    for(auto node : m_edges)
        if(m_flow4[node].size() + m_flow6[node].size() > 0.8 * m_labelSize)
            ClearNode(node);
    
    for(auto node : m_aggs)
        if(m_flow4[node].size() + m_flow6[node].size() > 0.8 * m_labelSize)
            ClearNode(node); 
    
    for(auto node : m_cores)
        if(m_flow4[node].size() + m_flow6[node].size() > 0.8 * m_labelSize)
            ClearNode(node);  
    
    if(m_data > 0 || m_delete > 0){
        fprintf(fout, "%lld,%lld,%lld,%lld\n", Simulator::Now().GetMilliSeconds(), m_data, m_update, m_delete);
        fflush(fout);
    }
    m_data = m_update = m_delete = 0;
    
    Simulator::Schedule(NanoSeconds(1000000), &ControlNode::ClearFlow, this);
}

} // namespace ns3