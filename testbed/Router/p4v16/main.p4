#include <core.p4>
#include <tna.p4>

#define CPU_PORT 192

#define MPLS_SIZE 16384

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86DD
#define ETHERTYPE_MPLS 0x8847

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

header ipv6_t {
    bit<4>      version;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<20>     flow_label;
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
    rsvp_t rsvp;
    object_t object;
}

/*================================
=            Metadata            =
================================*/


struct metadata_t {
    bit<16>     ingress;
    bit<16>     digest;

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

    bit<8>      op;

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
    bit<16>     ingress;
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
            IP_RSVP : parse_rsvp;
            default : accept;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            IP_RSVP : parse_rsvp;
            default : accept;
        }
    }
    state parse_mpls {
        pkt.extract(hdr.mpls);
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
    apply {
        if (ig_dprsr_md.digest_type == 1) {
            digest_flow.pack({md.srcAddr1, md.srcAddr2, md.srcAddr3, md.srcAddr4,
                md.dstAddr1, md.dstAddr2, md.dstAddr3, md.dstAddr4, 
                md.srcPort, md.dstPort, md.digest, md.ingress, md.op});
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

    
    action ai_drop() {
        ig_dprsr_md.drop_ctl = 0x1;
    }
    action ai_forward_user(bit<9> egress_port){
        ig_tm_md.ucast_egress_port = egress_port;
    }

    table ti_forward4_user {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            ai_forward_user;
            ai_drop;
        }
        default_action = ai_drop;
        size = 65536;
    }
    table ti_forward6_user {
        key = {
            hdr.ipv6.dstAddr1 : exact;
            hdr.ipv6.dstAddr2 : exact;
            hdr.ipv6.dstAddr3 : exact;
            hdr.ipv6.dstAddr4 : exact;
        }
        actions = {
            ai_forward_user;
            ai_drop;
        }
        default_action = ai_drop;
    }

    action ai_forward_mpls(bit<16> label, bit<9> egress_port) {
        hdr.mpls.label = label;
        ig_tm_md.ucast_egress_port = egress_port;
    }
    table table_read_mpls_out {
        key = {
            hdr.mpls.label : exact;
        }
        actions = { 
            ai_forward_mpls;
            ai_drop;
        }
	    const default_action = ai_drop();
        size = 65536;
    }

    apply {
        if(hdr.ipv4.isValid()) {
            ti_forward4_user.apply();
	    }
        else if(hdr.ipv6.isValid()) {
            ti_forward6_user.apply();
        }
        else if(hdr.mpls.isValid()){
            table_read_mpls_out.apply();
        }
        

        if(hdr.rsvp.isValid()){
            md.op = hdr.rsvp.type;
            // From other switches
            if(ig_intr_md.ingress_port != CPU_PORT){
                md.ingress = (bit<16>)ig_intr_md.ingress_port;
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

                if(hdr.rsvp.type == RSVP_PATHERR){

                }
                else{
                    ig_dprsr_md.digest_type = 1;
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