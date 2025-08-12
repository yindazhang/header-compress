#include "topology.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HeaderCompress");

int
main(int argc, char* argv[])
{
	srand(11);
	
    //std::string flow_file = "Hadoop_216_0.5_25G_1.0";
	std::string flow_file = "test";

	CommandLine cmd(__FILE__);
	cmd.AddValue("time", "the total run time (s), by default 1.0", duration);
	cmd.AddValue("flow", "the flow file", flow_file);
	cmd.AddValue("ip_version", "0 for ipv4, 1 for ipv6", ip_version);
	cmd.AddValue("compress_version", "compress type, 0 for no compress, 1 for mpls, 2 for ideal, 3 for rohc", compress_version);
	cmd.AddValue("threshold", "Threshold, by default 100", threshold);
	cmd.AddValue("label_size", "Label size, by default 16384", label_size);
	cmd.AddValue("vxlan", "VxLAN, by default 0", vxlan_version);
	cmd.AddValue("transport_version", "0 for tcp, 1 for rdma", transport_version);
    
    cmd.Parse(argc, argv);
	
	std::cout << "Run Experiment for ";
	if(ip_version == 0) 
		std::cout << "Ipv4." << std::endl;
	else if(ip_version == 1) 
		std::cout << "Ipv6." << std::endl;

	file_name = "logs/" + flow_file + "s_IP" + std::to_string(ip_version) + \
					"_Compress" + std::to_string(compress_version) + \
					"_RDMA" + std::to_string(transport_version) + \
					"_Thres" + std::to_string(threshold) + \
					"_Label" + std::to_string(label_size);
	
	if(vxlan_version)
		file_name += "_vx"; 

	SetVariables();
	std::cout << "Set Variables" << std::endl;
	BuildFatTree();
	std::cout << "Build Topology" << std::endl;

	FlowScheduler scheduler(flow_file, file_name);
	StartSinkApp(&scheduler);
	std::cout << "Start Application" << std::endl;

	auto start = std::chrono::system_clock::now();

	scheduler.Schedule();

	Simulator::Stop(Seconds(start_time + duration + 5));
	Simulator::Run();
	Simulator::Destroy();

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end - start;
	std::cout << "Used time: " << diff.count() << "s." << std::endl;
}