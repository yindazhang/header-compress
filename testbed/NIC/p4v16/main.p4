#include <core.p4>
#include <tna.p4>

#define CPU_PORT 192

#define THRESHOLD 100
#define MPLS_SIZE 16384

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86DD
#define ETHERTYPE_MPLS 0x8847

#define IP_TCP 6
#define IP_UDP 17
#define IP_RSVP 46

#define RSVP_PATH 1
#define RSVP_RESV 2
#define RSVP_PATHERR 3
#define RSVP_RESVERR 4
#define RSVP_PATHTEAR 5
#define RSVP_RESVTEAR 6

#define NECT 0
#define ECT0 1
#define ECT1 2
#define CE 3

/*==============================
=            Header            =
==============================*/

header ethernet_t {
    bit<48>     dstAddr;
    bit<48>     srcAddr;
    bit<16>     etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    bit<32>     srcAddr;
    bit<32>     dstAddr;
}

header compress_ipv4_t {
    bit<8>      protocol;
    bit<16>     totalLen;
    bit<16>     identification;
}

header ipv6_t {
    bit<4>      version;
    bit<4>      dscp1;
    bit<2>      dscp2;
    bit<2>      ecn;
    bit<4>      flow_label1;
    bit<16>     flow_label2;
    bit<16>     payload_length;
    bit<8>      nextHdr;
    bit<8>      hopLimit;
    bit<32>     srcAddr1;
    bit<32>     srcAddr2;
    bit<32>     srcAddr3;
    bit<32>     srcAddr4;
    bit<32>     dstAddr1;
    bit<32>     dstAddr2;
    bit<32>     dstAddr3;
    bit<32>     dstAddr4;
}

header compress_ipv6_t {
    bit<8>      nextHdr;
    bit<16>     flow_label2;
    bit<16>     payload_length;
}

header port_t {
    bit<16> srcPort;
    bit<16> dstPort;
}

header compress_tcp_t {
    bit<32> seqNo;
    bit<32> ackNo;
    bit<8>  dataOffset;
    bit<8>  flags;
}

header rsvp_t {
    bit<4>      version;
    bit<4>      flags;
    bit<8>      type;
    bit<16>     check;
    bit<8>      ttl;
    bit<8>      reserved;
    bit<16>     length;
}

header object_t {
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<32>     srcAddr1;
    bit<32>     srcAddr2;
    bit<32>     srcAddr3;
    bit<32>     srcAddr4;
    bit<32>     dstAddr1;
    bit<32>     dstAddr2;
    bit<32>     dstAddr3;
    bit<32>     dstAddr4;
    bit<16>     label;
}

header mpls_t {
    bit<16>     label;
    bit<4>      resv;
    bit<1>      type;
    bit<2>      ecn;
    bit<1>      bottom;
    bit<8>      ttl;
}

struct header_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
    ipv6_t ipv6;
    mpls_t mpls;
    compress_ipv4_t compress_ipv4;
    compress_ipv6_t compress_ipv6;
    port_t port;
    compress_tcp_t compress_tcp;
    rsvp_t rsvp;
    object_t object;
}

/*================================
=            Metadata            =
================================*/


struct metadata_t {
    bit<32>     srcAddr1;
    bit<32>     srcAddr2;
    bit<32>     srcAddr3;
    bit<32>     srcAddr4;
    bit<32>     dstAddr1;
    bit<32>     dstAddr2;
    bit<32>     dstAddr3;
    bit<32>     dstAddr4;

    bit<16>     srcPort;
    bit<16>     dstPort;

    bit<16>     digest;

    bit<16>     label;
    bit<8>      op;

    bit<8>      compress;
    bit<8>      type;
    bit<8>      ingress;
    bit<8>      port;

    bit<8>      sample;

    bit<19>     congestion_;
}

struct digest_t {
    bit<32>     srcAddr1;
    bit<32>     srcAddr2;
    bit<32>     srcAddr3;
    bit<32>     srcAddr4;
    bit<32>     dstAddr1;
    bit<32>     dstAddr2;
    bit<32>     dstAddr3;
    bit<32>     dstAddr4;
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<16>     digest;
    bit<8>      op;
}

