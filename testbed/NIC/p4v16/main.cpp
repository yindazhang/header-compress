#include "libs/mgr.h"

#include <dlfcn.h> 
#include <pipe_mgr/pipe_mgr_intf.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h> 
#include <sys/ioctl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/if_ether.h>

#include <arpa/inet.h>

#include <unordered_map>
#include <vector>
#include <queue>
#include <list>

#include <iostream>
#include <iomanip>
#include <chrono>

#define PKTGEN_SRC_PORT_PIPE0 68
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_VLAN 0x8100
#define UDP_TYPE 0x11

#define PD_DEV_PIPE_ALL 0xffff
#define PKTGEN_APP_1 0x1
#define PKTGEN_APP_2 0x1

#define MIN_PKTGEN_SIZE 54

#define DEV_ID 0

#define V4SIZE 80
#define V6SIZE 120

#define OFFSET 1024
#define MPLS_SIZE 16384
#define TIMEOUT 200000

bool compress = false;

pcap_t *handle;
u_char rsvp4_packet[V4SIZE] = {0};
u_char rsvp6_packet[V6SIZE] = {0};

p4_pd_sess_hdl_t p4_sess_hdl;
const bfrt::BfRtLearn *bfrtLearnDigest = nullptr;
bf_rt_id_t learn_srcPort, learn_dstPort, learn_srcAddr1, learn_srcAddr2, learn_srcAddr3, learn_srcAddr4, 
				learn_dstAddr1, learn_dstAddr2, learn_dstAddr3, learn_dstAddr4, learn_digest, learn_op;

bf_status_t bf_status;
bf_rt_target_t dev_tgt;
std::shared_ptr<bfrt::BfRtSession> session;
const bfrt::BfRtInfo *bfrtInfo = nullptr;

const bfrt::BfRtTable *compressTable, *decompressTable;
std::unique_ptr<bfrt::BfRtTableKey> compressKey, decompressKey;
std::unique_ptr<bfrt::BfRtTableData> compressData, decompressData;

bf_status_t digestCallback(const bf_rt_target_t &bf_rt_tgt,
                            const std::shared_ptr<bfrt::BfRtSession> bfrtsession,
                            std::vector<std::unique_ptr<bfrt::BfRtLearnData>> vec,
                            bf_rt_learn_msg_hdl *const learn_msg_hdl,
                            const void *cookie);

void init_nic();

struct NetTuple {
    uint32_t srcAddr1;
	uint32_t srcAddr2;
	uint32_t srcAddr3;
	uint32_t srcAddr4;
	uint32_t dstAddr1;
	uint32_t dstAddr2;
	uint32_t dstAddr3;
	uint32_t dstAddr4;
    uint16_t srcPort;
    uint16_t dstPort;

    NetTuple(){
		srcAddr1 = srcAddr2 = srcAddr3 = srcAddr4 = 0;
		dstAddr1 = dstAddr2 = dstAddr3 = dstAddr4 = 0;
		srcPort = dstPort = 0;
	};

	NetTuple(const NetTuple& tuple){
		srcAddr1 = tuple.srcAddr1;
		srcAddr2 = tuple.srcAddr2;
		srcAddr3 = tuple.srcAddr3;
		srcAddr4 = tuple.srcAddr4;
		dstAddr1 = tuple.dstAddr1;
		dstAddr2 = tuple.dstAddr2;
		dstAddr3 = tuple.dstAddr3;
		dstAddr4 = tuple.dstAddr4;
		srcPort = tuple.srcPort;
		dstPort = tuple.dstPort;
	};

    bool operator == (const NetTuple& other) const {
        return (srcAddr1 == other.srcAddr1 && srcAddr2 == other.srcAddr2 &&
                srcAddr3 == other.srcAddr3 && srcAddr4 == other.srcAddr4 &&
				dstAddr1 == other.dstAddr1 && dstAddr2 == other.dstAddr2 &&
                dstAddr3 == other.dstAddr3 && dstAddr4 == other.dstAddr4 &&
                srcPort == other.srcPort && dstPort == other.dstPort);
    }
};

