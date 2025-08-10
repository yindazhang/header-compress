#ifndef FLOW_SCHEDULER_H
#define FLOW_SCHEDULER_H

#include <stdio.h>

#include "socket-info.h"

#include "ns3/node.h"

namespace ns3
{

class FlowScheduler : public Object
{
	public:
		static TypeId GetTypeId();

    	FlowScheduler(std::string file, std::string fctFile, std::unordered_map<uint32_t, FlowInfo>* fctMp);
		~FlowScheduler();

		void SetSockets(std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<SocketInfo>>>* sockets);

		void Run();
		void Schedule();
		Ptr<SocketInfo> GetAvailableSocketInfo(uint32_t src, uint32_t dst);

	private:
		std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<SocketInfo>>>* m_sockets{nullptr};

		std::unordered_map<uint32_t, FlowInfo>* m_fctMp{nullptr};

		FILE* m_file;
		FILE* m_fctFile;
		FlowInfo m_flow;
};

} // namespace ns3

#endif /* FLOW_SCHEDULER */