/*========================================
=            Ingress parsing             =
========================================*/


parser EthIngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t md,
        out ingress_intrinsic_metadata_t ig_intr_md){

    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_IPV6 : parse_ipv6;
            ETHERTYPE_MPLS : parse_mpls;
            default : accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_TCP : parse_tcp;
            IP_UDP : parse_udp;
            IP_RSVP : parse_rsvp;
            default : accept;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            IP_TCP : parse_tcp;
            IP_UDP : parse_udp;
            IP_RSVP : parse_rsvp;
            default : accept;
        }
    }
    state parse_mpls {
        pkt.extract(hdr.mpls);
        transition select(hdr.mpls.type) {
            0 : parse_compress_ipv4;
            1 : parse_compress_ipv6;
            default : accept;
        }
    }
    state parse_compress_ipv4 {
        pkt.extract(hdr.compress_ipv4);
        transition select(hdr.compress_ipv4.protocol) {
            IP_TCP : parse_compress_tcp;
            IP_UDP : parse_compress_udp;
            default : accept;
        }
    }
    state parse_compress_ipv6 {
        pkt.extract(hdr.compress_ipv6);
        transition select(hdr.compress_ipv6.nextHdr) {
            IP_TCP : parse_compress_tcp;
            IP_UDP : parse_compress_udp;
            default : accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.port);
        pkt.extract(hdr.compress_tcp);
        transition accept;
    }
    state parse_udp {
        pkt.extract(hdr.port);
        transition accept;
    }
    state parse_compress_tcp {
        md.port = 1;
        transition accept;
    }
    state parse_compress_udp {
        md.port = 1;
        transition accept;
    }
    state parse_rsvp {
        pkt.extract(hdr.rsvp);
        pkt.extract(hdr.object);
        transition accept;
    }
}


parser IngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(64);
        EthIngressParser.apply(pkt, hdr, md, ig_intr_md);
        transition accept;
    }
}

control IngressDeparser(
        packet_out pkt, 
        inout header_t hdr,
        in metadata_t md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
            
    Digest<digest_t>() digest_flow;
    Checksum() ipv4_csum;
    apply {
        if(hdr.ipv4.isValid()) {
            hdr.ipv4.hdrChecksum = ipv4_csum.update({
                hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp,
                hdr.ipv4.ecn, hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags, hdr.ipv4.fragOffset,
                hdr.ipv4.ttl, hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr});
        }

        if (ig_dprsr_md.digest_type == 1) {
            digest_flow.pack({md.srcAddr1, md.srcAddr2, md.srcAddr3, md.srcAddr4,
                md.dstAddr1, md.dstAddr2, md.dstAddr3, md.dstAddr4, 
                md.srcPort, md.dstPort, md.digest, md.op});
        }

        pkt.emit(hdr);
    }
}

/*========================================
=            Egress parsing             =
========================================*/


parser EthEgressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t md,
        out egress_intrinsic_metadata_t eg_intr_md){
    
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_IPV6 : parse_ipv6;
            ETHERTYPE_MPLS : parse_mpls;
            default : accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition accept;
    }
    state parse_mpls {
        pkt.extract(hdr.mpls);
        transition accept;
    }
}

parser EgressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t md,
        out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        pkt.extract(eg_intr_md);
        EthEgressParser.apply(pkt, hdr, md, eg_intr_md);
        transition accept;
    }
}


control EgressDeparser(
        packet_out pkt, 
        inout header_t hdr,
        in metadata_t md,
        in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
    apply {
        pkt.emit(hdr);
    }
}

/*===========================================
=            Ingress match-action           =
===========================================*/