static const uint32_t Prime[5] = {2654435761U,246822519U,3266489917U,668265263U,374761393U};

struct NetTupleHash {
	static inline uint32_t rotateLeft(uint32_t x, unsigned char bits) {
        return (x << bits) | (x >> (32 - bits));
    }

    std::size_t operator()(const NetTuple& tuple) const {
		uint8_t* data = (uint8_t*)(&tuple);

        uint32_t seed = 5197;
        uint32_t state[4] = {seed + Prime[0] + Prime[1], seed + Prime[1], seed, seed - Prime[0]};
        uint32_t result = 36 + state[2] + Prime[4];

        const uint8_t* stop = data + 36;

        for (; data + 4 <= stop; data += 4)
            result = rotateLeft(result + *(uint32_t*)data * Prime[2], 17) * Prime[3];

        result ^= result >> 15;
        result *= Prime[1];
        result ^= result >> 13;
        result *= Prime[2];
        result ^= result >> 16;
        return result;
    }
};

struct Entry {
	uint64_t state;
	uint64_t timestamp;
	NetTuple tuple;

	Entry(){
		state = timestamp = 0;
	};

	Entry(NetTuple _tuple, uint64_t _state = 0, uint64_t _timestamp = 0):
		tuple(_tuple), state(_state), timestamp(_timestamp){};
};

std::vector<Entry> table;
std::queue<Entry> delQue;
std::unordered_map<NetTuple, uint16_t, NetTupleHash> flowMap;
std::list<uint16_t> availableList;

int main(int argc, char **argv) {
	start_switchd(argc, argv);
	sleep(10);

	std::cout << "Init" << std::endl;

	table.resize(MPLS_SIZE);
	for(int i = 0;i < MPLS_SIZE;++i)
		availableList.push_back(i);

	init_nic();
	
	dev_tgt.dev_id = 0;
	dev_tgt.pipe_id = 0xffff;

	auto &devMgr = bfrt::BfRtDevMgr::getInstance();

	bf_status = devMgr.bfRtInfoGet(dev_tgt.dev_id, "main", &bfrtInfo);

	session = bfrt::BfRtSession::sessionCreate();

	bf_status = bfrtInfo->bfrtTableFromNameGet("pipe.Ingress.ti_compress_mpls", &compressTable);
	bf_status = bfrtInfo->bfrtTableFromNameGet("pipe.Ingress.ti_decompress_mpls", &decompressTable);

	bf_status = compressTable->keyAllocate(&compressKey);
    bf_status = compressTable->dataAllocate(&compressData);

	bf_status = decompressTable->keyAllocate(&decompressKey);
    bf_status = decompressTable->dataAllocate(&decompressData);

	bf_status = bfrtInfo->bfrtLearnFromNameGet("IngressDeparser.digest_flow", &bfrtLearnDigest);

	bf_status = bfrtLearnDigest->learnFieldIdGet("srcAddr1", &learn_srcAddr1);
	bf_status = bfrtLearnDigest->learnFieldIdGet("srcAddr2", &learn_srcAddr2);
	bf_status = bfrtLearnDigest->learnFieldIdGet("srcAddr3", &learn_srcAddr3);
	bf_status = bfrtLearnDigest->learnFieldIdGet("srcAddr4", &learn_srcAddr4);
	bf_status = bfrtLearnDigest->learnFieldIdGet("dstAddr1", &learn_dstAddr1);
	bf_status = bfrtLearnDigest->learnFieldIdGet("dstAddr2", &learn_dstAddr2);
	bf_status = bfrtLearnDigest->learnFieldIdGet("dstAddr3", &learn_dstAddr3);
	bf_status = bfrtLearnDigest->learnFieldIdGet("dstAddr4", &learn_dstAddr4);
	bf_status = bfrtLearnDigest->learnFieldIdGet("srcPort", &learn_srcPort);
	bf_status = bfrtLearnDigest->learnFieldIdGet("dstPort", &learn_dstPort);
	bf_status = bfrtLearnDigest->learnFieldIdGet("digest", &learn_digest);
	bf_status = bfrtLearnDigest->learnFieldIdGet("op", &learn_op);
	
	bf_status = bfrtLearnDigest->bfRtLearnCallbackRegister(session, dev_tgt, digestCallback, nullptr);

	std::cout << "Joining switchd in main.cpp" << std::endl;

	return join_switchd();
}

