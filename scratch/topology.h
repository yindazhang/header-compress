#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-static-routing-helper.h"

#include "my-config.h"

#define CONTROL_ID 0xffff

using namespace ns3;

std::vector<Ptr<Node>> servers;
std::vector<Ptr<ControlNode>> controllers;
std::vector<Ipv4Address> server_v4addr;
std::vector<Ipv6Address> server_v6addr;

// Fat-tree
std::vector<Ptr<NICNode>> nics;
std::vector<Ptr<SwitchNode>> edges;
std::vector<Ptr<SwitchNode>> aggs;
std::vector<Ptr<SwitchNode>> cores;

FILE* countFile = nullptr;
FILE* fctFile = nullptr;

std::unordered_map<uint32_t, FlowInfo> fctMp;

void SetVariables(){
	Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpDctcp"));

	Config::SetDefault("ns3::TcpSocket::ConnTimeout", TimeValue(MilliSeconds(1)));
	Config::SetDefault("ns3::TcpSocket::ConnCount", UintegerValue(6));  
	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
	Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
	Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(MicroSeconds(10)));

	Config::SetDefault("ns3::RttEstimator::InitialEstimation", TimeValue(MicroSeconds(100)));

	Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MicroSeconds(1000))); 
	Config::SetDefault("ns3::TcpSocketBase::MaxSegLifetime", DoubleValue(0.001)); 
	Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(NanoSeconds(10)));
	
	GlobalValue::Bind("ChecksumEnabled", BooleanValue(false));
}

