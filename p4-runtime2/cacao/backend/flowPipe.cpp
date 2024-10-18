#include <linux/if_ether.h>
#include <linux/udp.h>

#include "net/dpdk.h"
#include "backend/flowPipe.h"
#include "backend/flowPipeInternal.h"
#include "backend/doca.h"
#include "backend/stateMachine.h"
#include "backend/stateMachineInternal.h"

static std::vector<std::unique_ptr<LLJIT>> GlobalJITs;

extern "C" struct flow_pipe* flow_get_pipe(char* pipe_name) {
    struct flow_pipe * pipe = NULL;
    for (auto p : pipelines) {
        pipe = p->lookupFlowPipe(pipe_name);
        if (pipe) {
            // printf("[%s:%d] Pipe %s in pipeline %s has id %u\n", __func__, __LINE__, pipe_name, p->Name.c_str(), pipe->id);
            return pipe;
        }
    }
    return NULL;
}

extern "C" int flow_get_pipe_id_by_name(char* pipe_name) {
    struct flow_pipe * pipe = NULL;
    for (auto p : pipelines) {
        pipe = p->lookupFlowPipe(pipe_name);
        if (pipe) {
            // printf("[%s:%d] Pipe %s in pipeline %s has id %u\n", __func__, __LINE__, pipe_name, p->Name.c_str(), pipe->id);
            return pipe->id;
        }
    }
    return 0;
}

extern "C" int flow_create_hw_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss, struct flow_pipe* pipe) {
    if (pipe_cfg->attr.type == FLOW_PIPE_CONTROL) {
        return doca_create_hw_control_pipe(pipe, pipe_cfg, fwd, fwd_miss);
    }
    return doca_create_hw_pipe(pipe, pipe_cfg, fwd, fwd_miss);
}

extern "C" int flow_hw_control_pipe_add_entry(struct flow_pipe* pipe, uint8_t priority, struct flow_match* match, struct flow_actions* action, struct flow_fwd* fwd) {
    return doca_hw_control_pipe_add_entry(pipe, priority, match, action, fwd);
}

extern "C" int flow_hw_pipe_add_entry(struct flow_pipe* pipe, struct flow_match* match, struct flow_actions* actions, struct flow_monitor* monitor, struct flow_fwd* fwd) {
    return doca_hw_pipe_add_entry(pipe, match, actions, monitor, fwd);
}

extern "C" uint8_t * get_packet(struct sk_buff* skb) {
    return get_packet_internal(skb);
}

extern "C" uint32_t get_packet_size(struct sk_buff* skb) {
    return get_packet_size_internal(skb);
}

extern "C" void set_packet_meta(struct sk_buff* skb, uint32_t meta) {
    return set_packet_meta_internal(skb, meta);
}

extern "C" struct sk_buff* prepend_packet(struct sk_buff* skb, uint32_t prepend_size) {
    return prepend_packet_internal(skb, prepend_size);
}

extern "C" int create_fsm_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_pipe* pipe) {
    if (pipe_cfg->attr.type == FLOW_PIPE_BASIC) {
        printf("Creating table pipe...\n");
        return fsm_create_table_pipe(pipe_cfg, fwd, pipe);
    } else if (pipe_cfg->attr.type == FLOW_PIPE_CONTROL) {
        printf("Creating conditional pipe...\n");
        return fsm_create_conditional_pipe(pipe_cfg, fwd, pipe);
    }
    return 0;
}

extern "C" int flow_pipe_query_entry(struct flow_pipe* pipe) {
    return doca_hw_pipe_query_entry(pipe);
}

