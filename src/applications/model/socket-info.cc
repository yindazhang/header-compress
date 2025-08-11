
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket.h"

#include "socket-info.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SocketInfo");

NS_OBJECT_ENSURE_REGISTERED(SocketInfo);

TypeId
SocketInfo::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SocketInfo")
                            .SetParent<Object>()
                            .SetGroupName("Applications")
							.AddAttribute("SendSize",
								"The amount of data to send each time.",
								UintegerValue(1400),
								MakeUintegerAccessor(&SocketInfo::m_sendSize),
								MakeUintegerChecker<uint32_t>())
                            .AddAttribute("SrcPort",
                                "The source port.",
                                UintegerValue(0),
                                MakeUintegerAccessor(&SocketInfo::m_srcPort),
                                MakeUintegerChecker<uint16_t>());
    return tid;
}

void ConnectionSucceeded() {}

void ConnectionFailed() {
	std::cerr << "Connection failed" << std::endl;
}

void 
SocketInfo::Init(){
	m_socket = Socket::CreateSocket(m_srcNode, TcpSocketFactory::GetTypeId());
	if (Inet6SocketAddress::IsMatchingType(m_dstAddr)){
		m_socket->Bind(Inet6SocketAddress(Ipv6Address::GetAny(), m_srcPort));
	}
	else if (InetSocketAddress::IsMatchingType(m_dstAddr)){
		m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_srcPort));
	}
	m_socket->Connect(m_dstAddr);
	m_socket->SetConnectCallback(
		MakeCallback(&ConnectionSucceeded),
		MakeCallback(&ConnectionFailed));
}

void 
SocketInfo::Connect(double delay){
	Simulator::Schedule(Seconds(delay), &SocketInfo::Init, this);
}

bool
SocketInfo::GetSending(){
	return m_sending;
}

void
SocketInfo::SetFlow(uint32_t id, uint32_t totalBytes, 
	std::unordered_map<uint32_t, FlowInfo>* fctMp, FILE* fctFile){
	m_sending = true;
	m_id = id;
	m_bytesSent = 0;
	m_totalBytes = totalBytes;
	m_unsentPacket = nullptr;
	m_socket->SetSendCallback(MakeCallback(&SocketInfo::SendData, this));

	m_fctMp = fctMp;
	m_fctFile = fctFile;
}

void 
SocketInfo::WriteFCT(){
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

void 
SocketInfo::SendData(Ptr<Socket>, uint32_t){
	if(!m_sending)
		return;
	while (m_bytesSent < m_totalBytes){
		uint64_t toSend = std::min(m_sendSize, m_totalBytes - m_bytesSent);
		Ptr<Packet> packet;
		if (m_unsentPacket)
		{
			packet = m_unsentPacket;
			toSend = packet->GetSize();
		}
		else
		{
			packet = Create<Packet>(toSend);
		}

		int actual = m_socket->Send(packet);

		if (actual == -1)
		{
			m_unsentPacket = packet;
			break;
		}
		else if ((unsigned)actual == toSend)
		{
			m_bytesSent += toSend;
			m_unsentPacket = nullptr;
		}
		else if (actual > 0 && (unsigned)actual < toSend)
		{
			std::cerr << "Fragment happen" << std::endl;
			Ptr<Packet> sent = packet->CreateFragment(0, actual);
			Ptr<Packet> unsent = packet->CreateFragment(actual, (toSend - (unsigned)actual));
			m_bytesSent += actual;
			m_unsentPacket = unsent;
			break;
		}
		else
		{
			std::cout << "Actual: " << actual << ", toSend: " << toSend << std::endl;
			NS_FATAL_ERROR("Unexpected return value from m_socket->Send ()");
		}
	}
	Ptr<TcpSocket> tcpSocket = DynamicCast<TcpSocket>(m_socket);
	if(tcpSocket == nullptr){
		std::cerr << "Not TCP socket" << std::endl;
		exit(1);
	}
	if(m_socket->GetTxAvailable() == tcpSocket->GetSndBufSize()){
		WriteFCT();
		m_sending = false;
	}
}

} // namespace ns3