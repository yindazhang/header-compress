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

RdmaScheduler::RdmaScheduler(std::string file, std::string fctFile)
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

RdmaScheduler::~RdmaScheduler()
{
	fclose(m_file);
	fclose(m_fctFile);
	if(m_qp)
		delete m_qp;
}

void 
RdmaScheduler::SetQP(std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<RdmaQueuePair>>>* qp)
{
	m_qp = qp;
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
	for(auto qp :(*m_qp)[std::make_pair(src, dst)]){
		if(!qp->GetSending()){
			return qp;
		}
	}
	std::cerr << "No available RDMA queue pair. NUM_QP is too small for src " << src << " dst " << dst << std::endl;
	exit(1);
}

} // namespace ns3
