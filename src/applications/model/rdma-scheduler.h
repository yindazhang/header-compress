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

    	RdmaScheduler(std::string file, std::string fctFile);
		~RdmaScheduler();

		void SetQP(std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<RdmaQueuePair>>>* qp);

		void Run();
		void Schedule();
		Ptr<RdmaQueuePair> GetAvailableQP(uint32_t src, uint32_t dst);

	private:
		std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<RdmaQueuePair>>>* m_qp{nullptr};

		std::unordered_map<uint32_t, FlowInfo> m_fctMp;

		FILE* m_file;
		FILE* m_fctFile;
		FlowInfo m_flow;
};

} // namespace ns3

#endif /* RDMA_SCHEDULER_H */