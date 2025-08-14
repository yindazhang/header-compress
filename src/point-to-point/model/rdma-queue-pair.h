#ifndef RDMA_QUEUE_PAIR_H
#define RDMA_QUEUE_PAIR_H

#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/socket-info.h"

#include "bth-header.h"
#include "switch-node.h"
#include "point-to-point-net-device.h"

namespace ns3
{

class PointToPointNetDevice;

class RdmaQueuePair : public Object
{
	public:
		static TypeId GetTypeId();

    	RdmaQueuePair(Ptr<PointToPointNetDevice> device = nullptr, Address srcAddr = Address(), Address dstAddr = Address(), uint32_t qp = 0):
        	m_device(device), m_srcAddr(srcAddr), m_dstAddr(dstAddr), m_qp(qp){};

		uint32_t GetQP();
	
		void SetFlow(uint32_t id, uint32_t totalBytes,
			std::unordered_map<uint32_t, FlowInfo>* fctMp, FILE* fctFile);

		bool GetSending();

		bool ProcessACK(BthHeader& bth);

		void ScheduleSend();

	private:
		Ptr<PointToPointNetDevice> m_device;
		Address m_srcAddr;
		Address m_dstAddr;

		bool m_cnpAlpha;

		double m_alpha{1.0};
		double m_g{1.0/16.0};

		double m_targetRate; // GB/s
		double m_sendRate{12.1};
		double m_maxRate{12.5};
		double m_minRate{0.05};

		uint32_t m_qp;
		uint32_t m_id;
		uint32_t m_timeStage{0};
		uint32_t m_sendSize{1000};

		uint64_t m_bytesAcked{0};
		uint64_t m_bytesSent{0};
		uint64_t m_totalBytes{0};

		FILE* m_fctFile;
		std::unordered_map<uint32_t, FlowInfo>* m_fctMp{nullptr};

		EventId m_updateAlpha;
		EventId m_increaseRate;
		EventId m_nextSend;

		void WriteFCT();
		void UpdateAlpha();
		void DecreaseRate();
		void IncreaseRate();

		Ptr<Packet> GenerateNextPacket();
};

} // namespace ns3

#endif /* RDMA_QUEUE_PAIR */