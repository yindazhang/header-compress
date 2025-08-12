/*
 * Copyright (c) 2008 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ppp-header.h"
#include "port-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PppHeader");

NS_OBJECT_ENSURE_REGISTERED(PppHeader);

std::pair<uint64_t, uint64_t> Ipv6ToPair(Ipv6Address address)
{
    uint8_t buf[16];
    std::pair<uint64_t, uint64_t> ret;

    address.GetBytes(buf);

    ret.first = *(uint64_t*)(&buf[0]);
    ret.second = *(uint64_t*)(&buf[8]);
    return ret;
}

Ipv6Address PairToIpv6(std::pair<uint64_t, uint64_t> pair)
{
    uint8_t address[16] = {0};
    Ipv6Address ret;

    *(uint64_t*)(&address[0]) = pair.first;
    *(uint64_t*)(&address[8]) = pair.second;

    ret.Set(address);
    return ret;
}

FlowV4Id::FlowV4Id()
{
    m_srcIP = 0;
    m_dstIP = 0;
    m_srcPort = 0;
    m_dstPort = 0;
    m_protocol = 0;
}

FlowV4Id::FlowV4Id(const FlowV4Id& o)
{
    m_srcIP = o.m_srcIP;
    m_dstIP = o.m_dstIP;
    m_srcPort = o.m_srcPort;
    m_dstPort = o.m_dstPort;
    m_protocol = o.m_protocol;
}

bool operator == (const FlowV4Id& a, const FlowV4Id& b){
    return (a.m_srcIP == b.m_srcIP) && (a.m_dstIP == b.m_dstIP) &&
        (a.m_srcPort == b.m_srcPort) && (a.m_dstPort == b.m_dstPort) &&
        (a.m_protocol == b.m_protocol);
}

bool operator < (const FlowV4Id& a, const FlowV4Id& b){
    return std::tie(a.m_srcIP, a.m_dstIP, a.m_srcPort, a.m_dstPort, a.m_protocol) <
          std::tie(b.m_srcIP, b.m_dstIP, b.m_srcPort, b.m_dstPort, b.m_protocol);
}

FlowV4Id 
getFlowV4Id(Ptr<Packet> packet)
{
    Ipv4Header ipv4_header;
    packet->RemoveHeader(ipv4_header);

    FlowV4Id v4Id;
    v4Id.m_srcIP = ipv4_header.GetSource().Get();
    v4Id.m_dstIP = ipv4_header.GetDestination().Get();
    v4Id.m_protocol = ipv4_header.GetProtocol();

    PortHeader port_header;
    packet->PeekHeader(port_header);
    v4Id.m_srcPort = port_header.GetSourcePort();
    v4Id.m_dstPort= port_header.GetDestinationPort();

    packet->AddHeader(ipv4_header);
    return v4Id;
}

FlowV6Id::FlowV6Id()
{
    m_srcIP[0] = 0;
    m_srcIP[1] = 0;
    m_dstIP[0] = 0;
    m_dstIP[1] = 0;
    m_srcPort = 0;
    m_dstPort = 0;
    m_protocol = 0;
}

FlowV6Id::FlowV6Id(const FlowV6Id& o)
{
    m_srcIP[0] = o.m_srcIP[0];
    m_srcIP[1] = o.m_srcIP[1];
    m_dstIP[0] = o.m_dstIP[0];
    m_dstIP[1] = o.m_dstIP[1];
    m_srcPort = o.m_srcPort;
    m_dstPort = o.m_dstPort;
    m_protocol = o.m_protocol;
}

bool operator == (const FlowV6Id& a, const FlowV6Id& b){
    return (a.m_srcIP[0] == b.m_srcIP[0]) && (a.m_srcIP[1] == b.m_srcIP[1]) &&
        (a.m_dstIP[0] == b.m_dstIP[0]) && (a.m_dstIP[1] == b.m_dstIP[1]) &&
        (a.m_srcPort == b.m_srcPort) && (a.m_dstPort == b.m_dstPort) &&
        (a.m_protocol == b.m_protocol);
}

bool operator < (const FlowV6Id& a, const FlowV6Id& b){
    return std::tie(a.m_srcIP[0], a.m_srcIP[1], a.m_dstIP[0], a.m_dstIP[1], 
                        a.m_srcPort, a.m_dstPort, a.m_protocol) <
          std::tie(b.m_srcIP[0], b.m_srcIP[1], b.m_dstIP[0], b.m_dstIP[1],
                        b.m_srcPort, b.m_dstPort, b.m_protocol);
}

FlowV6Id 
getFlowV6Id(Ptr<Packet> packet)
{
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
    packet->PeekHeader(port_header);
    v6Id.m_srcPort = port_header.GetSourcePort();
    v6Id.m_dstPort= port_header.GetDestinationPort();

    packet->AddHeader(ipv6_header);
    return v6Id;
}

const uint32_t Prime[5] = {2654435761U,246822519U,3266489917U,668265263U,374761393U};

const uint32_t prime[16] = {
    181, 5197, 1151, 137, 5569, 7699, 2887, 8753, 
    9323, 8963, 6053, 8893, 9377, 6577, 733, 3527
};

uint32_t
rotateLeft(uint32_t x, unsigned char bits)
{
    return (x << bits) | (x >> (32 - bits));
}

uint32_t 
FlowV4Id::hash(uint32_t seed){
    uint32_t result = prime[seed];

    result = rotateLeft(result + m_srcPort * Prime[2], 17) * Prime[3];
    result = rotateLeft(result + m_dstPort * Prime[4], 11) * Prime[0];
    result = rotateLeft(result + m_protocol * Prime[1], 17) * Prime[2];
    result = rotateLeft(result + ((m_srcIP >> 8) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((m_srcIP >> 16) & 0xff) * Prime[0], 17) * Prime[4];
    result = rotateLeft(result + ((m_dstIP >> 8) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((m_dstIP >> 16) & 0xff) * Prime[0], 17) * Prime[4];

    return result;
}

uint32_t 
FlowV6Id::hash(uint32_t seed){
    uint32_t result = prime[seed];

    result = rotateLeft(result + m_srcPort * Prime[2], 17) * Prime[3];
    result = rotateLeft(result + m_dstPort * Prime[4], 11) * Prime[0];
    result = rotateLeft(result + m_protocol * Prime[1], 17) * Prime[2];
    result = rotateLeft(result + ((m_srcIP[0] >> 40) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((m_srcIP[0] >> 24) & 0xff) * Prime[0], 17) * Prime[4];
    result = rotateLeft(result + ((m_dstIP[0] >> 40) & 0xff) * Prime[3], 11) * Prime[1];
    result = rotateLeft(result + ((m_dstIP[0] >> 24) & 0xff) * Prime[0], 17) * Prime[4];

    return result;
}

PppHeader::PppHeader()
{
    m_padding = 0;
}

PppHeader::~PppHeader()
{
}

TypeId
PppHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PppHeader")
                            .SetParent<Header>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<PppHeader>();
    return tid;
}

TypeId
PppHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
PppHeader::Print(std::ostream& os) const
{
    std::string proto;

    switch (m_protocol)
    {
    case 0x0021: /* IPv4 */
        proto = "IP (0x0021)";
        break;
    case 0x0057: /* IPv6 */
        proto = "IPv6 (0x0057)";
        break;
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    os << "Point-to-Point Protocol: " << proto;
}

uint32_t
PppHeader::GetSerializedSize() const
{
    return 14 + m_padding;
}

void
PppHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU16(m_protocol);
    start.WriteHtonU16(m_padding);
    for(int32_t i = 0;i < m_padding + 10;++i)
        start.WriteU8(0);
}

uint32_t
PppHeader::Deserialize(Buffer::Iterator start)
{
    m_protocol = start.ReadNtohU16();
    m_padding = start.ReadNtohU16();
    for(int32_t i = 0;i < m_padding + 10;++i)
        start.ReadU8();
    return GetSerializedSize();
}

void
PppHeader::SetProtocol(uint16_t protocol)
{
    m_protocol = protocol;
}

uint16_t
PppHeader::GetProtocol()
{
    return m_protocol;
}

void
PppHeader::SetPadding(uint8_t padding)
{
    m_padding = padding;
}

uint8_t
PppHeader::GetPadding()
{
    return m_padding;
}

} // namespace ns3
