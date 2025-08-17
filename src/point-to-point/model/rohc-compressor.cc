#include "rohc-compressor.h"

#include "ns3/simulator.h"

#include "port-header.h"
#include "rohc-header.h"
#include "rohc-ip-header.h"
#include "rohc-hctcp-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RohcCompressor");

NS_OBJECT_ENSURE_REGISTERED(RohcCompressor);

TypeId
RohcCompressor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RohcCompressor")
                            .SetParent<Object>()
                            .SetGroupName("PointToPoint");
    return tid;
}

RohcCompressor::RohcCompressor()
{
    m_contextList.resize(m_maxContext);
}

RohcCompressor::~RohcCompressor()
{
}

uint16_t 
RohcCompressor::Process(Ptr<Packet> packet, uint16_t protocol)
{
    if(protocol == 0x0800){
        Ipv4Header ipv4_header;
        packet->RemoveHeader(ipv4_header);

        FlowV4Id v4Id;
        v4Id.m_srcIP = ipv4_header.GetSource().Get();
        v4Id.m_dstIP = ipv4_header.GetDestination().Get();
        v4Id.m_protocol = ipv4_header.GetProtocol();
        
        PortHeader port_header;
        packet->RemoveHeader(port_header);
        v4Id.m_srcPort = port_header.GetSourcePort();
        v4Id.m_dstPort= port_header.GetDestinationPort();

        uint16_t index = v4Id.hash(6) % m_maxContext;
        if(m_contextList[index].flowV4Id == v4Id && Simulator::Now().GetNanoSeconds() - m_contextList[index].updateTimeNs <= 100000){
            if(v4Id.m_protocol == 6){
                HcTcpHeader hctcp_header;
                packet->RemoveHeader(hctcp_header);
                RohcHcTcpHeader rohc_hctcp_header;
                rohc_hctcp_header.SetHeader(m_contextList[index].hcTcpHeader, hctcp_header);
                packet->AddHeader(rohc_hctcp_header);
                m_contextList[index].hcTcpHeader = hctcp_header;
            }

            RohcIpHeader rohc_ip_header;
            rohc_ip_header.SetIpv4Header(ipv4_header);
            packet->AddHeader(rohc_ip_header);
            
            RohcHeader rohc_header;
            rohc_header.SetType(0);
            rohc_header.SetCid(index);

            packet->AddHeader(rohc_header);
        }
        else{
            if(v4Id.m_protocol == 6){
                packet->PeekHeader(m_contextList[index].hcTcpHeader);
            }

            m_contextList[index].updateTimeNs = Simulator::Now().GetNanoSeconds();
            m_contextList[index].flowV4Id = v4Id;

            packet->AddHeader(port_header);
            packet->AddHeader(ipv4_header);

            RohcHeader rohc_header;
            rohc_header.SetType(1);
            rohc_header.SetProfile(4);
            rohc_header.SetCid(index);

            packet->AddHeader(rohc_header);
        }
        return 0x0172;
    }
    else if(protocol == 0x86DD){
        Ipv6Header ipv6_header;
        packet->RemoveHeader(ipv6_header);

        auto src_pair = Ipv6ToPair(ipv6_header.GetSource());
        auto dst_pair = Ipv6ToPair(ipv6_header.GetDestination());

        FlowV6Id v6Id;
        v6Id.m_srcIP[0] = src_pair.first;
        v6Id.m_srcIP[1] = src_pair.second;
        v6Id.m_dstIP[0] = dst_pair.first;
        v6Id.m_dstIP[1] = dst_pair.second;
        v6Id.m_protocol = ipv6_header.GetNextHeader();

        PortHeader port_header;
        packet->RemoveHeader(port_header);
        v6Id.m_srcPort = port_header.GetSourcePort();
        v6Id.m_dstPort= port_header.GetDestinationPort();

        uint16_t index = v6Id.hash(6) % m_maxContext;
        if(m_contextList[index].flowV6Id == v6Id && Simulator::Now().GetNanoSeconds() - m_contextList[index].updateTimeNs <= 100000){
            if(v6Id.m_protocol == 6){
                HcTcpHeader hctcp_header;
                packet->RemoveHeader(hctcp_header);
                RohcHcTcpHeader rohc_hctcp_header;
                rohc_hctcp_header.SetHeader(m_contextList[index].hcTcpHeader, hctcp_header);
                packet->AddHeader(rohc_hctcp_header);
                m_contextList[index].hcTcpHeader = hctcp_header;
            }

            RohcIpHeader rohc_ip_header;
            rohc_ip_header.SetIpv6Header(ipv6_header);
            packet->AddHeader(rohc_ip_header);
            
            RohcHeader rohc_header;
            rohc_header.SetType(0);
            rohc_header.SetCid(index);

            packet->AddHeader(rohc_header);
        }
        else{
            HcTcpHeader hctcp_header;
            if(v6Id.m_protocol == 6){
                packet->PeekHeader(hctcp_header);
            }

            m_contextList[index].updateTimeNs = Simulator::Now().GetNanoSeconds();
            m_contextList[index].flowV6Id = v6Id;
            m_contextList[index].hcTcpHeader = hctcp_header;

            packet->AddHeader(port_header);
            packet->AddHeader(ipv6_header);

            RohcHeader rohc_header;
            rohc_header.SetType(1);
            rohc_header.SetProfile(6);
            rohc_header.SetCid(index);

            packet->AddHeader(rohc_header);
        }
        return 0x0172;
    }
    return protocol;
}

} // namespace ns3