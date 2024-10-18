#ifndef _STATEMACHINE_INTERNAL_H_
#define _STATEMACHINE_INTERNAL_H_

#include "backend/flowPipeInternal.h"
#include "utils/args.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iomanip>
#include <arpa/inet.h>
#include <netinet/in.h>

class Pipe {
public:
    virtual void print() const = 0; // Pure virtual function for printing component details
};

class ConditionalPipe : public Pipe {
public:
    std::string name;
    std::unordered_map<std::string, Pipe*> tables;

    ConditionalPipe(const std::string& name) : name(name) {}

    // void addTable(const TablePipe& table) {
    //     tables[table.name] = table; // Add table to the conditional
    // }

    void print() const override {
        std::cout << "Conditional Name: " << name << std::endl;
        // for (const auto& pair : tables) {
        //     pair.second.print();
        // }
    }
};

class TableEntry {
public:
    struct flow_match match;
    struct flow_actions action;
    struct flow_monitor monitor;
    struct flow_fwd fwd;
    std::vector<ArgBase*> args;
    int count;

    // Constructor that accepts parameters
    TableEntry(struct flow_match *match_ptr, struct flow_actions *action_ptr, struct flow_monitor* monitor_ptr, struct flow_fwd *fwd_ptr) : count(0) {
        if (match_ptr) {
            match = *match_ptr;
        }
        if (action_ptr) {
            action = *action_ptr;
            extract_args();
        }
        if (monitor_ptr) {
            monitor = *monitor_ptr;
        }
        if (fwd_ptr) {
            fwd = *fwd_ptr;
        }
    }

    void extract_args() {
        uint8_t eth_addr[6] = {0};
        if (action.meta.pkt_meta) {
            args.push_back(new ArgUInt32(action.meta.pkt_meta));
        }
        if (memcmp(action.outer.eth.h_dest, &eth_addr, 6) != 0) {
            std::array<uint8_t, 6> mac_dest;
            std::copy(action.outer.eth.h_dest, action.outer.eth.h_dest + 6, mac_dest.begin());
            args.push_back(new ArgMac(mac_dest));
        }
        if (memcmp(action.outer.eth.h_source, &eth_addr, 6) != 0) {
            std::array<uint8_t, 6> mac_source;
            std::copy(action.outer.eth.h_source, action.outer.eth.h_source + 6, mac_source.begin());
            args.push_back(new ArgMac(mac_source));
        }
        if (action.outer.ip4.saddr) {
            args.push_back(new ArgUInt32(action.outer.ip4.saddr));
        }
        if (action.outer.ip4.daddr) {
            args.push_back(new ArgUInt32(action.outer.ip4.daddr));
        }
        if (action.outer.udp.source) {
            args.push_back(new ArgUInt16(action.outer.udp.source));
        }
        if (action.outer.udp.dest) {
            args.push_back(new ArgUInt16(action.outer.udp.dest));
        }
        if (action.encap.tun.vxlan_tun_id) {
            args.push_back(new ArgUInt32(action.encap.tun.vxlan_tun_id));
        }
    }

    void printArgs() const {
        std::cout << "=== Arguments ===" << std::endl;
        for (const auto& arg : args) {
            if (arg) {
                arg->print(); // Assuming each ArgBase has a print method
            } else {
                std::cout << "Null argument detected." << std::endl;
            }
        }
    }

    void print() const {
        // Print flow_match
        std::cout << "=== Flow Match ===" << std::endl;
        printMeta(match.meta);
        printEthernet(match.outer.eth);
        printIP4(match.outer.ip4);
        printL4(match.outer.l4_type_ext, match.outer);

        // Print flow_actions
        std::cout << "=== Flow Actions ===" << std::endl;
        printMeta(action.meta);
        printEthernet(action.outer.eth);
        printIP4(action.outer.ip4);
        printL4(action.outer.l4_type_ext, action.outer);

        if (action.has_encap) {
            std::cout << "Encapsulation Tunnel ID: " << action.encap.tun.vxlan_tun_id << std::endl;
        }

        // Print flow_fwd
        std::cout << "=== Flow Forwarding ===" << std::endl;
        std::cout << "Forwarding Type: " << fwd.type << std::endl;
    }

