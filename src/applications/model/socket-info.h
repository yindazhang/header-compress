#ifndef SOCKET_INFO_H
#define SOCKET_INFO_H

#include "ns3/node.h"
#include "ns3/socket.h"

namespace ns3
{

struct FlowInfo
{
	uint32_t index;
	uint32_t src;
	uint32_t dst;
	uint32_t size;
	uint32_t start;
	uint32_t end;

	FlowInfo(uint32_t _index = 0, uint32_t _src = 0, uint32_t _dst = 0,
		uint32_t _size = 0, uint32_t _start = 0, uint32_t _end = 0):
		index(_index), src(_src), dst(_dst), size(_size), start(_start), end(_end){};
};

class SocketInfo : public Object
{
	public:
		static TypeId GetTypeId();

    	SocketInfo(Ptr<Node> srcNode = nullptr, uint16_t srcPort = 0, Address dstAddr = Address()):
        	m_srcNode(srcNode), m_srcPort(srcPort), m_dstAddr(dstAddr){};
	
		void Init();
		void Connect(double delay);

		bool GetSending();
		void SetFlow(uint32_t id, uint32_t totalBytes);

		void SendData(Ptr<Socket>, uint32_t);

	private:
		Ptr<Node> m_srcNode;
		uint16_t m_srcPort;
		Address m_dstAddr;

		Ptr<Socket> m_socket;

		bool m_sending;

		uint32_t m_id;
		uint32_t m_bytesSent;
		uint32_t m_totalBytes;
		uint32_t m_sendSize{1400};

		Ptr<Packet> m_unsentPacket;
};

} // namespace ns3

#endif /* SOCKET_INFO */