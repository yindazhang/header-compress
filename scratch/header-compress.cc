#include "ns3/core-module.h"

#include "schedule.h"

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
	cmd.AddValue("mpls_version", "add mpls or not", mpls_version);
	cmd.AddValue("threshold", "threshold for large flows, by default 1000", threshold);
    
    cmd.Parse(argc, argv);
	
	std::cout << "Run Experiment for ";
	if(ip_version == 0) 
		std::cout << "Ipv4." << std::endl;
	else if(ip_version == 1) 
		std::cout << "Ipv6." << std::endl;

	file_name = "logs/" + flow_file + "s_IP" + std::to_string(ip_version) + \
					"_MPLS" + std::to_string(mpls_version) + \
					"_Thres" + std::to_string(threshold);

	BuildDCTCP();
	std::cout << "Set DCTCP" << std::endl;
	BuildFatTree();
	std::cout << "Build Topology" << std::endl;
	StartSinkApp();
	std::cout << "Start Application" << std::endl;

	auto start = std::chrono::system_clock::now();

	ScheduleFlow(flow_file);

	Simulator::Stop(Seconds(start_time + duration + 5));
	Simulator::Run();
	Simulator::Destroy();

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end - start;
	std::cout << "Used time: " << diff.count() << "s." << std::endl;

	flow_input.close();
	fclose(fct_output);
}