#include "rdma-scheduler.h"

#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RdmaScheduler");

NS_OBJECT_ENSURE_REGISTERED(RdmaScheduler);

TypeId
RdmaScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RdmaScheduler")
                            .SetParent<Object>()
                            .SetGroupName("Applications");
    return tid;
}

RdmaScheduler::RdmaScheduler(std::string file, std::string fctFile, uint32_t ipVersion, std::vector<Ptr<PointToPointNetDevice>> nics, 
					std::vector<Ipv4Address> v4addr, std::vector<Ipv6Address> v6addr)
{
	m_ipVersion = ipVersion;
	m_nics = nics;
	m_v4addr = v4addr;
	m_v6addr = v6addr;
	if((m_file = fopen(("trace/" + file + ".tr").c_str(), "r")) == NULL) {
		std::cerr << "Failed to open flow file" << std::endl;
		exit(1);
	}
	if((m_fctFile = fopen((fctFile + ".fct").c_str(), "w")) == nullptr){
		std::cerr << "Failed to open fct file" << std::endl;
		exit(1);
	}
}

RdmaScheduler::~RdmaScheduler()
{
	fclose(m_file);
	fclose(m_fctFile);
}

void
RdmaScheduler::Run()
{
	m_fctMp[m_flow.index] = m_flow;
	auto qp = GetAvailableQP(m_flow.src, m_flow.dst);
	if(qp == nullptr){
		std::cerr << "NULL RDMA queue pair " << std::endl;
		exit(1);
	}
	qp->SetFlow(m_flow.index, m_flow.size, &m_fctMp, m_fctFile);
	Schedule();
}

void
RdmaScheduler::Schedule()
{
	char line[100];
	if (fgets(line, sizeof(line), m_file)) {
        if (sscanf(line, "%u %u %u %u", &m_flow.src, &m_flow.dst, &m_flow.size, &m_flow.start) == 4) {
			m_flow.index += 1;
            if(NanoSeconds(m_flow.start) != Simulator::Now())
				Simulator::Schedule(NanoSeconds(m_flow.start) - Simulator::Now(), &RdmaScheduler::Run, this);
			else Run();
        }
    }
}

Ptr<RdmaQueuePair> 
RdmaScheduler::GetAvailableQP(uint32_t src, uint32_t dst)
{
	auto conn = std::make_pair(src, dst);
	for(auto qp :m_qp[conn]){
		if(!qp->GetSending()){
			return qp;
		}
	}
	Ptr<RdmaQueuePair> ret;
	if(m_ipVersion == 0)
		ret = Create<RdmaQueuePair>(m_nics[src], m_v4addr[src], m_v4addr[dst], m_id++);
	else
		ret = Create<RdmaQueuePair>(m_nics[src], m_v6addr[src], m_v6addr[dst], m_id++);
	m_qp[conn].push_back(ret);
	return ret;
}

} // namespace ns3