    void printMeta(const flow_meta &meta) const {
        std::cout << "Meta Packet: " << meta.pkt_meta << std::endl;
        std::cout << "Meta u32: ";
        for (int i = 0; i < 4; ++i) {
            std::cout << meta.u32[i] << " ";
        }
        std::cout << std::endl;
    }

    void printEthernet(const flow_header_eth &eth) const {
        std::cout << "Ethernet Source: ";
        printMac(eth.h_source);
        std::cout << "Ethernet Destination: ";
        printMac(eth.h_dest);
        std::cout << "Ethernet Protocol: 0x" << std::hex << eth.h_proto << std::dec << std::endl;
    }

    void printIP4(const flow_header_ip4 &ip4) const {
        char src[INET_ADDRSTRLEN];
        char dst[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ip4.saddr), src, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip4.daddr), dst, INET_ADDRSTRLEN);

        std::cout << "IP4 Source: " << src << std::endl;
        std::cout << "IP4 Destination: " << dst << std::endl;
        std::cout << "IP4 Protocol: " << static_cast<int>(ip4.protocol) << std::endl;
    }

    void printL4(flow_l4_type_ext l4_type_ext, const flow_header_format &header) const {
        if (l4_type_ext == FLOW_L4_TYPE_EXT_TCP) {
            const flow_header_tcp &tcp = header.tcp;
            std::cout << "TCP Source Port: " << ntohs(tcp.source) << std::endl;
            std::cout << "TCP Destination Port: " << ntohs(tcp.dest) << std::endl;
        } else if (l4_type_ext == FLOW_L4_TYPE_EXT_UDP) {
            const flow_header_udp &udp = header.udp;
            std::cout << "UDP Source Port: " << ntohs(udp.source) << std::endl;
            std::cout << "UDP Destination Port: " << ntohs(udp.dest) << std::endl;
        } else {
            std::cout << "L4 Type: None" << std::endl;
        }
    }

    void printMac(const uint8_t mac[6]) const {
        std::cout << std::hex << std::setw(2) << std::setfill('0');
        for (int i = 0; i < 6; ++i) {
            std::cout << static_cast<int>(mac[i]);
            if (i != 5) std::cout << ":";
        }
        std::cout << std::dec << std::endl;
    }
};

#define MAX_ENTRIES 128

class TablePipe : public Pipe {
public:
    std::string name;
    std::vector<TableEntry> entries;
    struct flow_match match;
    std::vector<int> actionIds;
    struct flow_pipe *defaultNext;

    TablePipe(const std::string& name, struct flow_pipe * defaultNext)
        : name(name), defaultNext(defaultNext) {
            memset(&match, 0, sizeof(match));
        }

    void print() const {
        std::cout << "Table Name: " << name << std::endl;
        std::cout << "Action IDs: ";
        for (const auto& actionId : actionIds) {
            std::cout << actionId << " ";
        }
        std::cout << std::endl;
    }
};

class ActionPipe : public Pipe {
public:
    std::string name;

    ActionPipe(const std::string& name) : name(name) {}

    void execute() const {
        // Logic for executing the action
        std::cout << "Executing Action: " << name << std::endl;
    }

    void print() const override {
        std::cout << "Action Name: " << name << std::endl;
    }
};

extern "C" int fsm_create_table_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_pipe* pipe);
extern "C" int fsm_create_conditional_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_pipe* pipe);
extern "C" struct flow_pipe* fsm_table_pipe_match(struct flow_pipe* pipe, struct flow_match* match, struct sk_buff* skb);

#endif  /* _STATEMACHINE_INTERNAL_H_ */