// extern "C" struct flow_pipe* fsm_table_lookup(struct flow_pipe* pipe, struct sk_buff* skb) {
//     // return fsm_table_pipe_match(pipe, match, skb);
//     auto tablePipe = static_cast<TablePipe*>(pipe->swPipe.pipe);
//     // for (auto entry : tablePipe->entries) {
//     for (int i = 0; i < tablePipe->num_entries; i++) {
//         auto entry = tablePipe->entries[i];
//         if (fsm_table_lookup_internal(&tablePipe->match, &entry.match, skb)) {
//             // printf("Checking entry...\n");
//             // entry.print();
//             struct ethhdr * ethhdr = (struct ethhdr *)get_packet_internal(skb);
//             struct iphdr * iphdr = (struct iphdr *)&ethhdr[1];
//             struct udphdr * udphdr = (struct udphdr *)&iphdr[1];
//             struct flow_actions zeroAction = {{0}};
//             uint8_t eth_addr[6] = {0};
//             if (memcmp(&entry.action, &zeroAction, sizeof(struct flow_actions))!=0) {
//                 std::vector<ArgBase*> argList;
//                 if (memcmp(entry.action.outer.eth.h_dest, &eth_addr, 6)!=0) {
//                     std::array<uint8_t, 6> mac_dest;
//                     std::copy(entry.action.outer.eth.h_dest, entry.action.outer.eth.h_dest + 6, mac_dest.begin());
//                     ArgMac *h_dest_mac = new ArgMac(mac_dest);
//                     argList.push_back(h_dest_mac);
//                 }
//                 if (memcmp(entry.action.outer.eth.h_source, &eth_addr, 6)!=0) {
//                     std::array<uint8_t, 6> mac_source;
//                     std::copy(entry.action.outer.eth.h_source, entry.action.outer.eth.h_source + 6, mac_source.begin());
//                     ArgMac *h_source_mac = new ArgMac(mac_source);
//                     argList.push_back(h_source_mac);
//                 }
//                 if (entry.action.outer.ip4.saddr) {
//                     ArgUInt32 * saddr = new ArgUInt32(entry.action.outer.ip4.saddr);
//                     argList.push_back(saddr);
//                 }
//                 if (entry.action.outer.ip4.daddr) {
//                     ArgUInt32 * daddr = new ArgUInt32(entry.action.outer.ip4.daddr);
//                     argList.push_back(daddr);
//                 }
//                 if (entry.action.outer.udp.source) {
//                     // ArgUInt16 * source = new ArgUInt16(entry.action.outer.udp.source);
//                     ArgUInt16 * source = new ArgUInt16(udphdr->source);
//                     argList.push_back(source);
//                 }
//                 if (entry.action.outer.udp.dest) {
//                     // ArgUInt16 * dest = new ArgUInt16(entry.action.outer.udp.dest);
//                     ArgUInt16 * dest = new ArgUInt16(udphdr->dest);
//                     argList.push_back(dest);
//                 }
//                 if (entry.action.encap.tun.vxlan_tun_id) {
//                     ArgUInt32 * vni = new ArgUInt32(entry.action.encap.tun.vxlan_tun_id);
//                     argList.push_back(vni);
//                 }

//                 if (argList.size() == 7) { // 6 MAC/IP/UDP arguments + 1 VNI
//                 auto *h_dest_mac = static_cast<ArgMac*>(argList[0]);
//                 auto *h_source_mac = static_cast<ArgMac*>(argList[1]);
//                     pipe->swPipe.ops.run(
//                         pipe, 
//                         skb, 
//                         h_dest_mac->mac.data(),
//                         h_source_mac->mac.data(),      // Pass the pointer to the MAC array
//                         static_cast<ArgUInt32*>(argList[2])->value, // saddr
//                         static_cast<ArgUInt32*>(argList[3])->value, // daddr
//                         static_cast<ArgUInt16*>(argList[4])->value, // source
//                         static_cast<ArgUInt16*>(argList[5])->value, // dest
//                         static_cast<ArgUInt32*>(argList[6])->value  // vni
//                     );
//                 } else {
//                     std::cerr << "Unexpected number of arguments: " << argList.size() << std::endl;
//                 }
//             }

//             if (entry.fwd.type == FLOW_FWD_PIPE) {
//                 return entry.fwd.next_pipe;
//             } else if (entry.fwd.type == FLOW_FWD_PORT) {
//                 return NULL;
//             }
//         }
//     }

//     return (tablePipe->defaultNext)? tablePipe->defaultNext : NULL;
// }
extern "C" struct flow_pipe* fsm_table_lookup(struct flow_pipe* pipe, struct sk_buff* skb) {
    auto tablePipe = static_cast<TablePipe*>(pipe->swPipe.pipe);

    for (auto entry : tablePipe->entries) {
        bool lookup = fsm_table_lookup_internal(&tablePipe->match, &entry.match, skb);
        if (lookup) {
            // Create a vector to hold the converted arguments
            std::vector<uint64_t> convertedArgs;

            if (entry.args.size() > 0) {
                // Iterate through each argument and check its type
                for (size_t j = 0; j < entry.args.size(); j++) {
                    if (entry.args[j] == nullptr) {
                        std::cerr << "Error: Argument " << j << " is null." << std::endl;
                        convertedArgs.clear(); // Clear the list as we will not call run
                        break;
                    }

                    switch (entry.args[j]->getType()) {
                        case ArgType::MAC: {
                            auto* arg_mac = static_cast<ArgMac*>(entry.args[j]);
                            // Store the address of the MAC array
                            convertedArgs.push_back(reinterpret_cast<uint64_t>(arg_mac->mac.data()));
                            break;
                        }
                        case ArgType::UINT32: {
                            auto* arg_uint32 = static_cast<ArgUInt32*>(entry.args[j]);
                            convertedArgs.push_back(static_cast<uint64_t>(arg_uint32->value));
                            break;
                        }
                        case ArgType::UINT16: {
                            auto* arg_uint16 = static_cast<ArgUInt16*>(entry.args[j]);
                            convertedArgs.push_back(static_cast<uint64_t>(arg_uint16->value));
                            break;
                        }
                        default: {
                            std::cerr << "Error: Argument " << j << " is of an unsupported type." << std::endl;
                            convertedArgs.clear(); // Clear the list as we will not call run
                            break;
                        }
                    }
                }

                if (entry.monitor.flags & FLOW_MONITOR_COUNT) {
                    entry.count++;
                }

                // Check if we have enough valid arguments to call run
                if (convertedArgs.size() == 1) {
                    pipe->swPipe.ops.run(
                        pipe, 
                        skb, 
                        convertedArgs[0]
                    );
                } else if (convertedArgs.size() == 7) {
                    // Call the run method with the converted arguments
                    pipe->swPipe.ops.run(
                        pipe, 
                        skb, 
                        convertedArgs[0], // h_dest_mac
                        convertedArgs[1], // h_source_mac
                        convertedArgs[2], // saddr
                        convertedArgs[3], // daddr
                        convertedArgs[4], // source
                        convertedArgs[5], // dest
                        convertedArgs[6]  // vni
                    );
                } else {
                    std::cerr << "Error: Not enough valid arguments in entry." << std::endl;
                }
            }

            // Forwarding logic
            if (entry.fwd.type == FLOW_FWD_PIPE) {
                return entry.fwd.next_pipe;
            } else if (entry.fwd.type == FLOW_FWD_PORT) {
                return NULL;
            }
        }
    }

    return (tablePipe->defaultNext) ? tablePipe->defaultNext : NULL;
}