void init_nic(){
	const char *device = "enp6s0";  // The NIC to send the packet through
    char error_buffer[PCAP_ERRBUF_SIZE];

    // Open the NIC for packet injection
    handle = pcap_open_live(device, BUFSIZ, 1, 1000, error_buffer);
    if (handle == nullptr) {
        std::cerr << "Could not open device " << device << ": " << error_buffer << std::endl;
        exit(1);
    }

	struct ether_header *eth = (struct ether_header *)rsvp4_packet;
    memcpy(eth->ether_shost, "\x00\x02\x00\x00\x03\x00", 6);  // Source MAC
    memcpy(eth->ether_dhost, "\x66\x77\x88\x99\xaa\xbb", 6);  // Destination MAC
    eth->ether_type = htons(ETHERTYPE_IP);  // IPv4 protocol

	struct ip* iphdr = (struct ip*)(rsvp4_packet + sizeof(struct ether_header));
	iphdr->ip_v = 4;
    iphdr->ip_hl = 5;
    iphdr->ip_len = htons(V4SIZE - 14);
    iphdr->ip_ttl = 64;
    iphdr->ip_p = 46;

	u_char *rsvp_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip));
	rsvp_header[0] = 16;
	rsvp_header[4] = 64;
	rsvp_header[7] = 46;

	eth = (struct ether_header *)rsvp6_packet;
    memcpy(eth->ether_shost, "\x00\x02\x00\x00\x03\x00", 6);  // Source MAC
    memcpy(eth->ether_dhost, "\x66\x77\x88\x99\xaa\xbb", 6);  // Destination MAC
    eth->ether_type = htons(ETHERTYPE_IPV6);  // IPv4 protocol

	struct ip6_hdr* ip6hdr = (struct ip6_hdr*)(rsvp6_packet + sizeof(struct ether_header));
	ip6hdr->ip6_flow = htonl((6 << 28) | (0 << 20)); // Version (6) + Traffic Class (0)
    ip6hdr->ip6_plen = htons(V6SIZE - 14); // Payload length
    ip6hdr->ip6_nxt = 46; // Next header: UDP
    ip6hdr->ip6_hops = 64; // Hop limit

	rsvp_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
	rsvp_header[0] = 16;
	rsvp_header[4] = 64;
	rsvp_header[7] = 46;
}



