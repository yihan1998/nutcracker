/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 0x800;

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<32> ip4Addr_t;

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header tcp_t {
    bit<16> sport; // Source port
    bit<16> dport; // Destination port
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  reserved;
    bit<9>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> sport; // Source port
    bit<16> dport; // Destination port
    bit<16> length;
    bit<16> checksum;
}

struct metadata {
    /* empty */
}

struct headers {
    ethernet_t  ethernet;
    ipv4_t      ipv4;
    udp_t       udp;
    tcp_t       tcp;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            0x11: parse_udp;
            0x06: parse_tcp;
            default: accept;
        }
    }

    state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }

    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }
}


/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    counter((bit<32>)65536, CounterType.packets) counters;

    action do_dns_filter() {
        // Placeholder
    }

    action drop() {
        mark_to_drop(standard_metadata);
    }

    action dns_filter() {
        do_dns_filter();
    }

    action monitor_udp_flow() {
        counters.count((bit<32>)hdr.udp.dport);
    }

    action monitor_tcp_flow() {
        counters.count((bit<32>)hdr.tcp.dport);
    }

   table udp_flow_monitor_tbl {
        key = {
            hdr.udp.dport: exact; // Match on UDP destination port
        }
        actions = {
            monitor_udp_flow;
            NoAction;
        }
        size = 2048; // Adjust size as needed
        default_action = NoAction();
    }

    // Flow monitor table for TCP packets
    table tcp_flow_monitor_tbl {
        key = {
            hdr.tcp.dport: exact; // Match on TCP destination port
        }
        actions = {
            monitor_tcp_flow;
            NoAction;
        }
        size = 2048; // Adjust size as needed
        default_action = NoAction();
    }

    apply {
        if (hdr.ipv4.protocol == 17 && hdr.udp.dport == 53) {
            dns_filter();
        } else {
            if (hdr.ipv4.protocol == 17) {
                udp_flow_monitor_tbl.apply();
            } else if (hdr.ipv4.protocol == 6) {
                tcp_flow_monitor_tbl.apply();
            } else {
                mark_to_drop(standard_metadata);
            }
        }
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {  }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
     apply {
        update_checksum(
            hdr.ipv4.isValid(),
            { hdr.ipv4.version,
              hdr.ipv4.ihl,
              hdr.ipv4.diffserv,
              hdr.ipv4.totalLen,
              hdr.ipv4.identification,
              hdr.ipv4.flags,
              hdr.ipv4.fragOffset,
              hdr.ipv4.ttl,
              hdr.ipv4.protocol,
              hdr.ipv4.srcAddr,
              hdr.ipv4.dstAddr },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16);
    }
}


/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        /*
        Control function for deparser.

        This function is responsible for constructing the output packet by appending
        headers to it based on the input headers.

        Parameters:
        - packet: Output packet to be constructed.
        - hdr: Input headers to be added to the output packet.

        TODO: Implement the logic for constructing the output packet by appending
        headers based on the input headers.
        */
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;