#ifndef RDMA_SCHEDULER_H
#define RDMA_SCHEDULER_H

#include <stdio.h>

#include "ns3/node.h"
#include "ns3/rdma-queue-pair.h"

namespace ns3
{

class RdmaScheduler : public Object
{
	public:
		static TypeId GetTypeId();

    	RdmaScheduler(std::string file, std::string fctFile, uint32_t ipVersion, std::vector<Ptr<PointToPointNetDevice>> nics, 
					std::vector<Ipv4Address> v4addr, std::vector<Ipv6Address> v6addr);
		~RdmaScheduler();

		void Run();
		void Schedule();
		Ptr<RdmaQueuePair> GetAvailableQP(uint32_t src, uint32_t dst);

	private:
		uint32_t m_ipVersion{0};
		uint32_t m_id{10000};
		std::vector<Ptr<PointToPointNetDevice>> m_nics;
		std::vector<Ipv4Address> m_v4addr;
		std::vector<Ipv6Address> m_v6addr;

		std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<RdmaQueuePair>>> m_qp;
		std::unordered_map<uint32_t, FlowInfo> m_fctMp;

		FILE* m_file;
		FILE* m_fctFile;
		FlowInfo m_flow;
};

} // namespace ns3

#endif /* RDMA_SCHEDULER_H */