bf_status_t digestCallback(const bf_rt_target_t &bf_rt_tgt,
                            const std::shared_ptr<bfrt::BfRtSession> bfrtsession,
                            std::vector<std::unique_ptr<bfrt::BfRtLearnData>> vec,
                            bf_rt_learn_msg_hdl *const learn_msg_hdl,
                            const void *cookie){
	uint64_t srcPort, dstPort;
	uint64_t srcAddr1, srcAddr2, srcAddr3, srcAddr4;
	uint64_t dstAddr1, dstAddr2, dstAddr3, dstAddr4;
	uint64_t digest, op;

	NetTuple tuple;

	auto now = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());

	for (uint32_t i = 0; i < vec.size(); i++){
		vec[i].get()->getValue(learn_srcAddr1, &srcAddr1);
		vec[i].get()->getValue(learn_srcAddr2, &srcAddr2);
		vec[i].get()->getValue(learn_srcAddr3, &srcAddr3);
		vec[i].get()->getValue(learn_srcAddr4, &srcAddr4);
		vec[i].get()->getValue(learn_dstAddr1, &dstAddr1);
		vec[i].get()->getValue(learn_dstAddr2, &dstAddr2);
		vec[i].get()->getValue(learn_dstAddr3, &dstAddr3);
		vec[i].get()->getValue(learn_dstAddr4, &dstAddr4);
		vec[i].get()->getValue(learn_srcPort, &srcPort);
		vec[i].get()->getValue(learn_dstPort, &dstPort);
		vec[i].get()->getValue(learn_digest, &digest);
		vec[i].get()->getValue(learn_op, &op);


		tuple.srcAddr1 = srcAddr1;
		tuple.srcAddr2 = srcAddr2;
		tuple.srcAddr3 = srcAddr3;
		tuple.srcAddr4 = srcAddr4;
		tuple.dstAddr1 = dstAddr1;
		tuple.dstAddr2 = dstAddr2;
		tuple.dstAddr3 = dstAddr3;
		tuple.dstAddr4 = dstAddr4;
		tuple.srcPort = srcPort;
		tuple.dstPort = dstPort;
	
		if(op == 14){
			if(!compress) continue;

			if(flowMap.find(tuple) != flowMap.end()){
				continue;
			}
			if(availableList.size() == 0){
				std::cout << "No available space" << std::endl;
				continue;
			}

			uint16_t label = availableList.front();
			availableList.pop_front();

			if(table[label].state != 0){
				std::cout << "State of Table " << label << " is not 0" << std::endl;
				continue;
			}

			if(dstAddr4 == 0){
				struct ip* iphdr = (struct ip*)(rsvp4_packet + sizeof(struct ether_header));
				iphdr->ip_src.s_addr = htonl(srcAddr1);
				iphdr->ip_dst.s_addr = htonl(dstAddr1);

				u_char *rsvp_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip));
				rsvp_header[1] = 1;

				u_char *object_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip) + 8);
				*(uint16_t*)object_header = htons(srcPort);
				*(uint16_t*)(object_header + 2) = htons(dstPort);
				*(uint32_t*)(object_header + 4) = htonl(srcAddr1);
				*(uint32_t*)(object_header + 8) = 0;
				*(uint32_t*)(object_header + 12) = 0;
				*(uint32_t*)(object_header + 16) = 0;
				*(uint32_t*)(object_header + 20) = htonl(dstAddr1);
				*(uint32_t*)(object_header + 24) = 0;
				*(uint32_t*)(object_header + 28) = 0;
				*(uint32_t*)(object_header + 32) = 0;
				*(uint16_t*)(object_header + 36) = htons(label + OFFSET);

				if (pcap_sendpacket(handle, rsvp4_packet, V4SIZE) != 0)
					std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
			}
			else{
				struct ip6_hdr* ip6hdr = (struct ip6_hdr*)(rsvp6_packet + sizeof(struct ether_header));
				ip6hdr->ip6_src.s6_addr32[0] = htonl(srcAddr1);
				ip6hdr->ip6_src.s6_addr32[1] = htonl(srcAddr2);
				ip6hdr->ip6_src.s6_addr32[2] = htonl(srcAddr3);
				ip6hdr->ip6_src.s6_addr32[3] = htonl(srcAddr4);
				ip6hdr->ip6_dst.s6_addr32[0] = htonl(dstAddr1);
				ip6hdr->ip6_dst.s6_addr32[1] = htonl(dstAddr2);
				ip6hdr->ip6_dst.s6_addr32[2] = htonl(dstAddr3);
				ip6hdr->ip6_dst.s6_addr32[3] = htonl(dstAddr4);

				u_char *rsvp_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
				rsvp_header[1] = 1;

				u_char *object_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr) + 8);
				*(uint16_t*)object_header = htons(srcPort);
				*(uint16_t*)(object_header + 2) = htons(dstPort);
				*(uint32_t*)(object_header + 4) = htonl(srcAddr1);
				*(uint32_t*)(object_header + 8) = htonl(srcAddr2);
				*(uint32_t*)(object_header + 12) = htonl(srcAddr3);
				*(uint32_t*)(object_header + 16) = htonl(srcAddr4);
				*(uint32_t*)(object_header + 20) = htonl(dstAddr1);
				*(uint32_t*)(object_header + 24) = htonl(dstAddr2);
				*(uint32_t*)(object_header + 28) = htonl(dstAddr3);
				*(uint32_t*)(object_header + 32) = htonl(dstAddr4);
				*(uint16_t*)(object_header + 36) = htons(label + OFFSET);


				if (pcap_sendpacket(handle, rsvp6_packet, V6SIZE) != 0)
					std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
			}

			flowMap[tuple] = label;
			Entry entry(tuple, 1, duration.count() + TIMEOUT);
			table[label] = entry;
			delQue.push(entry);
		}
		else if(op == 1){
			uint16_t label = digest - OFFSET;

			/*
			if(flowMap.find(tuple) == flowMap.end()){
				std::cout << "Cannot find tuple in PATH" << std::endl;
				continue;
			}
			if(table[label].state > 1 || !(table[label].tuple == tuple)){
				std::cout << "Already update in" << label << std::endl;
				continue;
			}
			*/

			if(dstAddr4 == 0){
				struct ip* iphdr = (struct ip*)(rsvp4_packet + sizeof(struct ether_header));
				iphdr->ip_src.s_addr = htonl(dstAddr1);
				iphdr->ip_dst.s_addr = htonl(srcAddr1);

				u_char *rsvp_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip));
				rsvp_header[1] = 2;

				u_char *object_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip) + 8);
				*(uint16_t*)object_header = htons(srcPort);
				*(uint16_t*)(object_header + 2) = htons(dstPort);
				*(uint32_t*)(object_header + 4) = htonl(srcAddr1);
				*(uint32_t*)(object_header + 8) = 0;
				*(uint32_t*)(object_header + 12) = 0;
				*(uint32_t*)(object_header + 16) = 0;
				*(uint32_t*)(object_header + 20) = htonl(dstAddr1);
				*(uint32_t*)(object_header + 24) = 0;
				*(uint32_t*)(object_header + 28) = 0;
				*(uint32_t*)(object_header + 32) = 0;
				*(uint16_t*)(object_header + 36) = htons(label + OFFSET);

				if (pcap_sendpacket(handle, rsvp4_packet, V4SIZE) != 0)
					std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
			}
			else{
				struct ip6_hdr* ip6hdr = (struct ip6_hdr*)(rsvp6_packet + sizeof(struct ether_header));
				ip6hdr->ip6_src.s6_addr32[0] = htonl(dstAddr1);
				ip6hdr->ip6_src.s6_addr32[1] = htonl(dstAddr2);
				ip6hdr->ip6_src.s6_addr32[2] = htonl(dstAddr3);
				ip6hdr->ip6_src.s6_addr32[3] = htonl(dstAddr4);
				ip6hdr->ip6_dst.s6_addr32[0] = htonl(srcAddr1);
				ip6hdr->ip6_dst.s6_addr32[1] = htonl(srcAddr2);
				ip6hdr->ip6_dst.s6_addr32[2] = htonl(srcAddr3);
				ip6hdr->ip6_dst.s6_addr32[3] = htonl(srcAddr4);

				u_char *rsvp_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
				rsvp_header[1] = 2;

				u_char *object_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr) + 8);
				*(uint16_t*)object_header = htons(srcPort);
				*(uint16_t*)(object_header + 2) = htons(dstPort);
				*(uint32_t*)(object_header + 4) = htonl(srcAddr1);
				*(uint32_t*)(object_header + 8) = htonl(srcAddr2);
				*(uint32_t*)(object_header + 12) = htonl(srcAddr3);
				*(uint32_t*)(object_header + 16) = htonl(srcAddr4);
				*(uint32_t*)(object_header + 20) = htonl(dstAddr1);
				*(uint32_t*)(object_header + 24) = htonl(dstAddr2);
				*(uint32_t*)(object_header + 28) = htonl(dstAddr3);
				*(uint32_t*)(object_header + 32) = htonl(dstAddr4);
				*(uint16_t*)(object_header + 36) = htons(label + OFFSET);


				if (pcap_sendpacket(handle, rsvp6_packet, V6SIZE) != 0)
					std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
			}

			table[label].state = 2;

			decompressTable->keyReset(decompressKey.get());
			decompressKey->setValue(1, digest);

			decompressTable->dataReset(16812096, decompressData.get());
			decompressData->setValue(1, srcAddr1);
			decompressData->setValue(2, srcAddr2);
			decompressData->setValue(3, srcAddr3);
			decompressData->setValue(4, srcAddr4);
			decompressData->setValue(5, dstAddr1);
			decompressData->setValue(6, dstAddr2);
			decompressData->setValue(7, dstAddr3);
			decompressData->setValue(8, dstAddr4);
			decompressData->setValue(9, srcPort);
			decompressData->setValue(10, dstPort);

			session->sessionCompleteOperations();
			decompressTable->tableEntryAdd(*session, dev_tgt, *decompressKey, *decompressData);
			session->sessionCompleteOperations();
		}
		else if(op == 2){
			uint16_t label = digest - OFFSET;

			/*
			if(flowMap.find(tuple) == flowMap.end()){
				std::cout << "Cannot find tuple in RESV" << std::endl;
				continue;
			}
			if(!(table[label].tuple == tuple)){
				std::cout << "Does not match in Table " << label << std::endl;
				continue;
			}
			*/

			compressTable->keyReset(compressKey.get());
			compressKey->setValue(1, srcAddr1);
			compressKey->setValue(2, srcAddr2);
			compressKey->setValue(3, srcAddr3);
			compressKey->setValue(4, srcAddr4);
			compressKey->setValue(5, dstAddr1);
			compressKey->setValue(6, dstAddr2);
			compressKey->setValue(7, dstAddr3);
			compressKey->setValue(8, dstAddr4);
			compressKey->setValue(9, srcPort);
			compressKey->setValue(10, dstPort);

			compressTable->dataReset(16829808, compressData.get());
			compressData->setValue(1, digest);

			session->sessionCompleteOperations();
			compressTable->tableEntryAdd(*session, dev_tgt, *compressKey, *compressData);
			session->sessionCompleteOperations();

			// std::cout << "RTT: " << duration.count() + TIMEOUT - table[label].timestamp << std::endl;

			table[label].state = 3;
		}
		else if(op == 5){
			uint16_t label = digest - OFFSET;

			if(table[label].state != 0){
				std::cout << "State of Table " << label << " is not 0 in PATHTEAR" << std::endl;
				continue;
			}

			decompressTable->keyReset(decompressKey.get());
			decompressKey->setValue(1, digest);

			session->sessionCompleteOperations();
			decompressTable->tableEntryDel(*session, dev_tgt, *decompressKey);
			session->sessionCompleteOperations();
		}
		else if(op == 15){
			uint16_t label = digest - OFFSET;
			if(table[label].state == 0){
				std::cout << "State of Table " << label << " is 0" << std::endl;
				continue;
			}

			compressTable->keyReset(compressKey.get());
			compressKey->setValue(1, srcAddr1);
			compressKey->setValue(2, srcAddr2);
			compressKey->setValue(3, srcAddr3);
			compressKey->setValue(4, srcAddr4);
			compressKey->setValue(5, dstAddr1);
			compressKey->setValue(6, dstAddr2);
			compressKey->setValue(7, dstAddr3);
			compressKey->setValue(8, dstAddr4);
			compressKey->setValue(9, srcPort);
			compressKey->setValue(10, dstPort);

			session->sessionCompleteOperations();
			compressTable->tableEntryDel(*session, dev_tgt, *compressKey);
			session->sessionCompleteOperations();

			if(dstAddr4 == 0){
				struct ip* iphdr = (struct ip*)(rsvp4_packet + sizeof(struct ether_header));
				iphdr->ip_src.s_addr = htonl(srcAddr1);
				iphdr->ip_dst.s_addr = htonl(dstAddr1);

				u_char *rsvp_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip));
				rsvp_header[1] = 5;

				u_char *object_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip) + 8);
				*(uint16_t*)object_header = htons(srcPort);
				*(uint16_t*)(object_header + 2) = htons(dstPort);
				*(uint32_t*)(object_header + 4) = htonl(srcAddr1);
				*(uint32_t*)(object_header + 8) = 0;
				*(uint32_t*)(object_header + 12) = 0;
				*(uint32_t*)(object_header + 16) = 0;
				*(uint32_t*)(object_header + 20) = htonl(dstAddr1);
				*(uint32_t*)(object_header + 24) = 0;
				*(uint32_t*)(object_header + 28) = 0;
				*(uint32_t*)(object_header + 32) = 0;
				*(uint16_t*)(object_header + 36) = htons(label + OFFSET);

				if (pcap_sendpacket(handle, rsvp4_packet, V4SIZE) != 0)
					std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
			}
			else{
				struct ip6_hdr* ip6hdr = (struct ip6_hdr*)(rsvp6_packet + sizeof(struct ether_header));
				ip6hdr->ip6_src.s6_addr32[0] = htonl(srcAddr1);
				ip6hdr->ip6_src.s6_addr32[1] = htonl(srcAddr2);
				ip6hdr->ip6_src.s6_addr32[2] = htonl(srcAddr3);
				ip6hdr->ip6_src.s6_addr32[3] = htonl(srcAddr4);
				ip6hdr->ip6_dst.s6_addr32[0] = htonl(dstAddr1);
				ip6hdr->ip6_dst.s6_addr32[1] = htonl(dstAddr2);
				ip6hdr->ip6_dst.s6_addr32[2] = htonl(dstAddr3);
				ip6hdr->ip6_dst.s6_addr32[3] = htonl(dstAddr4);

				u_char *rsvp_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
				rsvp_header[1] = 5;

				u_char *object_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr) + 8);
				*(uint16_t*)object_header = htons(srcPort);
				*(uint16_t*)(object_header + 2) = htons(dstPort);
				*(uint32_t*)(object_header + 4) = htonl(srcAddr1);
				*(uint32_t*)(object_header + 8) = htonl(srcAddr2);
				*(uint32_t*)(object_header + 12) = htonl(srcAddr3);
				*(uint32_t*)(object_header + 16) = htonl(srcAddr4);
				*(uint32_t*)(object_header + 20) = htonl(dstAddr1);
				*(uint32_t*)(object_header + 24) = htonl(dstAddr2);
				*(uint32_t*)(object_header + 28) = htonl(dstAddr3);
				*(uint32_t*)(object_header + 32) = htonl(dstAddr4);
				*(uint16_t*)(object_header + 36) = htons(label + OFFSET);


				if (pcap_sendpacket(handle, rsvp6_packet, V6SIZE) != 0)
					std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
			}

			table[label].state = 0;
			flowMap.erase(tuple);
			availableList.push_back(label);
		}
		else{
			printf("Unknown Op = %X\n", op);
			printf("Src Addr = %X %X %X %X\n", srcAddr1, srcAddr2, srcAddr3, srcAddr4);
			printf("Dst Addr = %X %X %X %X\n", dstAddr1, dstAddr2, dstAddr3, dstAddr4);
			printf("Ports    = %u %u\n", srcPort, dstPort);
			printf("Digest   = %u\n", digest);
		}
  	}

	while(!delQue.empty() && duration.count() > delQue.front().timestamp){
		Entry entry = delQue.front(); delQue.pop();
		uint16_t label = flowMap[entry.tuple];
		if(table[label].state == 0 || table[label].timestamp != entry.timestamp)
			continue;

		compressTable->keyReset(compressKey.get());
		compressKey->setValue(1, entry.tuple.srcAddr1);
		compressKey->setValue(2, entry.tuple.srcAddr2);
		compressKey->setValue(3, entry.tuple.srcAddr3);
		compressKey->setValue(4, entry.tuple.srcAddr4);
		compressKey->setValue(5, entry.tuple.dstAddr1);
		compressKey->setValue(6, entry.tuple.dstAddr2);
		compressKey->setValue(7, entry.tuple.dstAddr3);
		compressKey->setValue(8, entry.tuple.dstAddr4);
		compressKey->setValue(9, entry.tuple.srcPort);
		compressKey->setValue(10, entry.tuple.dstPort);

		session->sessionCompleteOperations();
		compressTable->tableEntryDel(*session, dev_tgt, *compressKey);
		session->sessionCompleteOperations();

		if(entry.tuple.dstAddr4 == 0){
			struct ip* iphdr = (struct ip*)(rsvp4_packet + sizeof(struct ether_header));
			iphdr->ip_src.s_addr = htonl(entry.tuple.srcAddr1);
			iphdr->ip_dst.s_addr = htonl(entry.tuple.dstAddr1);

			u_char *rsvp_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip));
			rsvp_header[1] = 5;

			u_char *object_header = (u_char*)(rsvp4_packet + sizeof(struct ether_header) + sizeof(struct ip) + 8);
			*(uint16_t*)object_header = htons(entry.tuple.srcPort);
			*(uint16_t*)(object_header + 2) = htons(entry.tuple.dstPort);
			*(uint32_t*)(object_header + 4) = htonl(entry.tuple.srcAddr1);
			*(uint32_t*)(object_header + 8) = 0;
			*(uint32_t*)(object_header + 12) = 0;
			*(uint32_t*)(object_header + 16) = 0;
			*(uint32_t*)(object_header + 20) = htonl(entry.tuple.dstAddr1);
			*(uint32_t*)(object_header + 24) = 0;
			*(uint32_t*)(object_header + 28) = 0;
			*(uint32_t*)(object_header + 32) = 0;
			*(uint16_t*)(object_header + 36) = htons(label + OFFSET);

			if (pcap_sendpacket(handle, rsvp4_packet, V4SIZE) != 0)
				std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
		}
		else{
			struct ip6_hdr* ip6hdr = (struct ip6_hdr*)(rsvp6_packet + sizeof(struct ether_header));
			ip6hdr->ip6_src.s6_addr32[0] = htonl(entry.tuple.srcAddr1);
			ip6hdr->ip6_src.s6_addr32[1] = htonl(entry.tuple.srcAddr2);
			ip6hdr->ip6_src.s6_addr32[2] = htonl(entry.tuple.srcAddr3);
			ip6hdr->ip6_src.s6_addr32[3] = htonl(entry.tuple.srcAddr4);
			ip6hdr->ip6_dst.s6_addr32[0] = htonl(entry.tuple.dstAddr1);
			ip6hdr->ip6_dst.s6_addr32[1] = htonl(entry.tuple.dstAddr2);
			ip6hdr->ip6_dst.s6_addr32[2] = htonl(entry.tuple.dstAddr3);
			ip6hdr->ip6_dst.s6_addr32[3] = htonl(entry.tuple.dstAddr4);

			u_char *rsvp_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
			rsvp_header[1] = 5;

			u_char *object_header = (u_char*)(rsvp6_packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr) + 8);
			*(uint16_t*)object_header = htons(entry.tuple.srcPort);
			*(uint16_t*)(object_header + 2) = htons(entry.tuple.dstPort);
			*(uint32_t*)(object_header + 4) = htonl(entry.tuple.srcAddr1);
			*(uint32_t*)(object_header + 8) = htonl(entry.tuple.srcAddr2);
			*(uint32_t*)(object_header + 12) = htonl(entry.tuple.srcAddr3);
			*(uint32_t*)(object_header + 16) = htonl(entry.tuple.srcAddr4);
			*(uint32_t*)(object_header + 20) = htonl(entry.tuple.dstAddr1);
			*(uint32_t*)(object_header + 24) = htonl(entry.tuple.dstAddr2);
			*(uint32_t*)(object_header + 28) = htonl(entry.tuple.dstAddr3);
			*(uint32_t*)(object_header + 32) = htonl(entry.tuple.dstAddr4);
			*(uint16_t*)(object_header + 36) = htons(label + OFFSET);


			if (pcap_sendpacket(handle, rsvp6_packet, V6SIZE) != 0)
				std::cerr << "Error sending packet: " << pcap_geterr(handle) << std::endl;
		}

		table[label].state = 0;
		flowMap.erase(entry.tuple);
		availableList.push_back(label);
	}

	return bfrtLearnDigest->bfRtLearnNotifyAck(bfrtsession, learn_msg_hdl);
}