control Ingress(
        inout header_t hdr, 
        inout metadata_t md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    action ai_nop() {
    }
    action ai_drop() {
        ig_dprsr_md.drop_ctl = 0x1;
    }
    action ai_forward_rsvp(bit<9> egress_port){
        ig_tm_md.ucast_egress_port = egress_port;
    }
    action ai_forward_user(bit<9> egress_port, bit<8> ingress){
        ig_tm_md.ucast_egress_port = egress_port;
        md.ingress = ingress;
    }

    table ti_forward_user {
        key = {
            ig_intr_md.ingress_port : exact;
        }
        actions = {
            ai_forward_user;
            ai_nop;
        }
        default_action = ai_nop;
    }
    table ti_forward4_rsvp {
        key = {
            hdr.ipv4.srcAddr : exact;
        }
        actions = {
            ai_forward_rsvp;
            ai_drop;
        }
        default_action = ai_drop;
    }
    table ti_forward6_rsvp {
        key = {
            hdr.ipv6.srcAddr1 : exact;
            hdr.ipv6.srcAddr2 : exact;
            hdr.ipv6.srcAddr3 : exact;
            hdr.ipv6.srcAddr4 : exact;
        }
        actions = {
            ai_forward_rsvp;
            ai_drop;
        }
        default_action = ai_drop;
    }

    action ai_decompress_mpls(bit<32> srcAddr1, bit<32> srcAddr2, bit<32> srcAddr3, bit<32> srcAddr4,
                        bit<32> dstAddr1, bit<32> dstAddr2, bit<32> dstAddr3, bit<32> dstAddr4,
                        bit<16> srcPort, bit<16> dstPort){
        md.srcAddr1 = srcAddr1;
        md.srcAddr2 = srcAddr2;
        md.srcAddr3 = srcAddr3;
        md.srcAddr4 = srcAddr4;
        md.dstAddr1 = dstAddr1;
        md.dstAddr2 = dstAddr2;
        md.dstAddr3 = dstAddr3;
        md.dstAddr4 = dstAddr4;
        md.srcPort = srcPort;
        md.dstPort = dstPort;
    }
    table ti_decompress_mpls {
        key = {
            hdr.mpls.label : exact;
        }
        actions = {
            ai_decompress_mpls;
            ai_nop;
        }
        default_action = ai_nop;
        size = 65536;
    }

    action ai_compress_mpls(bit<16> label){
        md.label = label;
        md.compress = 1;
    }
    table ti_compress_mpls {
        key = {
            md.srcAddr1 : exact;
            md.srcAddr2 : exact;
            md.srcAddr3 : exact;
            md.srcAddr4 : exact;
            md.dstAddr1 : exact;
            md.dstAddr2 : exact;
            md.dstAddr3 : exact;
            md.dstAddr4 : exact;
            md.srcPort : exact;
            md.dstPort : exact;
        }
        actions = {
            ai_compress_mpls;
            ai_nop;
        }
        default_action = ai_nop;
        size = 65536;
    }

    CRCPolynomial<bit<32>>(coeff=0x04C11DB7,reversed=true, msb=false, extended=false, init=0xFFFFFFFF, xor=0xFFFFFFFF) crc32;
	Hash<bit<16>>(HashAlgorithm_t.CUSTOM, crc32) counter_hash;
	Register<bit<8>, _>(65536) counter_count;

    RegisterAction<bit<8>, _, bit<8>>(counter_count) counter_count_alu={
		void apply(inout bit<8> val, out bit<8> ret){
            if(val == THRESHOLD){
                val = 0;
                ret = 1;
            }
            else{
                val = val + 1;
                ret = 0;
            }
		}
	};
	action check_counter_count(){
		md.sample = counter_count_alu.execute(counter_hash.get({
            md.srcAddr1, md.srcAddr2, md.srcAddr3, md.srcAddr4,
            md.dstAddr1, md.dstAddr2, md.dstAddr3, md.dstAddr4,
            md.srcPort, md.dstPort}));
	}
	table counter_count_table{
		actions = {
			check_counter_count;
		}
		size = 1;
		const default_action = check_counter_count();
	}

    apply {
        ti_forward_user.apply();

        if(hdr.mpls.isValid()){
            ti_decompress_mpls.apply();

            if(hdr.compress_ipv4.isValid()){
                hdr.ethernet.etherType = ETHERTYPE_IPV4;
                hdr.ipv4.setValid();

                hdr.ipv4.version = 4;
                hdr.ipv4.ihl = 5;
                hdr.ipv4.ecn = hdr.mpls.ecn;
                hdr.ipv4.totalLen = hdr.compress_ipv4.totalLen;
                hdr.ipv4.identification = hdr.compress_ipv4.identification;
                hdr.ipv4.flags = 2;
                hdr.ipv4.ttl = hdr.mpls.ttl;
                hdr.ipv4.protocol = hdr.compress_ipv4.protocol;
                hdr.ipv4.srcAddr = md.srcAddr1;
                hdr.ipv4.dstAddr = md.dstAddr1;

                hdr.compress_ipv4.setInvalid();
            }
            else if(hdr.compress_ipv6.isValid()){
                hdr.ethernet.etherType = ETHERTYPE_IPV6;
                hdr.ipv6.setValid();

                hdr.ipv6.version = 6;
                hdr.ipv6.ecn = hdr.mpls.ecn;
                hdr.ipv6.flow_label2 = hdr.compress_ipv6.flow_label2;
                hdr.ipv6.payload_length = hdr.compress_ipv6.payload_length;
                hdr.ipv6.nextHdr = hdr.compress_ipv6.nextHdr;
                hdr.ipv6.hopLimit = hdr.mpls.ttl;
                hdr.ipv6.srcAddr1 = md.srcAddr1;
                hdr.ipv6.srcAddr2 = md.srcAddr2;
                hdr.ipv6.srcAddr3 = md.srcAddr3;
                hdr.ipv6.srcAddr4 = md.srcAddr4;
                hdr.ipv6.dstAddr1 = md.dstAddr1;
                hdr.ipv6.dstAddr2 = md.dstAddr2;
                hdr.ipv6.dstAddr3 = md.dstAddr3;
                hdr.ipv6.dstAddr4 = md.dstAddr4;

                hdr.compress_ipv6.setInvalid();
            }

            if(md.port == 1){
                hdr.port.setValid();
                hdr.port.srcPort = md.srcPort;
                hdr.port.dstPort = md.dstPort;
            }
            hdr.mpls.setInvalid();
            // ig_tm_md.ucast_egress_port = CPU_PORT;
        }
        else if(ig_intr_md.ingress_port == CPU_PORT){
            if(hdr.ipv4.isValid()){
                ti_forward4_rsvp.apply();
            }
            else if(hdr.ipv6.isValid()){
                ti_forward6_rsvp.apply();
            }
        }
        else if(hdr.rsvp.isValid()){
            ig_dprsr_md.drop_ctl = 0x1;
            ig_dprsr_md.digest_type = 1;

            md.op = hdr.rsvp.type;
            if(hdr.object.isValid()){
                md.srcAddr1 = hdr.object.srcAddr1;
                md.srcAddr2 = hdr.object.srcAddr2;
                md.srcAddr3 = hdr.object.srcAddr3;
                md.srcAddr4 = hdr.object.srcAddr4;
                md.dstAddr1 = hdr.object.dstAddr1;
                md.dstAddr2 = hdr.object.dstAddr2;
                md.dstAddr3 = hdr.object.dstAddr3;
                md.dstAddr4 = hdr.object.dstAddr4;
                md.srcPort = hdr.object.srcPort;
                md.dstPort = hdr.object.dstPort;
                md.digest = hdr.object.label;
            }
        }
        else{
            if(hdr.ipv4.isValid()){
                md.srcAddr1 = hdr.ipv4.srcAddr;
                md.dstAddr1 = hdr.ipv4.dstAddr;
            }
            else if(hdr.ipv6.isValid()){
                md.srcAddr1 = hdr.ipv6.srcAddr1;
                md.dstAddr1 = hdr.ipv6.dstAddr1;
                md.srcAddr2 = hdr.ipv6.srcAddr2;
                md.dstAddr2 = hdr.ipv6.dstAddr2;
                md.srcAddr3 = hdr.ipv6.srcAddr3;
                md.dstAddr3 = hdr.ipv6.dstAddr3;
                md.srcAddr4 = hdr.ipv6.srcAddr4;
                md.dstAddr4 = hdr.ipv6.dstAddr4;
            }

            if(hdr.port.isValid()){
                md.srcPort = hdr.port.srcPort;
                md.dstPort = hdr.port.dstPort;
            }

            if(md.ingress == 1){
                ti_compress_mpls.apply();
                if(md.compress == 1){
                    hdr.ethernet.etherType = ETHERTYPE_MPLS;
                    hdr.mpls.setValid();
                    hdr.mpls.bottom = 1;

                    if(hdr.ipv4.isValid()){
                        md.type = 0;
                        hdr.compress_ipv4.setValid();

                        hdr.mpls.ecn = hdr.ipv4.ecn;
                        hdr.mpls.ttl = hdr.ipv4.ttl;
                        hdr.mpls.label = md.label;

                        hdr.compress_ipv4.totalLen = hdr.ipv4.totalLen;
                        hdr.compress_ipv4.identification = hdr.ipv4.identification;
                        hdr.compress_ipv4.protocol = hdr.ipv4.protocol;

                        hdr.ipv4.setInvalid();
                    }
                    else if(hdr.ipv6.isValid()){
                        md.type = 1;
                        hdr.compress_ipv6.setValid();

                        hdr.mpls.ecn = hdr.ipv6.ecn;
                        hdr.mpls.ttl = hdr.ipv6.hopLimit;
                        hdr.mpls.label = md.label;

                        hdr.compress_ipv6.flow_label2 = hdr.ipv6.flow_label2;
                        hdr.compress_ipv6.payload_length = hdr.ipv6.payload_length;
                        hdr.compress_ipv6.nextHdr = hdr.ipv6.nextHdr;
                        
                        hdr.ipv6.setInvalid();
                    }

                    if(hdr.port.isValid()){
                        hdr.port.setInvalid();
                    }

                    if(hdr.compress_tcp.isValid()){
                        if((hdr.compress_tcp.flags & 1) != 0){
                            md.op = 15;
                            ig_dprsr_md.digest_type = 1;
                        }
                    }

                    hdr.mpls.type = (bit<1>)md.type;
                    // ig_tm_md.ucast_egress_port = CPU_PORT;
                }
                else{
                    counter_count_table.apply();
                    if(md.sample == 1){
                        md.op = 14;
                        ig_dprsr_md.digest_type = 1;
                    }
                }
            }
        }
    }
}

