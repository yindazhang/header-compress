
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket.h"
#include "ns3/udp-header.h"
#include "ns3/bth-header.h"

#include "rdma-queue-pair.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RdmaQueuePair");

NS_OBJECT_ENSURE_REGISTERED(RdmaQueuePair);

TypeId
RdmaQueuePair::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RdmaQueuePair")
                            .SetParent<Object>()
                            .SetGroupName("PointToPoint")
							.AddAttribute("SendSize",
								"The amount of data to send each time.",
								UintegerValue(1000),
								MakeUintegerAccessor(&RdmaQueuePair::m_sendSize),
								MakeUintegerChecker<uint32_t>());
    return tid;
}

uint32_t 
RdmaQueuePair::GetQP()
{
	return m_qp;
}

bool
RdmaQueuePair::GetSending()
{
	return !(m_totalBytes == m_bytesAcked);
}

void
RdmaQueuePair::SetFlow(uint32_t id, uint32_t totalBytes,
	std::unordered_map<uint32_t, FlowInfo>* fctMp, FILE* fctFile)
{
	m_id = id;
	m_totalBytes += totalBytes;

	m_cnpAlpha = false;
	m_alpha = 1;
	m_timeStage = 0;
	m_sendRate = 12.1;

	m_fctMp = fctMp;
	m_fctFile = fctFile;

	m_device->AddQP(this);

	auto packet = GenerateNextPacket();
	if(packet != nullptr) {
		if (Ipv6Address::IsMatchingType(m_dstAddr))
			m_device->Send(packet, m_device->GetBroadcast(), 0x86DD);
		else
			m_device->Send(packet, m_device->GetBroadcast(), 0x0800);
	} else {
		std::cerr << "Nothing to send at the beginning" << std::endl;
	}
	m_nextSend = Simulator::Schedule(NanoSeconds(m_sendSize / m_sendRate), &RdmaQueuePair::ScheduleSend, this);
}

void 
RdmaQueuePair::WriteFCT(){
	if((*m_fctMp)[m_id].end == 0){
		(*m_fctMp)[m_id].end = Simulator::Now().GetNanoSeconds();
		fprintf(m_fctFile, "%u,%u,%u,%u,%u,%u,%u\n",
			(*m_fctMp)[m_id].index, (*m_fctMp)[m_id].src, (*m_fctMp)[m_id].dst,
			(*m_fctMp)[m_id].size, (*m_fctMp)[m_id].start, (*m_fctMp)[m_id].end,
			(*m_fctMp)[m_id].end - (*m_fctMp)[m_id].start
		);
		fflush(m_fctFile);
	}
}

bool
RdmaQueuePair::ProcessACK(BthHeader& bth)
{
	if(bth.GetId() != m_qp)
		std::cerr << "QP ID does not match" << std::endl;
	if(bth.GetACK()){
		uint64_t seq = bth.GetSequence(m_bytesAcked);
		m_bytesAcked = std::max(m_bytesAcked, seq);
		// std::cout << "ACK: " << seq << " " << m_bytesAcked << " " << m_bytesSent << std::endl;
		if(m_bytesAcked > m_bytesSent){
			std::cerr << "m_bytesAcked > m_bytesSent in RDMA" << std::endl;
			m_bytesSent = m_bytesAcked;
		}
		else if(m_bytesAcked == m_bytesSent){
			WriteFCT();
			Simulator::Cancel(m_updateAlpha);
			Simulator::Cancel(m_increaseRate);
			Simulator::Cancel(m_nextSend);
			return true;
		}
	}
	else if(bth.GetNACK()){
		std::cerr << "Receive NACK" << std::endl;
		m_bytesSent = m_bytesAcked;
	}
	else std::cerr << "Unknown BTH flags" << std::endl;

	if(bth.GetCNP()){
		m_cnpAlpha = true;
		UpdateAlpha();
		DecreaseRate();
		m_timeStage = 0;
		Simulator::Cancel(m_increaseRate);
		m_increaseRate = Simulator::Schedule(MicroSeconds(100), &RdmaQueuePair::IncreaseRate, this);
	}
	
	if(bth.GetNACK()){
		Simulator::Cancel(m_nextSend);
		auto packet = GenerateNextPacket();
		if(packet != nullptr) {
			if (Ipv6Address::IsMatchingType(m_dstAddr))
				m_device->Send(packet, m_device->GetBroadcast(), 0x86DD);
			else
				m_device->Send(packet, m_device->GetBroadcast(), 0x0800);
		} else {
			std::cerr << "Nothing to send in NACK" << std::endl;
		}
		m_nextSend = Simulator::Schedule(NanoSeconds(m_sendSize / m_sendRate), &RdmaQueuePair::ScheduleSend, this);
	}
	return false;
}

