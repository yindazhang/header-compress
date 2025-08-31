/*
 * Copyright (c) 2007, 2008 University of Washington
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

#include "point-to-point-net-device.h"

#include "point-to-point-channel.h"
#include "pfc-header.h"
#include "ppp-header.h"
#include "mpls-header.h"
#include "vxlan-header.h"
#include "compress-ip-header.h"
#include "switch-node.h"

#include "ns3/error-model.h"
#include "ns3/llc-snap-header.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/socket.h"
#include "ns3/udp-header.h"

#include "bth-header.h"

#include "point-to-point-queue.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PointToPointNetDevice");

NS_OBJECT_ENSURE_REGISTERED(PointToPointNetDevice);

TypeId
PointToPointNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PointToPointNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("PointToPoint")
            .AddConstructor<PointToPointNetDevice>()
            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(DEFAULT_MTU),
                          MakeUintegerAccessor(&PointToPointNetDevice::SetMtu,
                                               &PointToPointNetDevice::GetMtu),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Address",
                          "The MAC address of this device.",
                          Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")),
                          MakeMac48AddressAccessor(&PointToPointNetDevice::m_address),
                          MakeMac48AddressChecker())
            .AddAttribute("DataRate",
                          "The default data rate for point to point links",
                          DataRateValue(DataRate("32768b/s")),
                          MakeDataRateAccessor(&PointToPointNetDevice::m_bps),
                          MakeDataRateChecker())
            .AddAttribute("ReceiveErrorModel",
                          "The receiver error model used to simulate packet loss",
                          PointerValue(),
                          MakePointerAccessor(&PointToPointNetDevice::m_receiveErrorModel),
                          MakePointerChecker<ErrorModel>())
            .AddAttribute("InterframeGap",
                          "The time to wait between packet (frame) transmissions",
                          TimeValue(Seconds(0.0)),
                          MakeTimeAccessor(&PointToPointNetDevice::m_tInterframeGap),
                          MakeTimeChecker())

            //
            // Transmit queueing discipline for the device which includes its own set
            // of trace hooks.
            //
            .AddAttribute("TxQueue",
                          "A queue to use as the transmit queue in the device.",
                          PointerValue(),
                          MakePointerAccessor(&PointToPointNetDevice::m_queue),
                          MakePointerChecker<Queue<Packet>>())

            //
            // Trace sources at the "top" of the net device, where packets transition
            // to/from higher layers.
            //
            .AddTraceSource("MacTx",
                            "Trace source indicating a packet has arrived "
                            "for transmission by this device",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_macTxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTxDrop",
                            "Trace source indicating a packet has been dropped "
                            "by the device before transmission",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_macTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacPromiscRx",
                            "A packet has been received by this device, "
                            "has been passed up from the physical layer "
                            "and is being forwarded up the local protocol stack.  "
                            "This is a promiscuous trace,",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_macPromiscRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacRx",
                            "A packet has been received by this device, "
                            "has been passed up from the physical layer "
                            "and is being forwarded up the local protocol stack.  "
                            "This is a non-promiscuous trace,",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_macRxTrace),
                            "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop",
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
            //
            // Trace sources at the "bottom" of the net device, where packets transition
            // to/from the channel.
            //
            .AddTraceSource("PhyTxBegin",
                            "Trace source indicating a packet has begun "
                            "transmitting over the channel",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_phyTxBeginTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxEnd",
                            "Trace source indicating a packet has been "
                            "completely transmitted over the channel",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxDrop",
                            "Trace source indicating a packet has been "
                            "dropped by the device during transmission",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_phyTxDropTrace),
                            "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin",
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
            .AddTraceSource("PhyRxEnd",
                            "Trace source indicating a packet has been "
                            "completely received by the device",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_phyRxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxDrop",
                            "Trace source indicating a packet has been "
                            "dropped by the device during reception",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_phyRxDropTrace),
                            "ns3::Packet::TracedCallback")

            //
            // Trace sources designed to simulate a packet sniffer facility (tcpdump).
            // Note that there is really no difference between promiscuous and
            // non-promiscuous traces in a point-to-point link.
            //
            .AddTraceSource("Sniffer",
                            "Trace source simulating a non-promiscuous packet sniffer "
                            "attached to the device",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_snifferTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PromiscSniffer",
                            "Trace source simulating a promiscuous packet sniffer "
                            "attached to the device",
                            MakeTraceSourceAccessor(&PointToPointNetDevice::m_promiscSnifferTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

PointToPointNetDevice::PointToPointNetDevice()
    : m_txMachineState(READY),
      m_channel(nullptr),
      m_linkUp(false),
      m_currentPkt(nullptr)
{
    NS_LOG_FUNCTION(this);
}

PointToPointNetDevice::~PointToPointNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
PointToPointNetDevice::SetID(uint32_t id)
{
    m_id = id;
}

bool
PointToPointNetDevice::Available()
{
    return m_queue->GetNBytes() < 20000;
}

void
PointToPointNetDevice::AddQP(Ptr<RdmaQueuePair> qp)
{
    m_rdmaQp[qp->GetQP()] = qp;
}

void
PointToPointNetDevice::AddHeader(Ptr<Packet> p, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << p << protocolNumber);
    PppHeader ppp;
    ppp.SetProtocol(EtherToPpp(protocolNumber));
    p->AddHeader(ppp);
}

bool
PointToPointNetDevice::ProcessHeader(Ptr<Packet> p, uint16_t& param)
{
    NS_LOG_FUNCTION(this << p << param);
    PppHeader ppp;
    p->RemoveHeader(ppp);
    param = PppToEther(ppp.GetProtocol());
    return true;
}

void
PointToPointNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_node = nullptr;
    m_channel = nullptr;
    m_receiveErrorModel = nullptr;
    m_currentPkt = nullptr;
    m_queue = nullptr;
    NetDevice::DoDispose();
}

void
PointToPointNetDevice::SetDataRate(DataRate bps)
{
    NS_LOG_FUNCTION(this);
    m_bps = bps;
}

void
PointToPointNetDevice::SetInterframeGap(Time t)
{
    NS_LOG_FUNCTION(this << t.As(Time::S));
    m_tInterframeGap = t;
}

bool
PointToPointNetDevice::TransmitStart(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

    // Add
    Ptr<SwitchNode> switch_node = DynamicCast<SwitchNode>(GetNode());

    PppHeader ppp;
    if(switch_node){
        p->PeekHeader(ppp);
        uint16_t protocol = ppp.GetProtocol();
        p = switch_node->EgressPipeline(p, PppToEther(protocol), this);
    }
    else if(m_id != 0){
        p->RemoveHeader(ppp);
        if(m_setting == CompressType::COMPRESS_ROHC && 
            (ppp.GetProtocol() == 0x0800 || ppp.GetProtocol() == 0x86DD)){
            uint16_t protocol = m_rohcCom.Process(p, ppp.GetProtocol());
            ppp.SetProtocol(PointToPointNetDevice::EtherToPpp(protocol));
        }
        p->AddHeader(ppp);
    }

    //
    // This function is called to start the process of transmitting a packet.
    // We need to tell the channel that we've started wiggling the wire and
    // schedule an event that will be executed when the transmission is complete.
    //
    NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
    m_txMachineState = BUSY;
    m_currentPkt = p;
    //m_phyTxBeginTrace(m_currentPkt);

    //NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.As(Time::S));
    if(p == nullptr){
        Simulator::Schedule(NanoSeconds(1), &PointToPointNetDevice::TransmitComplete, this);
        return true;
    }

    Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
    Time txCompleteTime = txTime + m_tInterframeGap;

    Simulator::Schedule(txCompleteTime, &PointToPointNetDevice::TransmitComplete, this);

    bool result = m_channel->TransmitStart(p, this, txTime);
    if (result == false)
    {
        m_phyTxDropTrace(p);
    }
    return result;
}

void
PointToPointNetDevice::TransmitComplete()
{
    NS_LOG_FUNCTION(this);

    //
    // This function is called to when we're all done transmitting a packet.
    // We try and pull another packet off of the transmit queue.  If the queue
    // is empty, we are done, otherwise we need to start transmitting the
    // next packet.
    //
    NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
    m_txMachineState = READY;

    //NS_ASSERT_MSG(m_currentPkt, "PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

    //m_phyTxEndTrace(m_currentPkt);
    m_currentPkt = nullptr;

    Ptr<Packet> p = m_queue->Dequeue(m_pause);
    if (!p)
    {
        NS_LOG_LOGIC("No pending packets in device queue after tx complete");
        return;
    }

    //
    // Got another packet off of the queue, so start the transmit process again.
    //
    m_snifferTrace(p);
    m_promiscSnifferTrace(p);
    TransmitStart(p);
}

bool
PointToPointNetDevice::Attach(Ptr<PointToPointChannel> ch)
{
    NS_LOG_FUNCTION(this << &ch);

    m_channel = ch;

    m_channel->Attach(this);

    //
    // This device is up whenever it is attached to a channel.  A better plan
    // would be to have the link come up when both devices are attached, but this
    // is not done for now.
    //
    NotifyLinkUp();
    return true;
}

void
PointToPointNetDevice::SetQueue(Ptr<Queue<Packet>> q)
{
    NS_LOG_FUNCTION(this << q);
    m_queue = DynamicCast<PointToPointQueue>(q);
    if(m_queue == nullptr)
        std::cerr << "Error: PointToPointNetDevice::SetQueue: Queue is not a PointToPointQueue." << std::endl;
}

void
PointToPointNetDevice::SetReceiveErrorModel(Ptr<ErrorModel> em)
{
    NS_LOG_FUNCTION(this << em);
    m_receiveErrorModel = em;
}

void
PointToPointNetDevice::Receive(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    uint16_t protocol = 0;

    if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
    {
        //
        // If we have an error model and it indicates that it is time to lose a
        // corrupted packet, don't forward this packet up, let it go.
        //
        m_phyRxDropTrace(packet);
    }
    else
    {
        //
        // Hit the trace hooks.  All of these hooks are in the same place in this
        // device because it is so simple, but this is not usually the case in
        // more complicated devices.
        //
        m_snifferTrace(packet);
        m_promiscSnifferTrace(packet);
        m_phyRxEndTrace(packet);

        //
        // Trace sinks will expect complete packets, not packets without some of the
        // headers.
        //
        Ptr<Packet> originalPacket = packet->Copy();

        //
        // Strip off the point-to-point protocol header and forward this packet
        // up the protocol stack.  Since this is a simple point-to-point link,
        // there is no difference in what the promisc callback sees and what the
        // normal receive callback sees.
        //
        ProcessHeader(packet, protocol);

        if(protocol == 0x8808){
            // std::cout << "Find PFC" << std::endl;
            PfcHeader pfc;
            packet->RemoveHeader(pfc);
            m_pause = (pfc.GetPause(2) != 0);

            if(!m_pause && m_txMachineState == READY){
                // std::cout << "Resume queue" << std::endl;
                packet = m_queue->Dequeue(m_pause);
                if(packet != nullptr) 
                    TransmitStart(packet);
            }
            return;
        }

        if(m_id != 0){
            if(protocol == 0x0172)
                protocol = m_rohcDecom.Process(packet);

            bool decap = true;
            if(protocol == 0x0170){
                CommandHeader cmd;
                packet->PeekHeader(cmd);

                if(cmd.GetDestinationId() != m_id)
                    std::cout << "Fail to find command destination in " << m_id << std::endl;

                switch(cmd.GetType()){
                    case CommandHeader::NICUpdateCompress4 :
                        Simulator::Schedule(NanoSeconds(COMMAND_DELAY), &PointToPointNetDevice::UpdateCompress4, this, cmd);
                        return;
                    case CommandHeader::NICUpdateDecompress4 :
                        Simulator::Schedule(NanoSeconds(COMMAND_DELAY), &PointToPointNetDevice::UpdateDecompress4, this, cmd);
                        return;
                    case CommandHeader::NICUpdateCompress6 :
                        Simulator::Schedule(NanoSeconds(COMMAND_DELAY), &PointToPointNetDevice::UpdateCompress6, this, cmd);
                        return;
                    case CommandHeader::NICUpdateDecompress6 :
                        Simulator::Schedule(NanoSeconds(COMMAND_DELAY), &PointToPointNetDevice::UpdateDecompress6, this, cmd);
                        return;
                    case CommandHeader::NICDeleteCompress4 :
                        Simulator::Schedule(NanoSeconds(1000), &PointToPointNetDevice::DeleteCompress4, this, cmd);
                        return;
                    case CommandHeader::NICDeleteCompress6 :
                        Simulator::Schedule(NanoSeconds(1000), &PointToPointNetDevice::DeleteCompress6, this, cmd);
                        return;
                    default : std::cout << "Unknown Type" << std::endl; return;
                }
                return;
            }
            else if(protocol == 0x8847){
                decap = false;
                MplsHeader mpls_header;
                packet->RemoveHeader(mpls_header);

                uint16_t label = mpls_header.GetLabel();
                if(m_decompress4.find(label) != m_decompress4.end()){
                    CompressIpHeader compressIpHeader;
                    packet->RemoveHeader(compressIpHeader);

                    Ipv4Header ipv4_header = compressIpHeader.GetIpv4Header();
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

                    protocol = 0x0800;
                }
                else if(m_decompress6.find(label) != m_decompress6.end()){
                    CompressIpHeader compressIpHeader;
                    packet->RemoveHeader(compressIpHeader);

                    Ipv6Header ipv6_header = compressIpHeader.GetIpv6Header();
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

                    protocol = 0x86DD;
                }
                else{
                    std::cout << "Unknown Label for IngressPipeline" << std::endl;
                    return;
                }
            }
            else if(protocol == 0x0171){
                protocol = m_idealDecom.Process(packet);
            }

            if(m_vxlan && decap) DecapVxLAN(packet);

            if(m_rdma){
                RdmaReceive(packet, protocol);
                return;
            }
        }

        if (!m_promiscCallback.IsNull())
        {
            m_macPromiscRxTrace(originalPacket);
            m_promiscCallback(this,
                              packet,
                              protocol,
                              GetRemote(),
                              GetAddress(),
                              NetDevice::PACKET_HOST);
        }

        m_macRxTrace(originalPacket);
        m_rxCallback(this, packet, protocol, GetRemote());
    }
}

void 
PointToPointNetDevice::RdmaReceive(Ptr<Packet> packet, uint16_t protocol)
{
    Ipv4Header ipv4_header;
    Ipv6Header ipv6_header;
    UdpHeader udp_header;
    BthHeader bth_header;

    Address srcAddr;

    if(protocol == 0x0800) {
        packet->RemoveHeader(ipv4_header);
        srcAddr = ipv4_header.GetSource();
    } else if(protocol == 0x86DD) {
        packet->RemoveHeader(ipv6_header);
        srcAddr = ipv6_header.GetSource();
    } else{
        std::cerr << "Unknown protocol for RDMA" << std::endl;
        return;
    }

    packet->RemoveHeader(udp_header);
    packet->RemoveHeader(bth_header);
    
    uint32_t id = bth_header.GetId();
    if(bth_header.GetACK() || bth_header.GetNACK()) {
        if(m_rdmaQp.find(id) == m_rdmaQp.end()) {
            std::cerr << "Unknown RDMA QP ID: " << id << " in NIC " << m_id << std::endl;
            return;
        }
        m_rdmaQp[id]->ProcessACK(bth_header);
    } else {
        auto key = std::pair<Address, uint32_t>(srcAddr, id);
        uint64_t preSeq = m_rdmaReceiver[key].first;
        uint64_t seq = bth_header.GetSequence(preSeq);
        // std::cout << "Receive: " << preSeq << " " << seq << " " << bth_header.GetSize() << std::endl;
        if(seq <= preSeq) {
            std::cout << "Duplicate or out-of-order packet" << std::endl;
            return;
        }
        else if(seq - preSeq == bth_header.GetSize()) {
            m_rdmaReceiver[key].first = seq;
            if(protocol == 0x0800) SendACK(ipv4_header, key);
            else if(protocol == 0x86DD) SendACK(ipv6_header, key);
        }
        else {
            std::cerr << "RDMA sequence error: " << seq << " - " << m_rdmaReceiver[key].first
                    << " not matching " << bth_header.GetSize() << std::endl;
            if(protocol == 0x0800) SendACK(ipv4_header, key, true);
            else if(protocol == 0x86DD) SendACK(ipv6_header, key, true);
        }
    }
}

void 
PointToPointNetDevice::SendACK(Ipv4Header& header, std::pair<Address, uint32_t> key, bool isNack)
{
    Ptr<Packet> packet = Create<Packet>();

    BthHeader bth_header;
    bth_header.SetSize(0);
    bth_header.SetId(key.second);
    bth_header.SetSequence(m_rdmaReceiver[key].first);
    if(isNack) {
        bth_header.SetNACK();
        if(Simulator::Now().GetNanoSeconds() - m_rdmaReceiver[key].second > 40000){
            bth_header.SetCNP();
            m_rdmaReceiver[key].second = Simulator::Now().GetNanoSeconds();
        }
    } else {
        bth_header.SetACK();
        if(header.GetEcn() == Ipv4Header::EcnType::ECN_CE){
            if(Simulator::Now().GetNanoSeconds() - m_rdmaReceiver[key].second > 40000){
                bth_header.SetCNP();
                m_rdmaReceiver[key].second = Simulator::Now().GetNanoSeconds();
            }
        }
    }
    packet->AddHeader(bth_header);

    UdpHeader udp_header;
    udp_header.SetSourcePort(key.second >> 16);
	udp_header.SetDestinationPort(key.second);
    packet->AddHeader(udp_header);

    header.SetPayloadSize(20);
    header.SetTtl(64);
    Ipv4Address tmp = header.GetSource();
    header.SetSource(header.GetDestination());
    header.SetDestination(tmp);
    packet->AddHeader(header);

    Send(packet, GetBroadcast(), 0x0800);
}

void 
PointToPointNetDevice::SendACK(Ipv6Header& header, std::pair<Address, uint32_t> key, bool isNack)
{
    Ptr<Packet> packet = Create<Packet>();

    BthHeader bth_header;
    bth_header.SetSize(0);
    bth_header.SetId(key.second);
    bth_header.SetSequence(m_rdmaReceiver[key].first);
    if(isNack) {
        bth_header.SetNACK();
        if(Simulator::Now().GetNanoSeconds() - m_rdmaReceiver[key].second > 40000){
            bth_header.SetCNP();
            m_rdmaReceiver[key].second = Simulator::Now().GetNanoSeconds();
        }
    } else {
        bth_header.SetACK();
        if(header.GetEcn() == Ipv6Header::EcnType::ECN_CE){
            if(Simulator::Now().GetNanoSeconds() - m_rdmaReceiver[key].second > 40000){
                bth_header.SetCNP();
                m_rdmaReceiver[key].second = Simulator::Now().GetNanoSeconds();
            }
        }
    }
    packet->AddHeader(bth_header);

    UdpHeader udp_header;
    udp_header.SetSourcePort(key.second >> 16);
	udp_header.SetDestinationPort(key.second);
    packet->AddHeader(udp_header);

    header.SetPayloadLength(20);
    header.SetHopLimit(64);
    Ipv6Address tmp = header.GetSource();
    header.SetSource(header.GetDestination());
    header.SetDestination(tmp);
    packet->AddHeader(header);

    Send(packet, GetBroadcast(), 0x86DD);
}

Ptr<Queue<Packet>>
PointToPointNetDevice::GetQueue() const
{
    NS_LOG_FUNCTION(this);
    return m_queue;
}

void
PointToPointNetDevice::NotifyLinkUp()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = true;
    m_linkChangeCallbacks();
}

void
PointToPointNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(this);
    m_ifIndex = index;
}

uint32_t
PointToPointNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
PointToPointNetDevice::GetChannel() const
{
    return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
PointToPointNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_address = Mac48Address::ConvertFrom(address);
}

Address
PointToPointNetDevice::GetAddress() const
{
    return m_address;
}

bool
PointToPointNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return m_linkUp;
}

void
PointToPointNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this);
    m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
PointToPointNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

//
// We don't really need any addressing information since this is a
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
PointToPointNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool
PointToPointNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
PointToPointNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this);
    return Mac48Address("01:00:5e:00:00:00");
}

Address
PointToPointNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    return Mac48Address("33:33:00:00:00:00");
}

bool
PointToPointNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

bool
PointToPointNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
PointToPointNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    NS_LOG_LOGIC("p=" << packet << ", dest=" << &dest);
    NS_LOG_LOGIC("UID is " << packet->GetUid());

    //
    // If IsLinkUp() is false it means there is no channel to send any packet
    // over so we just hit the drop trace on the packet and return an error.
    //
    if (IsLinkUp() == false)
    {
        m_macTxDropTrace(packet);
        return false;
    }

    if(m_id != 0){
        uint64_t t = Simulator::Now().GetNanoSeconds();
        if(protocolNumber == 0x0800){
            m_userCount += 1;
            auto v4Id = getFlowV4Id(packet);
            Ipv4Header ipv4_header;
            packet->RemoveHeader(ipv4_header);
            SetPriority(packet, ipv4_header.GetProtocol());
            if(m_setting == CompressType::COMPRESS_MPLS){
                if(m_compress4.find(v4Id) != m_compress4.end()){
                    m_mplsCount += 1;
                    PortHeader port_header;
                    packet->RemoveHeader(port_header);
                    CompressIpHeader compressIpHeader;
                    compressIpHeader.SetIpv4Header(ipv4_header);
                    packet->AddHeader(compressIpHeader);

                    MplsHeader mpls_header;
                    mpls_header.SetLabel(m_compress4[v4Id]);
                    mpls_header.SetExp(ipv4_header.GetEcn());
                    mpls_header.SetTtl(ipv4_header.GetTtl());
                    packet->AddHeader(mpls_header);

                    if(t - m_v4count[v4Id].second > 10000000){
                        m_v4count[v4Id].first = 0;
                        m_v4count[v4Id].second = t;
                    }
                    m_v4count[v4Id].first += 1;
                    if(m_v4count[v4Id].first == m_threshold){
                        Simulator::Schedule(NanoSeconds(1), &PointToPointNetDevice::GenData4, this, v4Id);
                    }
                    protocolNumber = 0x8847;
                }
                else{
                    if(t - m_v4count[v4Id].second > 10000000){
                        m_v4count[v4Id].first = 0;
                        m_v4count[v4Id].second = t;
                    }
                    m_v4count[v4Id].first += 1;
                    if(m_v4count[v4Id].first == m_threshold){
                        Simulator::Schedule(NanoSeconds(1), &PointToPointNetDevice::GenData4, this, v4Id);
                    }
                    packet->AddHeader(ipv4_header);
                }
            }
            else if(m_setting == CompressType::COMPRESS_IDEAL)
                protocolNumber = m_idealCom.Process(packet, ipv4_header);
            else packet->AddHeader(ipv4_header);
        }
        else if(protocolNumber == 0x86DD){
            m_userCount += 1;
            auto v6Id = getFlowV6Id(packet);
            Ipv6Header ipv6_header;
            packet->RemoveHeader(ipv6_header);
            SetPriority(packet, ipv6_header.GetNextHeader());
            if(m_vxlan){
                packet->AddHeader(ipv6_header);
                EncapVxLAN(packet);
                packet->RemoveHeader(ipv6_header);
            }
            if(m_setting == CompressType::COMPRESS_MPLS){
                if(m_compress6.find(v6Id) != m_compress6.end()){
                    m_mplsCount += 1;
                    if(m_vxlan){
                        UdpHeader udp_header;
                        VxlanHeader vxlan_header;
                        PppHeader ppp_header;
                        Ipv6Header tmp_header;
                        packet->RemoveHeader(udp_header);
                        packet->RemoveHeader(vxlan_header);
                        packet->RemoveHeader(ppp_header);
                        packet->RemoveHeader(tmp_header);
                    }
                    PortHeader port_header;
                    packet->RemoveHeader(port_header);
                    CompressIpHeader compressIpHeader;
                    compressIpHeader.SetIpv6Header(ipv6_header);
                    packet->AddHeader(compressIpHeader);

                    MplsHeader mpls_header;
                    mpls_header.SetLabel(m_compress6[v6Id]);
                    mpls_header.SetExp(ipv6_header.GetEcn());
                    mpls_header.SetTtl(ipv6_header.GetHopLimit());
                    packet->AddHeader(mpls_header);

                    if(t - m_v6count[v6Id].second > 10000000){
                        m_v6count[v6Id].first = 0;
                        m_v6count[v6Id].second = t;
                    }
                    m_v6count[v6Id].first += 1;
                    if(m_v6count[v6Id].first == m_threshold){
                        Simulator::Schedule(NanoSeconds(1), &PointToPointNetDevice::GenData6, this, v6Id);
                    }
                    protocolNumber = 0x8847;
                }
                else{
                    if(t - m_v6count[v6Id].second > 10000000){
                        m_v6count[v6Id].first = 0;
                        m_v6count[v6Id].second = t;
                    }
                    m_v6count[v6Id].first += 1;
                    if(m_v6count[v6Id].first == m_threshold){
                        Simulator::Schedule(NanoSeconds(1), &PointToPointNetDevice::GenData6, this, v6Id);
                    }
                    packet->AddHeader(ipv6_header);
                }
            }
            else if(m_setting == CompressType::COMPRESS_IDEAL)
                protocolNumber = m_idealCom.Process(packet, ipv6_header);
            else packet->AddHeader(ipv6_header);
        }
    }

    //
    // Stick a point to point protocol header on the packet in preparation for
    // shoving it out the door.
    //
    AddHeader(packet, protocolNumber);
    
    m_macTxTrace(packet);

    //
    // We should enqueue and dequeue the packet to hit the tracing hooks.
    //
    if (m_queue->Enqueue(packet))
    {
        //
        // If the channel is ready for transition we send the packet right now
        //
        if (m_txMachineState == READY)
        {
            packet = m_queue->Dequeue(m_pause);
            if(packet != nullptr)
            {
                m_snifferTrace(packet);
                m_promiscSnifferTrace(packet);
                TransmitStart(packet);
            }
            return true;
        }
        return true;
    }

    // Enqueue may fail (overflow)

    m_macTxDropTrace(packet);
    return false;
}

bool
PointToPointNetDevice::SendFrom(Ptr<Packet> packet,
                                const Address& source,
                                const Address& dest,
                                uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << source << dest << protocolNumber);
    return false;
}

Ptr<Node>
PointToPointNetDevice::GetNode() const
{
    return m_node;
}

void
PointToPointNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    m_node = node;
}

bool
PointToPointNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
PointToPointNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    m_rxCallback = cb;
}

void
PointToPointNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    m_promiscCallback = cb;
}

bool
PointToPointNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
PointToPointNetDevice::DoMpiReceive(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    Receive(p);
}

Address
PointToPointNetDevice::GetRemote() const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_channel->GetNDevices() == 2);
    for (std::size_t i = 0; i < m_channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> tmp = m_channel->GetDevice(i);
        if (tmp != this)
        {
            return tmp->GetAddress();
        }
    }
    NS_ASSERT(false);
    // quiet compiler.
    return Address();
}

bool
PointToPointNetDevice::SetMtu(uint16_t mtu)
{
    NS_LOG_FUNCTION(this << mtu);
    m_mtu = mtu;
    return true;
}

uint16_t
PointToPointNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    return m_mtu;
}

void 
PointToPointNetDevice::SetSetting(int setting)
{
    m_setting = CompressType(setting);
}

void
PointToPointNetDevice::SetVxLAN(uint32_t vxlan)
{
    m_vxlan = vxlan;
}

void
PointToPointNetDevice::SetThreshold(uint32_t threshold)
{
    m_threshold = threshold;
}

void
PointToPointNetDevice::SetRdma(uint32_t rdma)
{
    m_rdma = rdma;
}

uint64_t 
PointToPointNetDevice::GetUserCount()
{
    return m_userCount;
}

uint64_t 
PointToPointNetDevice::GetMplsCount()
{
    return m_mplsCount;
}

void 
PointToPointNetDevice::SetUserCount(uint64_t count)
{
    m_userCount = count;
}
    
void 
PointToPointNetDevice::SetMplsCount(uint64_t count)
{
    m_mplsCount = count;
}

void 
PointToPointNetDevice::SetPriority(Ptr<Packet> packet, uint8_t protocol)
{
    SocketPriorityTag tag;
    if(protocol == 6){
        TcpHeader tcp_header;
        packet->PeekHeader(tcp_header);
        if(tcp_header.GetLength() * 4 == packet->GetSize())
            tag.SetPriority(1);
        else
            tag.SetPriority(2);
    } else if (protocol == 17) {
        if(packet->GetSize() == 20)
            tag.SetPriority(1);
        else 
            tag.SetPriority(2);
    } else {
        std::cout << "Unknown Protocol " << uint32_t(protocol) << " for SetPriority" << std::endl;
    }
    packet->ReplacePacketTag(tag);
}

void
PointToPointNetDevice::DecapVxLAN(Ptr<Packet> packet){
    Ipv6Header ipv6_header;
    UdpHeader udp_header;
    VxlanHeader vxlan_header;
    PppHeader ppp_header;
    Ipv6Header tmp_header;

    packet->RemoveHeader(ipv6_header);
    ipv6_header.SetNextHeader(6);
    packet->RemoveHeader(udp_header);
    packet->RemoveHeader(vxlan_header);
    packet->RemoveHeader(ppp_header);
    packet->RemoveHeader(tmp_header);
    packet->AddHeader(ipv6_header);
}

void
PointToPointNetDevice::EncapVxLAN(Ptr<Packet> packet){
    Ipv6Header ipv6_header;
    PortHeader port_header;
    PppHeader ppp_header;
    VxlanHeader vxlan_header;
    UdpHeader udp_header;

    packet->RemoveHeader(ipv6_header);
    ipv6_header.SetNextHeader(17);

    packet->PeekHeader(port_header);
    packet->AddHeader(ipv6_header);
    packet->AddHeader(ppp_header);
    packet->AddHeader(vxlan_header);

    udp_header.SetSourcePort(port_header.GetSourcePort());
    udp_header.SetDestinationPort(port_header.GetDestinationPort());
    packet->AddHeader(udp_header);
    packet->AddHeader(ipv6_header);
}

void
PointToPointNetDevice::GenData4(FlowV4Id id){
    CommandHeader cmd;
    cmd.SetType(CommandHeader::NICData4);
    cmd.SetFlow4Id(id);
    SendCommand(cmd);
}

void
PointToPointNetDevice::GenData6(FlowV6Id id){
    CommandHeader cmd;
    cmd.SetType(CommandHeader::NICData6);
    cmd.SetFlow6Id(id);
    SendCommand(cmd);
}

void 
PointToPointNetDevice::SendCommand(CommandHeader& cmd)
{
    Ptr<Packet> packet = Create<Packet>();

    cmd.SetSourceId(m_id);
    cmd.SetDestinationId(0xffff);
    packet->AddHeader(cmd);

    SocketPriorityTag tag;
    tag.SetPriority(0);
    packet->ReplacePacketTag(tag);

    Send(packet, GetBroadcast(), 0x0170);
}

void
PointToPointNetDevice::UpdateCompress4(CommandHeader cmd)
{
    m_compress4[cmd.GetFlow4Id()] = cmd.GetLabel();
}

void
PointToPointNetDevice::UpdateDecompress4(CommandHeader cmd)
{
    m_decompress4[cmd.GetLabel()] = cmd.GetFlow4Id();
}

void
PointToPointNetDevice::UpdateCompress6(CommandHeader cmd)
{
    m_compress6[cmd.GetFlow6Id()] = cmd.GetLabel();
}

void
PointToPointNetDevice::UpdateDecompress6(CommandHeader cmd)
{
    m_decompress6[cmd.GetLabel()] = cmd.GetFlow6Id();
}

void
PointToPointNetDevice::DeleteCompress4(CommandHeader cmd)
{
    if(m_compress4.find(cmd.GetFlow4Id()) != m_compress4.end())
        m_compress4.erase(cmd.GetFlow4Id());
    else
        std::cout << "Fail to find compress4 flow id in " << m_id << std::endl;
}

void
PointToPointNetDevice::DeleteCompress6(CommandHeader cmd)
{
    if(m_compress6.find(cmd.GetFlow6Id()) != m_compress6.end())
        m_compress6.erase(cmd.GetFlow6Id());
    else
        std::cout << "Fail to find compress6 flow id in " << m_id << std::endl;
}

uint16_t
PointToPointNetDevice::PppToEther(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0021: return 0x0800; // IPv4
    case 0x0057: return 0x86DD; // IPv6
    case 0x0281: return 0x8847; // MPLS  
    case 0x0170: return 0x0170; // Command
    case 0x0171: return 0x0171; // Ideal
    case 0x0172: return 0x0172; // ROHC 
    case 0x8808: return 0x8808; // Ethernet Control Protocol
    default: NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    return 0;
}

uint16_t
PointToPointNetDevice::EtherToPpp(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0800: return 0x0021; // IPv4
    case 0x86DD: return 0x0057; // IPv6
    case 0x8847: return 0x0281; // MPLS 
    case 0x0170: return 0x0170; // Command   
    case 0x0171: return 0x0171; // Ideal  
    case 0x0172: return 0x0172; // ROHC
    case 0x8808: return 0x8808; // Ethernet Control Protocol
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    return 0;
}

} // namespace ns3
