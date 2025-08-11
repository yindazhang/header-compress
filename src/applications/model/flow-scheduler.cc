#include "flow-scheduler.h"

#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlowScheduler");

NS_OBJECT_ENSURE_REGISTERED(FlowScheduler);

TypeId
FlowScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FlowScheduler")
                            .SetParent<Object>()
                            .SetGroupName("Applications");
    return tid;
}

FlowScheduler::FlowScheduler(std::string file, std::string fctFile)
{
	if((m_file = fopen(("trace/" + file + ".tr").c_str(), "r")) == NULL) {
		std::cerr << "Failed to open flow file" << std::endl;
		exit(1);
	}
	if((m_fctFile = fopen((fctFile + ".fct").c_str(), "w")) == nullptr){
		std::cerr << "Failed to open fct file" << std::endl;
		exit(1);
	}
}

FlowScheduler::~FlowScheduler()
{
	fclose(m_file);
	fclose(m_fctFile);
	if(m_sockets)
		delete m_sockets;
}

void 
FlowScheduler::SetSockets(std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<SocketInfo>>>* sockets)
{
	m_sockets = sockets;
}

void
FlowScheduler::Run()
{
	m_fctMp[m_flow.index] = m_flow;
	auto socket = GetAvailableSocketInfo(m_flow.src, m_flow.dst);
	if(socket == nullptr){
		std::cerr << "NULL socket " << std::endl;
		exit(1);
	}
	socket->SetFlow(m_flow.index, m_flow.size, &m_fctMp, m_fctFile);
	socket->SendData(nullptr, m_flow.size);
	Schedule();
}

void
FlowScheduler::Schedule()
{
	char line[100];
	if (fgets(line, sizeof(line), m_file)) {
        if (sscanf(line, "%u %u %u %u", &m_flow.src, &m_flow.dst, &m_flow.size, &m_flow.start) == 4) {
			m_flow.index += 1;
            if(NanoSeconds(m_flow.start) != Simulator::Now())
				Simulator::Schedule(NanoSeconds(m_flow.start) - Simulator::Now(), &FlowScheduler::Run, this);
			else Run();
        }
    }
}

Ptr<SocketInfo> 
FlowScheduler::GetAvailableSocketInfo(uint32_t src, uint32_t dst)
{
	for(auto socketInfo :(*m_sockets)[std::make_pair(src, dst)]){
		if(!socketInfo->GetSending()){
			return socketInfo;
		}
	}
	std::cerr << "No available socket. NUM_SOCKET is too small for src " << src << " dst " << dst << std::endl;
	exit(1);
}

} // namespace ns3