/*===========================================
=            Egress match-action           =
===========================================*/

control Egress(
        inout header_t hdr, 
        inout metadata_t md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
        inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_oport_md){

    action action_congestion_() {
        md.congestion_ = (eg_intr_md.deq_qdepth & 0x7f800);
    }
    table table_congestion_ {
        actions = { action_congestion_; }
	    const default_action = action_congestion_();
    }

    apply {
        table_congestion_.apply();
        if(md.congestion_ != 0){
            if(hdr.ipv4.isValid()) {
                if(hdr.ipv4.ecn == ECT0 || hdr.ipv4.ecn == ECT1){
                    hdr.ipv4.ecn = CE;
                }
            }
            else if(hdr.ipv6.isValid()) {
                if(hdr.ipv6.ecn == ECT0 || hdr.ipv6.ecn == ECT1){
                    hdr.ipv6.ecn = CE;
                }
            }
            else if(hdr.mpls.isValid()) {
                if(hdr.mpls.ecn == ECT0 || hdr.mpls.ecn == ECT1){
                    hdr.mpls.ecn = CE;
                }
            }
        }
    }
}

/*==============================================
=            The switch's pipeline             =
==============================================*/
Pipeline(
    IngressParser(), Ingress(), IngressDeparser(),
    EgressParser(), Egress(), EgressDeparser()) pipe;

Switch(pipe) main;