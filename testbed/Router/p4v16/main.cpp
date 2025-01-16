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
#define TIMEOUT 100000

pcap_t *handle;
u_char rsvp4_packet[V4SIZE] = {0};
u_char rsvp6_packet[V6SIZE] = {0};

p4_pd_sess_hdl_t p4_sess_hdl;
const bfrt::BfRtLearn *bfrtLearnDigest = nullptr;
bf_rt_id_t learn_srcPort, learn_dstPort, learn_srcAddr1, learn_srcAddr2, learn_srcAddr3, learn_srcAddr4, 
				learn_dstAddr1, learn_dstAddr2, learn_dstAddr3, learn_dstAddr4, learn_digest, learn_ingress, learn_op;

bf_status_t bf_status;
bf_rt_target_t dev_tgt;
std::shared_ptr<bfrt::BfRtSession> session;
const bfrt::BfRtInfo *bfrtInfo = nullptr;

const bfrt::BfRtTable *mplsTable;
std::unique_ptr<bfrt::BfRtTableKey> mplsKey;
std::unique_ptr<bfrt::BfRtTableData> mplsData;

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
	uint64_t ingress;
	NetTuple tuple;

	Entry(){
		state = timestamp = 0;
	};

	Entry(NetTuple _tuple, uint64_t _state = 0, uint64_t _timestamp = 0, uint64_t _ingress = 0):
		tuple(_tuple), state(_state), timestamp(_timestamp), ingress(_ingress){};
};

std::vector<Entry> table;
std::unordered_map<NetTuple, uint16_t, NetTupleHash> flowMap;

int main(int argc, char **argv) {
	start_switchd(argc, argv);
	sleep(10);
	std::cout << "Init" << std::endl;

	table.resize(MPLS_SIZE);

	init_nic();

	dev_tgt.dev_id = 0;
	dev_tgt.pipe_id = 0xffff;

	auto &devMgr = bfrt::BfRtDevMgr::getInstance();

	bf_status = devMgr.bfRtInfoGet(dev_tgt.dev_id, "main", &bfrtInfo);

	session = bfrt::BfRtSession::sessionCreate();

	bf_status = bfrtInfo->bfrtTableFromNameGet("pipe.Ingress.table_read_mpls_out", &mplsTable);
	bf_status = mplsTable->keyAllocate(&mplsKey);
    bf_status = mplsTable->dataAllocate(&mplsData);

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
	bf_status = bfrtLearnDigest->learnFieldIdGet("ingress", &learn_ingress);
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
	uint64_t ingress, digest, op;

	NetTuple tuple;

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
		vec[i].get()->getValue(learn_ingress, &ingress);
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
	
		uint16_t label = digest - OFFSET;
		if(op == 1){
			if(flowMap.find(tuple) != flowMap.end()){
				std::cout << "Already inserted" << std::endl;
				continue;
			}
			if(table[label].state != 0){
				std::cout << "Table " << label << " is in used" << std::endl;
				continue;
			}

			flowMap[tuple] = label;
			Entry entry(tuple, 1, -1, ingress);
			table[label] = entry;
		}
		else if(op == 2){
			if(flowMap.find(tuple) == flowMap.end()){
				std::cout << "Cannot find tuple in RESV" << std::endl;
				continue;
			}
			if(!(table[label].tuple == tuple)){
				std::cout << "Does not match in Table " << label << std::endl;
				continue;
			}

			mplsTable->keyReset(mplsKey.get());
			mplsKey->setValue(1, digest);

			mplsTable->dataReset(16793932, mplsData.get());
			mplsData->setValue(1, digest);
			mplsData->setValue(2, ingress);

			session->sessionCompleteOperations();
			mplsTable->tableEntryAdd(*session, dev_tgt, *mplsKey, *mplsData);
			session->sessionCompleteOperations();

			table[label].state = 2;
		}
		else if(op == 5){
			flowMap.erase(tuple);
			table[label].state = 0;

			mplsTable->keyReset(mplsKey.get());
			mplsKey->setValue(1, digest);

			session->sessionCompleteOperations();
			mplsTable->tableEntryDel(*session, dev_tgt, *mplsKey);
			session->sessionCompleteOperations();
		}
		
			printf("Unknown Op = %X\n", op);
			printf("Src Addr = %X %X %X %X\n", srcAddr1, srcAddr2, srcAddr3, srcAddr4);
			printf("Dst Addr = %X %X %X %X\n", dstAddr1, dstAddr2, dstAddr3, dstAddr4);
			printf("Ports    = %u %u\n", srcPort, dstPort);
			printf("Ingress  = %u\n", ingress);
			printf("Digest   = %u\n", digest);
		
  	}

	return bfrtLearnDigest->bfRtLearnNotifyAck(bfrtsession, learn_msg_hdl);
}