#include "rohc-decompressor.h"

#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "port-header.h"
#include "rohc-header.h"
#include "rohc-ip-header.h"
#include "rohc-hctcp-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RohcDecompressor");

NS_OBJECT_ENSURE_REGISTERED(RohcDecompressor);

TypeId
RohcDecompressor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RohcDecompressor")
                            .SetParent<Object>()
                            .SetGroupName("PointToPoint");
    return tid;
}

RohcDecompressor::RohcDecompressor()
{
    m_contentList.resize(m_maxContent);
}

RohcDecompressor::~RohcDecompressor()
{
}

uint16_t 
RohcDecompressor::Process(Ptr<Packet> packet)
{
    RohcHeader rohc_header;
    packet->RemoveHeader(rohc_header);

    uint16_t protocol = 0x0800;
    uint16_t index = rohc_header.GetCid();
    RohcContent& content = m_contentList[index];

    if(rohc_header.GetType() == 1){
        content.profile = rohc_header.GetProfile();
        if(content.profile == 4){
            packet->RemoveHeader(content.ipv4Header);
            packet->RemoveHeader(content.portHeader);
            if(content.ipv4Header.GetProtocol() == 6){
                packet->PeekHeader(content.hcTcpHeader);
            }
            packet->AddHeader(content.portHeader);
            packet->AddHeader(content.ipv4Header);
        }
        else if(content.profile == 6){
            packet->RemoveHeader(content.ipv6Header);
            packet->RemoveHeader(content.portHeader);
            if(content.ipv6Header.GetNextHeader() == Ipv6Header::IPV6_TCP){
                packet->PeekHeader(content.hcTcpHeader);
            }
            packet->AddHeader(content.portHeader);
            packet->AddHeader(content.ipv6Header);
            protocol = 0x86DD;
        }
    }
    else{
        RohcIpHeader rohc_ip_header;
        packet->RemoveHeader(rohc_ip_header);
        if(content.profile == 4){
            rohc_ip_header.GetIpv4Header(content.ipv4Header);
            
            if(content.ipv4Header.GetProtocol() == 6){
                RohcHcTcpHeader rohc_hctcp_header;
                packet->RemoveHeader(rohc_hctcp_header);
                content.hcTcpHeader = rohc_hctcp_header.GetHeader(content.hcTcpHeader);
                packet->AddHeader(content.hcTcpHeader);
            }

            packet->AddHeader(content.portHeader);
            packet->AddHeader(content.ipv4Header);
        }
        else if(content.profile == 6){
            rohc_ip_header.GetIpv6Header(content.ipv6Header);
            
            if(content.ipv6Header.GetNextHeader() == Ipv6Header::IPV6_TCP){
                RohcHcTcpHeader rohc_hctcp_header;
                packet->RemoveHeader(rohc_hctcp_header);
                content.hcTcpHeader = rohc_hctcp_header.GetHeader(content.hcTcpHeader);
                packet->AddHeader(content.hcTcpHeader);
            }

            packet->AddHeader(content.portHeader);
            packet->AddHeader(content.ipv6Header);
            protocol = 0x86DD;
        }
    }

    return protocol;
}

} // namespace ns3