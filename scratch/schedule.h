#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "topology.h"

FILE *fct_output;
std::ifstream flow_input;

struct Flows{
    uint32_t index, total_size;
    uint32_t src, dst, bytes, start_time;
};
Flows flow = {0};

void RecordFCT(FILE* fout, uint64_t app_id, int64_t size, int64_t fct, int64_t end_time){
	fprintf(fout, "%ld %ld %ld %ld\n", app_id, size, fct, end_time);
    fflush(fout);
}

void ReadFlowInput(){
	if(flow.index < flow.total_size)
		flow_input >> flow.src >> flow.dst >> flow.bytes >> flow.start_time;
}

void ScheduleFlowInputs(){
	while(flow.index < flow.total_size && NanoSeconds(flow.start_time) == Simulator::Now()){
		// std::cout << "Schedule Flow " << flow.index  << std::endl;
		if(ip_version == 0){
			BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress(server_v4addr[flow.dst], DEFAULT_PORT));
			source.SetAttribute("MaxBytes", UintegerValue(flow.bytes));
			source.SetAttribute("ApplicationID", UintegerValue(flow.index));

			ApplicationContainer sourceApps = source.Install(servers[flow.src]);
			sourceApps.Get(0)->TraceConnectWithoutContext("BulkEnd", MakeBoundCallback(RecordFCT, fct_output));
			sourceApps.Start(Time(0));
		}
		else if(ip_version == 1){
			BulkSendHelper source ("ns3::TcpSocketFactory",
                         Inet6SocketAddress(server_v6addr[flow.dst], DEFAULT_PORT));
			source.SetAttribute("MaxBytes", UintegerValue(flow.bytes));
			source.SetAttribute("ApplicationID", UintegerValue(flow.index));

			ApplicationContainer sourceApps = source.Install(servers[flow.src]);
			sourceApps.Get(0)->TraceConnectWithoutContext("BulkEnd", MakeBoundCallback(RecordFCT, fct_output));
			sourceApps.Start(Time(0));
		}
		
		flow.index++;
		ReadFlowInput();
	}

	if (flow.index < flow.total_size)
		Simulator::Schedule(NanoSeconds(flow.start_time) - Simulator::Now(), ScheduleFlowInputs);
	else
		flow_input.close();
}

void ScheduleFlow(std::string flow_file){
	fct_output = fopen((file_name + ".fct").c_str(), "w");
    	
	flow_input.open("trace/" + flow_file + ".tr");
	if(!flow_input.is_open()){
		std::cout << "Cannot open flow file " << "trace/" + flow_file + ".tr" << std::endl;
		exit(1);
	}

	flow_input >> flow.total_size;
	std::cout << "Total flow number: " << flow.total_size << std::endl;

	if(flow.total_size > 0){
		ReadFlowInput();
		Simulator::Schedule(NanoSeconds(flow.start_time), ScheduleFlowInputs);
	}
}


#endif 