void
RdmaQueuePair::UpdateAlpha()
{
	Simulator::Cancel(m_updateAlpha);
	if(m_cnpAlpha) m_alpha = (1 - m_g) * m_alpha + m_g;
	else m_alpha = (1 - m_g) * m_alpha;
	m_cnpAlpha = false;
	m_updateAlpha = Simulator::Schedule(MicroSeconds(20), &RdmaQueuePair::UpdateAlpha, this);
}

void 
RdmaQueuePair::DecreaseRate()
{
	m_targetRate = m_sendRate;
	m_sendRate = std::max(m_minRate, m_sendRate * (1 - m_alpha / 2.0));
}

void 
RdmaQueuePair::IncreaseRate()
{
	Simulator::Cancel(m_increaseRate);
	if(m_timeStage > 0) m_targetRate = std::min(m_maxRate, m_targetRate + 0.01);
	m_sendRate = (m_targetRate + m_sendRate) / 2.0;
	m_timeStage += 1;
	m_increaseRate = Simulator::Schedule(MicroSeconds(100), &RdmaQueuePair::IncreaseRate, this);
}

void
RdmaQueuePair::ScheduleSend()
{
	if(!m_device->Available()){
		m_nextSend = Simulator::Schedule(NanoSeconds(1000), &RdmaQueuePair::ScheduleSend, this);
		return;
	}
	auto packet = GenerateNextPacket();
	if(packet != nullptr) {
		if (Ipv6Address::IsMatchingType(m_dstAddr))
			m_device->Send(packet, m_device->GetBroadcast(), 0x86DD);
		else
			m_device->Send(packet, m_device->GetBroadcast(), 0x0800);
		m_nextSend = Simulator::Schedule(NanoSeconds(m_sendSize / m_sendRate), &RdmaQueuePair::ScheduleSend, this);
	}
}

Ptr<Packet>
RdmaQueuePair::GenerateNextPacket()
{
	if(m_totalBytes <= m_bytesSent)
		return nullptr;

	uint64_t toSend = std::min(m_totalBytes - m_bytesSent, uint64_t(m_sendSize));
	Ptr<Packet> ret = Create<Packet>(toSend);

	BthHeader bth_header;
	bth_header.SetSize(toSend);
	bth_header.SetId(m_qp);
	bth_header.SetSequence(m_bytesSent + toSend);
	ret->AddHeader(bth_header);

	UdpHeader udp_header;
	udp_header.SetSourcePort(m_qp >> 16);
	udp_header.SetDestinationPort(m_qp);
	ret->AddHeader(udp_header);

	if (Ipv6Address::IsMatchingType(m_dstAddr)){
		Ipv6Header ipv6_header;
		ipv6_header.SetEcn(Ipv6Header::EcnType::ECN_ECT0);
		ipv6_header.SetPayloadLength(toSend + 20);
		ipv6_header.SetNextHeader(17);
		ipv6_header.SetHopLimit(64);
		ipv6_header.SetSource(Ipv6Address::ConvertFrom(m_srcAddr));
		ipv6_header.SetDestination(Ipv6Address::ConvertFrom(m_dstAddr));
		ret->AddHeader(ipv6_header);
	} else {
		Ipv4Header ipv4_header;
		ipv4_header.SetEcn(Ipv4Header::EcnType::ECN_ECT0);
		ipv4_header.SetPayloadSize(toSend + 20);
		ipv4_header.SetProtocol(17);
		ipv4_header.SetTtl(64);
		ipv4_header.SetSource(Ipv4Address::ConvertFrom(m_srcAddr));
		ipv4_header.SetDestination(Ipv4Address::ConvertFrom(m_dstAddr));
		ret->AddHeader(ipv4_header);
	}

	m_bytesSent += toSend;
	// std::cout << "Send " << m_bytesSent << " " << toSend << " " << m_bytesAcked << std::endl;
	return ret;
}

} // namespace ns3