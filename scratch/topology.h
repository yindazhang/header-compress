#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-static-routing-helper.h"

#include "my-config.h"

using namespace ns3;

std::vector<Ptr<Node>> servers;
std::vector<Ipv4Address> server_v4addr;
std::vector<Ipv6Address> server_v6addr;

std::vector<Ptr<NICNode>> nics;
std::vector<Ipv4Address> nic_v4addr;
std::vector<Ipv6Address> nic_v6addr;

// Fat-tree
std::vector<Ptr<SwitchNode>> edges;
std::vector<Ptr<SwitchNode>> aggs;
std::vector<Ptr<SwitchNode>> cores;

void BuildDCTCP(){
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpDctcp"));
	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
	Config::SetDefault("ns3::TcpSocket::ConnTimeout", TimeValue(MicroSeconds(2000)));
	Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(MicroSeconds(200)));
	Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MicroSeconds(800)));
	Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(MicroSeconds(10)));
	GlobalValue::Bind("ChecksumEnabled", BooleanValue(false));
}

void BuildFatTreeRoute(
	uint32_t K, 
    uint32_t NUM_BLOCK ,
	uint32_t RATIO){

	Ipv4StaticRoutingHelper ipv4_helper;
	Ipv6StaticRoutingHelper ipv6_helper;

	std::cout << "Start Fat Tree Routing" << std::endl;

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
			}
			else{
				nics[i]->AddHostRouteTo(server_v4addr[j], 2);
				nics[i]->AddHostRouteTo(server_v6addr[j], 2);
			}
		}
		Ptr<Icmpv6L4Protocol> icmpv6 = nics[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}

	for(uint32_t i = 0;i < K * K;++i){
		for(uint32_t j = 0;j< servers.size();++j){
			uint32_t rack_id = j / K / K / RATIO;
			cores[i]->AddHostRouteTo(server_v4addr[j], rack_id + 1);
			cores[i]->AddHostRouteTo(server_v6addr[j], rack_id + 1);
		}
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
				}
			}
			else{
				aggs[i]->AddHostRouteTo(server_v4addr[j], (j / K / RATIO) % K + 1);
				aggs[i]->AddHostRouteTo(server_v6addr[j], (j / K / RATIO) % K + 1);
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
				}
			}
			else{
				edges[i]->AddHostRouteTo(server_v4addr[j], j % (K * RATIO) + 1);
				edges[i]->AddHostRouteTo(server_v6addr[j], j % (K * RATIO) + 1);
			}
		}
		Ptr<Icmpv6L4Protocol> icmpv6 = edges[i]->GetObject<Icmpv6L4Protocol>();
		icmpv6->SetAttribute("DAD", BooleanValue(false));
	}
}

void BuildFatTree(
    uint32_t K = 3, 
    uint32_t NUM_BLOCK = 6,
	uint32_t RATIO = 4){

	uint32_t number_server = K * K * NUM_BLOCK * RATIO;

	servers.resize(number_server);
	server_v4addr.resize(number_server);
	server_v6addr.resize(number_server);

	nics.resize(number_server);
	nic_v4addr.resize(number_server);
	nic_v6addr.resize(number_server);
	
	edges.resize(K * NUM_BLOCK);
	aggs.resize(K * NUM_BLOCK);
	cores.resize(K * K);

	for(uint32_t i = 0;i < number_server;++i){
		servers[i] = CreateObject<Node>();
		nics[i] = CreateObject<NICNode>();
		nics[i]->SetID(i);
	}
	
	for(uint32_t i = 0;i < K * NUM_BLOCK;++i){
		edges[i] = CreateObject<SwitchNode>(); 
		edges[i]->SetECMPHash(1);
		edges[i]->SetID(100 + i);
	}

	for(uint32_t i = 0;i < K * NUM_BLOCK;++i){
		aggs[i] = CreateObject<SwitchNode>();
		aggs[i]->SetECMPHash(2);
		aggs[i]->SetID(200 + i);
	}

	for(uint32_t i = 0;i < K * K;++i){
		cores[i] = CreateObject<SwitchNode>();
		cores[i]->SetECMPHash(3);
		cores[i]->SetID(300 + i);
	}
    
	InternetStackHelper internet;
    internet.InstallAll();

	// Initilize link
	PointToPointHelper pp_server_nic;
	pp_server_nic.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
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

			NetDeviceContainer ndc = pp_nic_switch.Install(nics[nic_id], edges[i]);

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

			nic_v4addr[nic_id] = ic4.GetAddress(0, 0);
			nic_v6addr[nic_id] = ic6.GetAddress(0, 0);
		}
	}
	
	for(uint32_t i = 0;i < NUM_BLOCK;++i){
		for(uint32_t j = 0;j < K;++j){
			Ipv4InterfaceContainer ic4;
			Ipv6InterfaceContainer ic6;

			for(uint32_t k = 0;k < K;++k){
				NetDeviceContainer ndc = pp_switch_switch.Install(edges[i*K+j], aggs[i*K+k]);

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

	for(uint32_t i = 0;i < number_server;++i){
		Ipv4InterfaceContainer ic4;
		Ipv6InterfaceContainer ic6;

		NetDeviceContainer ndc = pp_server_nic.Install(servers[i], nics[i]);
		tch.Install(ndc);

		std::string ipv4_base = std::to_string(3*NUM_BLOCK) + "." + std::to_string(i / (K * RATIO)) + 
									"." + std::to_string(i % (K * RATIO)) + ".0";
		ipv4.SetBase(ipv4_base.c_str(), "255.255.255.0");
		ic4 = ipv4.Assign(ndc);

		std::string ipv6_base = std::to_string(3*NUM_BLOCK) + ":" + std::to_string(i / (K * RATIO)) + 
									":" + std::to_string(i % (K * RATIO)) + "::";
		ipv6.SetBase(ipv6_base.c_str(), Ipv6Prefix(64));
		ic6 = ipv6.Assign(ndc);

		ic6.SetForwarding(0, true);
		ic6.SetForwarding(1, true);

		Ptr<Ipv6> ip6 = servers[i]->GetObject<Ipv6>();

		server_v4addr[i] = ic4.GetAddress(0, 0);
		server_v6addr[i] = ic6.GetAddress(0, 1);

		/*
		print_v6addr(Ipv6Address(ipv6_base.c_str()));
		print_v6addr(ic6.GetAddress(0, 0));
		print_v6addr(ic6.GetAddress(0, 1));
		print_v6addr(ic6.GetAddress(1, 0));
		print_v6addr(ic6.GetAddress(1, 1));
		*/
	}

	BuildFatTreeRoute(K, NUM_BLOCK, RATIO);
}

void StartSinkApp(){
	for(uint32_t i = 0;i < servers.size();++i){
		PacketSinkHelper sink("ns3::TcpSocketFactory",
                         InetSocketAddress(Ipv4Address::GetAny(), DEFAULT_PORT));
  		ApplicationContainer sinkApps = sink.Install(servers[i]);
		sinkApps.Start(Seconds(start_time - 1));
  		sinkApps.Stop(Seconds(start_time + duration + 4));
	}
	for(uint32_t i = 0;i < servers.size();++i){
		PacketSinkHelper sink("ns3::TcpSocketFactory",
                         Inet6SocketAddress(Ipv6Address::GetAny(), DEFAULT_PORT));
  		ApplicationContainer sinkApps = sink.Install(servers[i]);
		sinkApps.Start(Seconds(start_time - 1));
  		sinkApps.Stop(Seconds(start_time + duration + 4));
	}
}

#endif 