extern "C" int fsm_table_add_entry(struct flow_pipe* pipe, struct flow_match* match, struct flow_actions* action, struct flow_monitor* monitor, struct flow_fwd* fwd) {
    auto tablePipe = static_cast<TablePipe*>(pipe->swPipe.pipe);

    TableEntry entry(match,action,monitor,fwd);

    pthread_rwlock_wrlock(&fsm_lock);
    tablePipe->entries.push_back(entry);
    pthread_rwlock_unlock(&fsm_lock);

    // entry.printArgs();

    return 0;
}

Pipeline::Pipeline(const std::string& name, const std::string& configName) : Name(name) {
    auto J = cantFail(LLJITBuilder().create());
    JitEnv = std::move(J);

    if (!JitEnv) {
        std::cerr << "Error: JitEnv is null after creation" << std::endl;
        return;
    }

    std::cout << "Getting main JIT DyLib..." << std::endl;
    auto &JD = JitEnv->getMainJITDylib();

    std::cout << "Create Symbols..." << std::endl;

    auto PrintfSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&printf));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("printf"), PrintfSymbol}})));

    auto FlowGetPipeSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_get_pipe));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_get_pipe"), FlowGetPipeSymbol}})));

    auto FlowGetPipeIdByNameSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_get_pipe_id_by_name));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_get_pipe_id_by_name"), FlowGetPipeIdByNameSymbol}})));

    auto FlowCreateHwPipeSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_create_hw_pipe));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_create_hw_pipe"), FlowCreateHwPipeSymbol}})));

    auto FlowControlPipeAddEntrySymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_hw_control_pipe_add_entry));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_hw_control_pipe_add_entry"), FlowControlPipeAddEntrySymbol}})));

    auto FlowPipeAddEntrySymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_hw_pipe_add_entry));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_hw_pipe_add_entry"), FlowPipeAddEntrySymbol}})));

    auto CreateFsmPipeSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&create_fsm_pipe));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("create_fsm_pipe"), CreateFsmPipeSymbol}})));

    auto PrependPacketSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&prepend_packet));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("prepend_packet"), PrependPacketSymbol}})));

    auto GetPacketSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&get_packet));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("get_packet"), GetPacketSymbol}})));

    auto GetPacketSizeSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&get_packet_size));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("get_packet_size"), GetPacketSizeSymbol}})));

    auto SetPacketMetaSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&set_packet_meta));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("set_packet_meta"), SetPacketMetaSymbol}})));

    auto FsmTableLookupSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&fsm_table_lookup));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("fsm_table_lookup"), FsmTableLookupSymbol}})));

    auto FsmTableAddEntrySymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&fsm_table_add_entry));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("fsm_table_add_entry"), FsmTableAddEntrySymbol}})));

    auto ntohsSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&ntohs));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("ntohs"), ntohsSymbol}})));

    auto htonsSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&htons));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("htons"), htonsSymbol}})));

    auto htonlSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&htonl));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("htonl"), htonlSymbol}})));

    auto ObjBuffer = MemoryBuffer::getFile(configName);
    if (!ObjBuffer) {
        errs() << "Error reading bitcode file\n";
    }

    // LLVMContext Context;
    Context = std::make_unique<LLVMContext>();
    auto ModuleOrErr = parseBitcodeFile(ObjBuffer->get()->getMemBufferRef(), *Context);
    if (!ModuleOrErr) {
        errs() << "Error parsing bitcode file\n";
    }

    TSM = std::make_unique<ThreadSafeModule>(std::move(*ModuleOrErr), std::make_unique<LLVMContext>());
    cantFail(JitEnv->addIRModule(std::move(*TSM)));

    std::cout << "JIT Initialization Complete!" << std::endl;
}