void BuildFatTreeRoute(
	uint32_t K, 
    uint32_t NUM_BLOCK ,
	uint32_t RATIO){

	Ipv4StaticRoutingHelper ipv4_helper;
	Ipv6StaticRoutingHelper ipv6_helper;

	for(uint32_t i = 0;i < servers.size();++i){
		std::string ipv4_base = std::to_string(3*NUM_BLOCK) + ".0.0.0";
		Ptr<Ipv4> ipv4_server = servers[i]->GetObject<Ipv4>();
		Ptr<Ipv4StaticRouting> ipv4_routing = ipv4_helper.GetStaticRouting(ipv4_server);
		ipv4_routing->AddNetworkRouteTo(Ipv4Address(ipv4_base.c_str()), Ipv4Mask("255.0.0.0"), 1);

		std::string ipv6_base = std::to_string(3*NUM_BLOCK) + "::";
		Ptr<Ipv6> ipv6_server = servers[i]->GetObject<Ipv6>();
		Ptr<Ipv6StaticRouting> ipv6_routing = ipv6_helper.GetStaticRouting(ipv6_server);
		ipv6_routing->AddNetworkRouteTo(Ipv6Address(ipv6_base.c_str()), Ipv6Prefix(16), 1);
		
		Ptr<Icmpv6L4Protocol> icmpv6 = servers[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}

	for(uint32_t i = 0;i < servers.size();++i){
		for(uint32_t j = 0;j < servers.size();++j){
			if(i != j){
				nics[i]->AddHostRouteTo(server_v4addr[j], 1);
				nics[i]->AddHostRouteTo(server_v6addr[j], 1);
				nics[i]->AddControlRouteTo(1000 + j, 1);
			}
			else{
				nics[i]->AddHostRouteTo(server_v4addr[j], 2);
				nics[i]->AddHostRouteTo(server_v6addr[j], 2);
			}
		}

		for(uint32_t j = 0;j < edges.size();++j)
			nics[i]->AddControlRouteTo(2000 + j, 1);
		
		for(uint32_t j = 0;j < aggs.size();++j)
			nics[i]->AddControlRouteTo(3000 + j, 1);
	
		for(uint32_t j = 0;j < cores.size();++j)
			nics[i]->AddControlRouteTo(4000 + j, 1);

		nics[i]->AddControlRouteTo(CONTROL_ID, 1);

		Ptr<Icmpv6L4Protocol> icmpv6 = nics[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}

	for(uint32_t i = 0;i < K * K;++i){
		for(uint32_t j = 0;j < servers.size();++j){
			uint32_t rack_id = j / K / K / RATIO;
			cores[i]->AddHostRouteTo(server_v4addr[j], rack_id + 1);
			cores[i]->AddHostRouteTo(server_v6addr[j], rack_id + 1);
			cores[i]->AddControlRouteTo(1000 + j, rack_id + 1);
		}

		for(uint32_t j = 0;j < edges.size();++j)
			cores[i]->AddControlRouteTo(2000 + j, j / K + 1);
		
		for(uint32_t j = 0;j < aggs.size();++j){
			if(j % K == i / K)
				cores[i]->AddControlRouteTo(3000 + j, j / K + 1);
		}

		cores[i]->AddControlRouteTo(CONTROL_ID, NUM_BLOCK);

		Ptr<Icmpv6L4Protocol> icmpv6 = cores[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}

	for(uint32_t i = 0;i < NUM_BLOCK * K;++i){
		for(uint32_t j = 0;j < servers.size();++j){
			uint32_t rack_id = j / K / K / RATIO;
			if(rack_id != i / K){
				for(uint32_t coreId = 1;coreId <= K;++coreId){
					aggs[i]->AddHostRouteTo(server_v4addr[j], K + coreId);
					aggs[i]->AddHostRouteTo(server_v6addr[j], K + coreId);
					aggs[i]->AddControlRouteTo(1000 + j, K + coreId);
				}
			}
			else{
				aggs[i]->AddHostRouteTo(server_v4addr[j], (j / K / RATIO) % K + 1);
				aggs[i]->AddHostRouteTo(server_v6addr[j], (j / K / RATIO) % K + 1);
				aggs[i]->AddControlRouteTo(1000 + j, (j / K / RATIO) % K + 1);
			}
		}

		for(uint32_t j = 0;j < edges.size();++j){
			if(j / K != i / K){
				for(uint32_t coreId = 1;coreId <= K;++coreId){
					aggs[i]->AddControlRouteTo(2000 + j, K + coreId);
				}
			}
			else{
				aggs[i]->AddControlRouteTo(2000 + j, j % K + 1);
			}
		}
		
		for(uint32_t j = 0;j < aggs.size();++j){
			if(j != i && j % K == i % K){
				for(uint32_t coreId = 1;coreId <= K;++coreId){
					aggs[i]->AddControlRouteTo(3000 + j, K + coreId);
				}
			}
		}
	
		for(uint32_t j = 0;j < cores.size();++j){
			if(i % K == j / K){
				aggs[i]->AddControlRouteTo(4000 + j, K + (j % K) + 1);
			}
		}

		if(i / K == NUM_BLOCK - 1){
			aggs[i]->AddControlRouteTo(CONTROL_ID, K);
		}
		else{
			for(uint32_t coreId = 1;coreId <= K;++coreId){
				aggs[i]->AddControlRouteTo(CONTROL_ID, K + coreId);
			}
		}

		Ptr<Icmpv6L4Protocol> icmpv6 = aggs[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}

	for(uint32_t i = 0;i < NUM_BLOCK * K;++i){
		for(uint32_t j = 0;j < servers.size();++j){
			uint32_t rack_id = j / K / RATIO;
			if(rack_id != i){
				for(uint32_t aggId = 1;aggId <= K;++aggId){
					edges[i]->AddHostRouteTo(server_v4addr[j], K * RATIO + aggId);
					edges[i]->AddHostRouteTo(server_v6addr[j], K * RATIO + aggId);
					edges[i]->AddControlRouteTo(1000 + j, K * RATIO + aggId);
				}
			}
			else{
				edges[i]->AddHostRouteTo(server_v4addr[j], j % (K * RATIO) + 1);
				edges[i]->AddHostRouteTo(server_v6addr[j], j % (K * RATIO) + 1);
				edges[i]->AddControlRouteTo(1000 + j, j % (K * RATIO) + 1);
			}
		}

		for(uint32_t j = 0;j < edges.size();++j){
			if(i != j){
				for(uint32_t aggId = 1;aggId <= K;++aggId){
					edges[i]->AddControlRouteTo(2000 + j, K * RATIO + aggId);
				}
			}
		}
		
		for(uint32_t j = 0;j < aggs.size();++j){
			edges[i]->AddControlRouteTo(3000 + j, K * RATIO + (j % K) + 1);
		}
	
		for(uint32_t j = 0;j < cores.size();++j){
			edges[i]->AddControlRouteTo(4000 + j, K * RATIO + (j / K) + 1);
		}

		if(i == NUM_BLOCK * K - 1){
			edges[i]->AddControlRouteTo(CONTROL_ID, K * RATIO);
		}
		else{
			for(uint32_t aggId = 1;aggId <= K;++aggId){
				edges[i]->AddControlRouteTo(CONTROL_ID, K * RATIO + aggId);
			}
		}

		Ptr<Icmpv6L4Protocol> icmpv6 = edges[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}

	Ptr<Icmpv6L4Protocol> icmpv6 = controllers[0]->GetObject<Icmpv6L4Protocol>();
	icmpv6->SetAttribute("DAD", BooleanValue(false));
}

void BuildFatTree(
    uint32_t K = 3, 
    uint32_t NUM_BLOCK = 6,
	uint32_t RATIO = 4){

	uint32_t number_server = K * K * NUM_BLOCK * RATIO;
	uint32_t number_control = 1;

	servers.resize(number_server - number_control);
	server_v4addr.resize(number_server - number_control);
	server_v6addr.resize(number_server - number_control);
	controllers.resize(number_control);

	nics.resize(number_server - number_control);
	
	edges.resize(K * NUM_BLOCK);
	aggs.resize(K * NUM_BLOCK);
	cores.resize(K * K);

	for(uint32_t i = 0;i < number_server - number_control;++i){
		servers[i] = CreateObject<Node>();
		nics[i] = CreateObject<NICNode>();
		nics[i]->SetID(1000 + i);
		nics[i]->SetThreshold(threshold);
		nics[i]->SetOutput(file_name);
		nics[i]->SetSetting(mpls_version);
		nics[i]->SetVxLAN(vxlan_version);
	}
	for(uint32_t i = 0;i < number_control;++i){
		controllers[i] = CreateObject<ControlNode>();
		controllers[i]->SetID(CONTROL_ID);
		controllers[i]->SetOutput(file_name);
		controllers[i]->SetLabelSize(label_size);
	}
	
	for(uint32_t i = 0;i < K * NUM_BLOCK;++i){
		edges[i] = CreateObject<SwitchNode>(); 
		edges[i]->SetECMPHash(1);
		edges[i]->SetID(2000 + i);
		edges[i]->SetOutput(file_name);
		edges[i]->SetSetting(mpls_version);
		edges[i]->SetPFC(transport_version);
	}

	for(uint32_t i = 0;i < K * NUM_BLOCK;++i){
		aggs[i] = CreateObject<SwitchNode>();
		aggs[i]->SetECMPHash(2);
		aggs[i]->SetID(3000 + i);
		aggs[i]->SetOutput(file_name);
		aggs[i]->SetSetting(mpls_version);
		aggs[i]->SetPFC(transport_version);
	}

	for(uint32_t i = 0;i < K * K;++i){
		cores[i] = CreateObject<SwitchNode>();
		cores[i]->SetECMPHash(3);
		cores[i]->SetID(4000 + i);
		cores[i]->SetOutput(file_name);
		cores[i]->SetSetting(mpls_version);
		cores[i]->SetPFC(transport_version);
	}
    
	for(uint32_t i = 0;i < number_control;++i){
		controllers[i]->SetTopology(K, NUM_BLOCK, RATIO, nics, edges, aggs, cores);
	}

	InternetStackHelper internet;
    internet.InstallAll();

	// Initilize link
	PointToPointHelper pp_server_nic;
	pp_server_nic.SetDeviceAttribute("DataRate", StringValue("128Gbps"));
	pp_server_nic.SetChannelAttribute("Delay", StringValue("100ns"));

	PointToPointHelper pp_nic_switch;
	pp_nic_switch.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
	pp_nic_switch.SetChannelAttribute("Delay", StringValue("1us"));

	PointToPointHelper pp_switch_switch;
	pp_switch_switch.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
	pp_switch_switch.SetChannelAttribute("Delay", StringValue("1us"));


	TrafficControlHelper tch;
	tch.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("16MiB")));

	Ipv4AddressHelper ipv4;
	Ipv6AddressHelper ipv6;

	for(uint32_t i = 0;i < K * NUM_BLOCK;++i){
		for(uint32_t j = 0;j < K * RATIO;++j){
			uint32_t nic_id = i * K * RATIO + j;

			Ipv4InterfaceContainer ic4;
			Ipv6InterfaceContainer ic6;

			NetDeviceContainer ndc;
			
			if(nic_id < number_server - number_control){
				ndc = pp_nic_switch.Install(nics[nic_id], edges[i]);
				nics[nic_id]->SetNextNode(1, edges[i]->GetID());
				edges[i]->SetNextNode(j + 1, nics[nic_id]->GetID());
			}
			else
				ndc = pp_nic_switch.Install(controllers[0], edges[i]);

			std::string ipv4_base = std::to_string(i / K) + "." + std::to_string(i % K) + 
									"." + std::to_string(j) + ".0";
			ipv4.SetBase(ipv4_base.c_str(), "255.255.255.0");
			ic4 = ipv4.Assign(ndc);

			std::string ipv6_base = std::to_string(i / K) + ":" + std::to_string(i % K) + 
									":" + std::to_string(j) + "::";
			ipv6.SetBase(ipv6_base.c_str(), Ipv6Prefix(64));
			ic6 = ipv6.Assign(ndc);

			ic6.SetForwarding(0, true);
			ic6.SetForwarding(1, true);
		}
	}
	
	for(uint32_t i = 0;i < NUM_BLOCK;++i){
		for(uint32_t j = 0;j < K;++j){
			Ipv4InterfaceContainer ic4;
			Ipv6InterfaceContainer ic6;

			for(uint32_t k = 0;k < K;++k){
				NetDeviceContainer ndc = pp_switch_switch.Install(edges[i*K+j], aggs[i*K+k]);
				edges[i*K+j]->SetNextNode(K * RATIO + k + 1, aggs[i*K+k]->GetID());
				aggs[i*K+k]->SetNextNode(j + 1, edges[i*K+j]->GetID());

				std::string ipv4_base = std::to_string(i + NUM_BLOCK) + "." + std::to_string(j) + 
									"." + std::to_string(k) + ".0";
				ipv4.SetBase(ipv4_base.c_str(), "255.255.255.0");
				ic4 = ipv4.Assign(ndc);

				std::string ipv6_base = std::to_string(i + NUM_BLOCK) + ":" + std::to_string(j) + 
									":" + std::to_string(k) + "::";
				ipv6.SetBase(ipv6_base.c_str(), Ipv6Prefix(64));
				ic6 = ipv6.Assign(ndc);

				ic6.SetForwarding(0, true);
				ic6.SetForwarding(1, true);
			}
		}
	}

	for(uint32_t i = 0;i < NUM_BLOCK;++i){
		for(uint32_t j = 0;j < K;++j){
			Ipv4InterfaceContainer ic4;
			Ipv6InterfaceContainer ic6;

			for(uint32_t k = 0;k < K;++k){
				NetDeviceContainer ndc = pp_switch_switch.Install(aggs[i*K+j], cores[j*K+k]);

				aggs[i*K+j]->SetNextNode(K + k + 1, cores[j*K+k]->GetID());
				cores[j*K+k]->SetNextNode(i + 1, aggs[i*K+j]->GetID());

				std::string ipv4_base = std::to_string(i + 2*NUM_BLOCK) + "." + std::to_string(j) + 
									"." + std::to_string(k) + ".0";
				ipv4.SetBase(ipv4_base.c_str(), "255.255.255.0");
				ic4 = ipv4.Assign(ndc);

				std::string ipv6_base = std::to_string(i + 2*NUM_BLOCK) + ":" + std::to_string(j) + 
									":" + std::to_string(k) + "::";
				ipv6.SetBase(ipv6_base.c_str(), Ipv6Prefix(64));
				ic6 = ipv6.Assign(ndc);

				ic6.SetForwarding(0, true);
				ic6.SetForwarding(1, true);
			}
		}
	}

	for(uint32_t i = 0;i < number_server - number_control;++i){
		Ipv4InterfaceContainer ic4;
		Ipv6InterfaceContainer ic6;

		NetDeviceContainer ndc = pp_server_nic.Install(servers[i], nics[i]);
		tch.Install(ndc);

		std::string ipv4_base = std::to_string(3*NUM_BLOCK) + "." + std::to_string(i / (K * RATIO)) + 
									"." + std::to_string(i % (K * RATIO)) + ".0";
		ipv4.SetBase(ipv4_base.c_str(), "255.255.255.0");
		ic4 = ipv4.Assign(ndc);

		std::stringstream astream, bstream;
		astream << std::hex << i / (K * RATIO);
		bstream << std::hex << i % (K * RATIO);

		// std::cout << astream.str() << " " << bstream.str() << std::endl;

		std::string ipv6_base = std::to_string(3*NUM_BLOCK) + ":" + astream.str() + 
									":" + bstream.str() + "::";
		ipv6.SetBase(ipv6_base.c_str(), Ipv6Prefix(64));
		ic6 = ipv6.Assign(ndc);

		ic6.SetForwarding(0, true);
		ic6.SetForwarding(1, true);

		Ptr<Ipv6> ip6 = servers[i]->GetObject<Ipv6>();

		server_v4addr[i] = ic4.GetAddress(0, 0);
		server_v6addr[i] = ic6.GetAddress(0, 1);
	}

	std::cout << "Start Route" << std::endl;
	BuildFatTreeRoute(K, NUM_BLOCK, RATIO);
	std::cout << "Finish Route" << std::endl;
}

void CountPacket(){
	uint64_t userCount = 0;
	uint64_t mplsCount = 0;

	for(auto nic : nics){
		userCount += nic->GetUserCount();
		mplsCount += nic->GetMplsCount();
		nic->SetUserCount(0);
		nic->SetMplsCount(0);
	}

	if(userCount != 0 || mplsCount != 0){
		fprintf(countFile, "%ld,%lu,%lu\n", Simulator::Now().GetMilliSeconds(), userCount, mplsCount);
		fflush(countFile);
	}

	if(Simulator::Now().GetMilliSeconds() > 4000){
		fclose(countFile);
		return;
	}

	Simulator::Schedule(NanoSeconds(1000000), CountPacket);
}

void WriteFCT(uint32_t index, uint32_t endTime){
	if(fctMp[index].end == 0){
		fctMp[index].end = endTime;
		fprintf(fctFile, "%u,%u,%u,%u,%u,%u,%u\n",
			fctMp[index].index, fctMp[index].src, fctMp[index].dst,
			fctMp[index].size, fctMp[index].start, fctMp[index].end,
			fctMp[index].end - fctMp[index].start
		);
		fflush(fctFile);
	}
}

void StartSocket(FlowScheduler* scheduler){
	auto sockets = new std::map<std::pair<uint32_t, uint32_t>, std::vector<Ptr<SocketInfo>>>();
	double delay = 0.000001;

	for(uint16_t port = 10000; port < 10000 + NUM_SOCKET;++port){
		// std::cout << "Allocate sender port " << port << std::endl;
		for(uint32_t src = 0;src < servers.size();++src){
			for(uint32_t dst = 0;dst < servers.size();++dst){
				if(src == dst)
					continue;
				
				auto conn = std::make_pair(src, dst);

				if(ip_version == 0)
					(*sockets)[conn].push_back(Create<SocketInfo>(servers[src], port, InetSocketAddress(server_v4addr[dst], DEFAULT_PORT)));
				else
					(*sockets)[conn].push_back(Create<SocketInfo>(servers[src], port, Inet6SocketAddress(server_v6addr[dst], DEFAULT_PORT)));
				(*sockets)[conn].back()->Connect(delay);
				delay += 0.000001;
			}
		}
	}

	scheduler->SetSockets(sockets);
}

void StartSinkApp(FlowScheduler* scheduler){
	countFile = fopen((file_name + ".count").c_str(), "w");

	Simulator::Schedule(Seconds(start_time + 0.001), CountPacket);

	for(uint32_t i = 0;i < servers.size();++i){
		ApplicationContainer sinkApps;
		if(ip_version == 0){
			PacketSinkHelper sink("ns3::TcpSocketFactory",
							InetSocketAddress(Ipv4Address::GetAny(), DEFAULT_PORT));
			sinkApps = sink.Install(servers[i]);
		}
		else if(ip_version == 1){
			PacketSinkHelper sink("ns3::TcpSocketFactory",
							Inet6SocketAddress(Ipv6Address::GetAny(), DEFAULT_PORT));
			sinkApps = sink.Install(servers[i]);
		}
		DynamicCast<PacketSink>(sinkApps.Get(0))->SetWriteFCT(WriteFCT);
		sinkApps.Start(Seconds(start_time - 1));
		sinkApps.Stop(Seconds(start_time + duration + 4));
	}

	Simulator::Schedule(Seconds(start_time - 0.8), StartSocket, scheduler);
}

#endif 
