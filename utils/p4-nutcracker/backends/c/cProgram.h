#ifndef _BACKENDS_C_CPROGRAM_H_
#define _BACKENDS_C_CPROGRAM_H_

#include <filesystem>
#include <nlohmann/json.hpp>

#include "CallGraph/CallGraph.h"
#include "CallGraph/Pipeline.h"
#include "AST/ASTContext.h"
#include "ADT/APInt.h"
#include "backends/codeGen.h"

using json = nlohmann::json;

#define CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, VarName, VarNameStr, VarType)                 \
    IdentifierInfo *VarName##Id = &ctx.getIdentifierTable()->get(VarNameStr);                     \
    ParmVarDecl *VarName##Decl = ParmVarDecl::Create(ctx, FuncDecl, VarName##Id, VarType, SC_None);  \
    DeclRefExpr *VarName##Ref = DeclRefExpr::Create(ctx, VarName##Decl, VarName##Id, VarType, ExprValueKind::VK_LValue);

#define DECLARE_FUNCTION_WITH_STORAGE(ctx, FuncName, FuncNameStr, ReturnType, StorageClass, ...)   \
    QualType FuncName##RetType = QualType(ReturnType);                                             \
    std::vector<QualType> FuncName##ArgsType = { __VA_ARGS__ };                                    \
    QualType FuncName##Type = ctx.getFunctionType(FuncName##RetType, FuncName##ArgsType);          \
    IdentifierInfo *FuncName##Id = &ctx.getIdentifierTable()->get(FuncNameStr);                    \
    FunctionDecl *FuncName##Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(),          \
                                                        FuncName##Id, FuncName##Type, StorageClass);

#define SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl, ...)                                  \
    std::vector<ParmVarDecl *> FuncDecls = { __VA_ARGS__ };                              \
    FuncDecl->setParams(ctx, FuncDecls);                                                 \
    ctx.getTranslationUnitDecl()->addDecl(FuncDecl);

// Macro for normal (non-extern) function declaration
#define DECLARE_FUNC(ctx, FuncName, FuncNameStr, ReturnType, ...)   \
    DECLARE_FUNCTION_WITH_STORAGE(ctx, FuncName, FuncNameStr, ReturnType, SC_None, __VA_ARGS__)

// Macro for extern function declaration
#define DECLARE_EXTERN_FUNC(ctx, FuncName, FuncNameStr, ReturnType, ...)   \
    DECLARE_FUNCTION_WITH_STORAGE(ctx, FuncName, FuncNameStr, ReturnType, SC_Extern, __VA_ARGS__)   \
    DeclRefExpr *FuncName##Ref = DeclRefExpr::Create(ctx, FuncName##Decl, FuncName##Id,             \
                                                     FuncName##Type, ExprValueKind::VK_PRValue);

#define CALL_FUNC(ctx, FuncDecl, FuncName, ...)                                        \
    std::vector<Expr*> FuncName##Args = { __VA_ARGS__ };                                    \
    CallExpr *FuncName##Call = CallExpr::Create(ctx, FuncName##Ref, FuncName##Args, FuncDecl->getReturnType(), ExprValueKind::VK_PRValue);

#define CALL_FUNC_AND_STMT(ctx, FuncDecl, FuncName, Stmt, ...)             \
    {                                                                           \
        CALL_FUNC(ctx, FuncDecl, FuncName, __VA_ARGS__);                   \
        Stmt.push_back(FuncName##Call);                                        \
    }

#define CALL_FUNC_WITH_REF(ctx, Func, FuncName, ...)                                        \
    std::vector<Expr*> FuncName##Args = { __VA_ARGS__ };                                    \
    IdentifierInfo *FuncName##Id = const_cast<IdentifierInfo*>((Func).Decl->getIdentifier());                \
    DeclRefExpr *FuncName##Ref = DeclRefExpr::Create(ctx, (Func).Decl, FuncName##Id, (Func).Type, ExprValueKind::VK_PRValue);   \
    CallExpr *FuncName##Call = CallExpr::Create(ctx, FuncName##Ref, FuncName##Args, (Func).Decl->getReturnType(), ExprValueKind::VK_PRValue);

#define CALL_FUNC_WITH_REF_AND_STMT(ctx, Func, FuncName, Stmt, ...)            \
    {                                                                                       \
        CALL_FUNC_WITH_REF(ctx, Func, FuncName, __VA_ARGS__);                          \
        Stmt.push_back(FuncName##Call);                                                    \
    }

#define VAR(name)       (name##Var)
#define VARREF(name)    (name##VarRef)
#define DECL(name)      (name##Decl)
#define REF(name)       (name##Ref)
#define CALL(name)      (name##Call)

#define DECLARE_VAR_AND_STMT(ctx, FuncDecl, VarName, VarNameStr, VarType, Stmts) \
    IdentifierInfo *VarName##Id = &ctx.getIdentifierTable()->get(VarNameStr);       \
    VarDecl *VarName##Var = VarDecl::Create(ctx, FuncDecl, VarName##Id, VarType, SC_None); \
    Stmt *VarName##Stmt = DeclStmt::Create(ctx, DeclGroupRef(VarName##Var));      \
    DeclRefExpr *VarName##VarRef = DeclRefExpr::Create(ctx, VarName##Var, VarName##Id, VarType, ExprValueKind::VK_LValue); \
    Stmts.push_back(VarName##Stmt);

class CProgram {
    friend class ASTContext;
public:
    const CallGraph& callGraph;
    json j;

    bool buildHeaderStructure();

    bool buildPipe(Pipeline::Kind K, const CallGraphNodeBase* node, bool isRoot);
    bool initPipe(Pipeline::Kind K, const CallGraphNodeBase* node);

    bool buildActionPipe(Pipeline::Kind K, const ActionNode* actionNode);

    bool buildConditionalPipe(Pipeline::Kind K, const ConditionalNode* conditionalNode, bool isRoot);
    bool initConditionalPipe(const ConditionalNode* conditionalNode);

    bool buildTablePipe(Pipeline::Kind K, const TableNode* tableNode, bool isRoot);
    bool initTablePipe(const TableNode* tableNode);

    bool buildAddActionPipeEntry(const ActionNode* actionNode);
    bool buildAddTablePipeEntry(const TableNode* tableNode);

    bool build();

    void processExpression(const std::shared_ptr<Expression>& expr, std::vector<Stmt*>& Stmts,  DeclRefExpr* MatchVarRef,  bool *ipProtocolSet, bool *transportProtocolSet, bool *tunnelSet);

    CProgram(const CallGraph& callGraph) : callGraph(callGraph),  ctx() {}

    void emit(void) { ctx.emit(); }

    void emitGeneratedComment(CodeBuilder *builder);
    void emitH(CodeBuilder *builder, const std::filesystem::path &headerFile);  // emits C headers
    void emitC(CodeBuilder *builder, const std::filesystem::path &headerFile);  // emits C program

    ASTContext ctx;

    class MetaDecl {
        friend class CProgram;

        RecordDecl          * Decl;
        RecordType          * Type;

        FieldDecl           * PktMeta;
        FieldDecl           * U32;
    };

    class EthhdrDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Dest;
        FieldDecl *Src;
        FieldDecl *Proto;
    };

    class L3TypeDecl {
        friend class CProgram;
        EnumDecl * Decl;
        EnumType * Type;
        EnumConstantDecl * TypeNone;
        EnumConstantDecl * TypeIpv4;
    };

    class IphdrDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Version;
        FieldDecl *Ihl;
        FieldDecl *Tos;
        FieldDecl *TotLen;
        FieldDecl *Id;
        FieldDecl *FragOff;
        FieldDecl *Ttl;
        FieldDecl *Protocol;
        FieldDecl *Check;
        FieldDecl *Saddr;
        FieldDecl *Daddr;
    };

    class L4TypeExtDecl {
        friend class CProgram;
        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * ExtNone;
        EnumConstantDecl * ExtTcp;
        EnumConstantDecl * ExtUdp;
    };

    class UdphdrDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Source;
        FieldDecl *Dest;
        FieldDecl *Len;
        FieldDecl *Check;
    };

    class TdphdrDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Source;
        FieldDecl *Dest;
        FieldDecl *Seq;
        FieldDecl *AckSeq;
        FieldDecl *Doff;
        FieldDecl *Res1;
        FieldDecl *Flags;
        FieldDecl *Window;
        FieldDecl *Check;
        FieldDecl *UrgPtr;
    };

    class L4HeaderDecl {
        friend class CProgram;
        friend class UdphdrDecl;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Udp;
        FieldDecl *Tcp;
    };

    class TunTypeDecl {
        friend class CProgram;
        EnumDecl * Decl;
        EnumType * Type;
        EnumConstantDecl * TypeNone;
        EnumConstantDecl * TypeVxlan;
    };

    class TunDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *TunType;
        FieldDecl *VxlanTunId;
    };

    class FormatDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Eth;
        FieldDecl   *EthValid;
        FieldDecl   *L3Type;
        FieldDecl   *Ip;
        FieldDecl   *L4TypeExt;
        FieldDecl   *Udp;
    };

    class MatchDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Meta;
        FieldDecl   *Outer;
        FieldDecl   *Inner;
    };

    class EncapActionDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Outer;
        FieldDecl   *Tun;
    };

    class ActionsDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Meta;
        FieldDecl   *Outer;
        FieldDecl   *HasEncap;
        FieldDecl   *Encap;
    };

    class FwdTypeDecl {
        friend class CProgram;

        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * FwdNone;
        EnumConstantDecl * FwdRss;
        EnumConstantDecl * FwdHairpin;
        EnumConstantDecl * FwdPort;
        EnumConstantDecl * FwdPipe;
        EnumConstantDecl * FwdDrop;
    };

    class FwdDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *FwdType;
        FieldDecl   *FwdNextPipe;
    };

    class PipeTypeDecl {
        friend class CProgram;

        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * Basic;
        EnumConstantDecl * Control;
    };

    class PipeDomainDecl {
        friend class CProgram;

        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * Ingress;
        EnumConstantDecl * Egress;
    };

    class PipeAttrDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Name;
        FieldDecl   *PipeType;
        FieldDecl   *Domain;
        FieldDecl   *IsRoot;
        FieldDecl   *NbActions;
    };

    class PipeCfgDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Attr;
        FieldDecl   *Match;
        FieldDecl   *Actions;
    };

    class PipeDecl {
        friend class CProgram;

        RecordDecl * Decl;
        RecordType * Type;
    };

    class GetPipeDecl {
        friend class CProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class GetPipeIdDecl {
        friend class CProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class CreatePipeDecl {
        friend class CProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class ControlPipeAddEntryDecl {
        friend class CProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class PipeAddEntryDecl {
        friend class CProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class MatchStateMachine {
        friend class CProgram;

        enum class State {
            Start,        // Initial state
            NestedHeaderParsed,
            HeaderParsed, // Header identified
            FieldParsed,  // Field identified
            Success,      // Successfully parsed
            Error         // Error in parsing
        };

        MatchStateMachine() : currentState(State::Start) {}

        MemberExpr * parseTarget(CProgram *cProgram, std::vector<Stmt *> &Stmts, const std::vector<std::string>& target, 
                DeclRefExpr *MatchVarRef, QualType *MatchFieldType, bool *ipProtocolSet, bool *transportProtocolSet, bool *tunnelSet) {
            currentState = State::Start;
            MemberExpr *expr = nullptr;

            for (const auto& component : target) {
                switch (currentState) {
                    case State::Start:
                        if (isValidHeader(component)) {
                            currentState = State::HeaderParsed;
                            currentHeader = component;
                            // std::cout << "Checking header " << currentHeader << std::endl;
                            if (currentHeader == "eth_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowMatch.Outer, cProgram->FlowMatch.Outer->getDeclName(), cProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.Eth, cProgram->FlowFormat.Eth->getDeclName(), cProgram->FlowFormat.Eth->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                            } else if (currentHeader == "ipv4_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowMatch.Outer, cProgram->FlowMatch.Outer->getDeclName(), cProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.Ip, cProgram->FlowFormat.Ip->getDeclName(), cProgram->FlowFormat.Ip->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*ipProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowMatch.Outer, cProgram->FlowMatch.Outer->getDeclName(), cProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.L3Type, cProgram->FlowFormat.L3Type->getDeclName(), cProgram->FlowFormat.L3Type->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L3TypeIpv4Id = const_cast<IdentifierInfo*>(cProgram->FlowL3Type.TypeIpv4->getIdentifier());
                                    DeclRefExpr *L3TypeIpv4Ref = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowL3Type.TypeIpv4, L3TypeIpv4Id, QualType(cProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L3TypeAssign = BinaryOperator::Create(cProgram->ctx, expr, L3TypeIpv4Ref, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L3TypeAssign);
                                    *ipProtocolSet = true;
                                }
                            } else if (currentHeader == "udp_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowMatch.Outer, cProgram->FlowMatch.Outer->getDeclName(), cProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowL4Header.Udp, cProgram->FlowL4Header.Udp->getDeclName(), cProgram->FlowL4Header.Udp->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*transportProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowMatch.Outer, cProgram->FlowMatch.Outer->getDeclName(), cProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.L4TypeExt, cProgram->FlowFormat.L4TypeExt->getDeclName(), cProgram->FlowFormat.L4TypeExt->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L4TypeExtUdpId = const_cast<IdentifierInfo*>(cProgram->FlowL4TypeExt.ExtUdp->getIdentifier());
                                    DeclRefExpr *L4TypeExtUdpRef = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowL4TypeExt.ExtUdp, L4TypeExtUdpId, QualType(cProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L4TypeExtAssign = BinaryOperator::Create(cProgram->ctx, expr, L4TypeExtUdpRef, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L4TypeExtAssign);
                                    *transportProtocolSet = true;
                                }
                                if (!(*ipProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowMatch.Outer, cProgram->FlowMatch.Outer->getDeclName(), cProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.L3Type, cProgram->FlowFormat.L3Type->getDeclName(), cProgram->FlowFormat.L3Type->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L3TypeIpv4Id = const_cast<IdentifierInfo*>(cProgram->FlowL3Type.TypeIpv4->getIdentifier());
                                    DeclRefExpr *L3TypeIpv4Ref = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowL3Type.TypeIpv4, L3TypeIpv4Id, QualType(cProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L3TypeAssign = BinaryOperator::Create(cProgram->ctx, expr, L3TypeIpv4Ref, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L3TypeAssign);
                                    *ipProtocolSet = true;
                                }
                            } else {
                                std::cerr << "Unknown header" << std::endl;
                            }
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::HeaderParsed:
                        // Check if it's a nested header or a field
                        if (isValidHeader(component)) {
                            currentState = State::NestedHeaderParsed;
                            currentHeader = component;
                            // std::cout << "Checking header " << currentHeader << std::endl;
                        } else if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                            currentField = component;  // Save field name
                            // std::cout << "Checking field " << currentField << std::endl;
                            if (currentHeader == "eth_hdr") {
                                if (currentField == "h_dest") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowEthhdr.Dest->getDeclName(), cProgram->FlowEthhdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowEthhdr.Src->getDeclName(), cProgram->FlowEthhdr.Src->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowIphdr.Daddr->getDeclName(), cProgram->FlowIphdr.Daddr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "protocol") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowIphdr.Protocol->getDeclName(), cProgram->FlowIphdr.Protocol->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowUdphdr.Dest->getDeclName(), cProgram->FlowUdphdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowUdphdr.Dest->getType();
                                }
                            }
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::NestedHeaderParsed:
                        if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                            currentField = component;
                            std::cout << "Checking field " << currentField << std::endl;

                        } else {
                            currentState = State::Error;
                        }
                        break;

                    default:
                        currentState = State::Error;
                }

                if (currentState == State::Error) {
                    std::cerr << "Error: Invalid target field: " << component << std::endl;
                    break;
                }
            }

            currentState = State::Success;
            // std::cout << "Successfully parsed: " << currentHeader << "." << currentField << std::endl;
            // if (expr != nullptr) std::cout << "Built stmt: " << expr->emitStmt() << std::endl;
            return expr;
        }

        QualType getLastFieldTypeAndName(CProgram *cProgram, const std::vector<std::string>& target) {
            currentState = State::Start;
            QualType lastFieldType;

            for (const auto& component : target) {
                switch (currentState) {
                    case State::Start:
                        if (isValidHeader(component)) {
                            currentState = State::HeaderParsed;
                            currentHeader = component;
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::HeaderParsed:
                        // Check if it's a nested header or a field
                        if (isValidHeader(component)) {
                            currentState = State::NestedHeaderParsed;
                            currentHeader = component;
                        } else if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                            currentField = component;  // Save the last field name

                            // Determine the field type based on the header
                            if (currentHeader == "eth_hdr") {
                                if (currentField == "h_dest") {
                                    lastFieldType = cProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    lastFieldType = cProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    lastFieldType = cProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "saddr") {
                                    lastFieldType = cProgram->FlowIphdr.Saddr->getType();
                                } else if (currentField == "protocol") {
                                    lastFieldType = cProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    lastFieldType = cProgram->FlowUdphdr.Dest->getType();
                                } else if (currentField == "source") {
                                    lastFieldType = cProgram->FlowUdphdr.Source->getType();
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                if (currentField == "vni") {
                                    lastFieldType = cProgram->FlowTun.VxlanTunId->getType();
                                }
                            }
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::NestedHeaderParsed:
                        if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    default:
                        currentState = State::Error;
                }

                if (currentState == State::Error) {
                    std::cerr << "Error: Invalid target field: " << component << std::endl;
                    break;
                }
            }

            currentState = State::Success;
            return lastFieldType;
        }

        State currentState;
        std::string currentHeader;
        std::string currentField;

        // Header map to simulate available headers and fields
        const std::unordered_map<std::string, std::vector<std::string>> headerFields = {
            // Ethernet header
            {"eth_hdr", {"h_dest", "h_source", "h_proto"}},

            // Outer IPv4 and UDP headers
            {"outer_ipv4_hdr", {"saddr", "daddr", "ttl", "protocol"}},
            {"outer_udp_hdr", {"source", "dest", "len", "check"}},

            // Inner IPv4 and UDP headers (in case of encapsulation, like VXLAN)
            {"inner_ipv4_hdr", {"saddr", "daddr", "ttl", "protocol"}},
            {"inner_udp_hdr", {"source", "dest", "len", "check"}},

            // VXLAN header fields
            {"vxlan_hdr", {"vni"}},

            // Generic UDP and IPv4 headers
            {"udp_hdr", {"source", "dest", "len", "check"}},
            {"ipv4_hdr", {"saddr", "daddr", "ttl", "protocol"}}
        };

        bool isValidHeader(const std::string& header) {
            return headerFields.find(header) != headerFields.end();
        }

        bool isValidField(const std::string& header, const std::string& field) {
            auto it = headerFields.find(header);
            if (it != headerFields.end()) {
                const auto& fields = it->second;
                return std::find(fields.begin(), fields.end(), field) != fields.end();
            }
            std::cerr << "Field " << field << " is not valid in " << header << std::endl;
            return false;
        }
    };
    friend class MatchStateMachine;

    class ActionStateMachine {
        friend class CProgram;

        enum class State {
            Start,        // Initial state
            NestedHeaderParsed,
            HeaderParsed, // Header identified
            FieldParsed,  // Field identified
            Success,      // Successfully parsed
            Error         // Error in parsing
        };

        ActionStateMachine() : currentState(State::Start) {}

        MemberExpr * parseTarget(CProgram *cProgram, std::vector<Stmt *> &Stmts, const std::vector<std::string>& target, 
                DeclRefExpr *MatchVarRef, QualType *MatchFieldType, bool *ipProtocolSet, bool *transportProtocolSet, bool *tunnelSet) {
            currentState = State::Start;
            MemberExpr *expr = nullptr;

            for (const auto& component : target) {
                switch (currentState) {
                    case State::Start:
                        if (isValidHeader(component)) {
                            currentState = State::HeaderParsed;
                            currentHeader = component;
                            // std::cout << "Checking header " << currentHeader << std::endl;
                            if (currentHeader == "eth_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Outer, cProgram->FlowActions.Outer->getDeclName(), cProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.Eth, cProgram->FlowFormat.Eth->getDeclName(), cProgram->FlowFormat.Eth->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                            } else if (currentHeader == "ipv4_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Outer, cProgram->FlowActions.Outer->getDeclName(), cProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.Ip, cProgram->FlowFormat.Ip->getDeclName(), cProgram->FlowFormat.Ip->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*ipProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Outer, cProgram->FlowActions.Outer->getDeclName(), cProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.L3Type, cProgram->FlowFormat.L3Type->getDeclName(), cProgram->FlowFormat.L3Type->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L3TypeIpv4Id = const_cast<IdentifierInfo*>(cProgram->FlowL3Type.TypeIpv4->getIdentifier());
                                    DeclRefExpr *L3TypeIpv4Ref = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowL3Type.TypeIpv4, L3TypeIpv4Id, QualType(cProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L3TypeAssign = BinaryOperator::Create(cProgram->ctx, expr, L3TypeIpv4Ref, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L3TypeAssign);
                                    *ipProtocolSet = true;
                                }
                            } else if (currentHeader == "udp_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Outer, cProgram->FlowActions.Outer->getDeclName(), cProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowL4Header.Udp, cProgram->FlowL4Header.Udp->getDeclName(), cProgram->FlowL4Header.Udp->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*transportProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Outer, cProgram->FlowActions.Outer->getDeclName(), cProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowFormat.L4TypeExt, cProgram->FlowFormat.L4TypeExt->getDeclName(), cProgram->FlowFormat.L4TypeExt->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L4TypeExtUdpId = const_cast<IdentifierInfo*>(cProgram->FlowL4TypeExt.ExtUdp->getIdentifier());
                                    DeclRefExpr *L4TypeExtUdpRef = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowL4TypeExt.ExtUdp, L4TypeExtUdpId, QualType(cProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L4TypeExtAssign = BinaryOperator::Create(cProgram->ctx, expr, L4TypeExtUdpRef, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L4TypeExtAssign);
                                    *transportProtocolSet = true;
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Encap, cProgram->FlowActions.Encap->getDeclName(), cProgram->FlowActions.Encap->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowEncapAction.Tun, cProgram->FlowEncapAction.Tun->getDeclName(), cProgram->FlowEncapAction.Tun->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*tunnelSet)) {
                                    {
                                        MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.HasEncap, cProgram->FlowActions.HasEncap->getDeclName(), cProgram->FlowActions.HasEncap->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        // IdentifierInfo *ActionsHasEncapId = const_cast<IdentifierInfo*>(cProgram->FlowActions.HasEncap->getIdentifier());
                                        // DeclRefExpr *ActionsHasEncapRef = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowActions.HasEncap, ActionsHasEncapId, QualType(cProgram->ctx.BoolTy), ExprValueKind::VK_PRValue);
                                        IntegerLiteral *hasEncap = IntegerLiteral::Create(cProgram->ctx, APInt(1, 1), IntegerLiteral::Dec, QualType(cProgram->ctx.BoolTy));
                                        BinaryOperator *ActionsHasEncapAssign = BinaryOperator::Create(cProgram->ctx, expr, hasEncap, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                        Stmts.push_back(ActionsHasEncapAssign);
                                    }
                                    {
                                        MemberExpr *expr = MemberExpr::Create(cProgram->ctx, MatchVarRef, false, cProgram->FlowActions.Encap, cProgram->FlowActions.Encap->getDeclName(), cProgram->FlowActions.Encap->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowEncapAction.Tun, cProgram->FlowEncapAction.Tun->getDeclName(), cProgram->FlowEncapAction.Tun->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        expr = MemberExpr::Create(cProgram->ctx, expr, false, cProgram->FlowTun.TunType, cProgram->FlowTun.TunType->getDeclName(), cProgram->FlowTun.TunType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        IdentifierInfo *TunnelTypeVxlanId = const_cast<IdentifierInfo*>(cProgram->FlowTunType.TypeVxlan->getIdentifier());
                                        DeclRefExpr *TunnelTypeVxlanRef = DeclRefExpr::Create(cProgram->ctx, cProgram->FlowTunType.TypeVxlan, TunnelTypeVxlanId, QualType(cProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                        BinaryOperator *TunnelTypeAssign = BinaryOperator::Create(cProgram->ctx, expr, TunnelTypeVxlanRef, BO_Assign, QualType(cProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                        Stmts.push_back(TunnelTypeAssign);
                                    }
                                    *tunnelSet = true;
                                }
                            } else {
                                std::cerr << "Unknown header" << std::endl;
                            }
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::HeaderParsed:
                        // Check if it's a nested header or a field
                        if (isValidHeader(component)) {
                            currentState = State::NestedHeaderParsed;
                            currentHeader = component;
                            // std::cout << "Checking header " << currentHeader << std::endl;
                        } else if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                            currentField = component;  // Save field name
                            // std::cout << "Checking field " << currentField << std::endl;
                            if (currentHeader == "eth_hdr") {
                                if (currentField == "h_dest") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowEthhdr.Dest->getDeclName(), cProgram->FlowEthhdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowEthhdr.Src->getDeclName(), cProgram->FlowEthhdr.Src->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowIphdr.Daddr->getDeclName(), cProgram->FlowIphdr.Daddr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "saddr") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowIphdr.Saddr->getDeclName(), cProgram->FlowIphdr.Saddr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowIphdr.Saddr->getType();
                                } else if (currentField == "protocol") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowIphdr.Protocol->getDeclName(), cProgram->FlowIphdr.Protocol->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowUdphdr.Dest->getDeclName(), cProgram->FlowUdphdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowUdphdr.Dest->getType();
                                } else if (currentField == "source") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowUdphdr.Source->getDeclName(), cProgram->FlowUdphdr.Source->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowUdphdr.Source->getType();
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                if (currentField == "vni") {
                                    expr = MemberExpr::Create(cProgram->ctx, expr, false, nullptr, cProgram->FlowTun.VxlanTunId->getDeclName(), cProgram->FlowTun.VxlanTunId->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = cProgram->FlowTun.VxlanTunId->getType();
                                }
                            }
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::NestedHeaderParsed:
                        if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                            currentField = component;
                            std::cout << "Checking field " << currentField << std::endl;

                        } else {
                            currentState = State::Error;
                        }
                        break;

                    default:
                        currentState = State::Error;
                }

                if (currentState == State::Error) {
                    std::cerr << "Error: Invalid target field: " << component << std::endl;
                    break;
                }
            }

            currentState = State::Success;
            // std::cout << "Successfully parsed: " << currentHeader << "." << currentField << std::endl;
            // if (expr != nullptr) std::cout << "Built stmt: " << expr->emitStmt() << std::endl;
            return expr;
        }

        QualType getLastFieldTypeAndName(CProgram *cProgram, const std::vector<std::string>& target) {
            currentState = State::Start;
            QualType lastFieldType;

            for (const auto& component : target) {
                switch (currentState) {
                    case State::Start:
                        if (isValidHeader(component)) {
                            currentState = State::HeaderParsed;
                            currentHeader = component;
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::HeaderParsed:
                        // Check if it's a nested header or a field
                        if (isValidHeader(component)) {
                            currentState = State::NestedHeaderParsed;
                            currentHeader = component;
                        } else if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                            currentField = component;  // Save the last field name

                            // Determine the field type based on the header
                            if (currentHeader == "eth_hdr") {
                                if (currentField == "h_dest") {
                                    lastFieldType = cProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    lastFieldType = cProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    lastFieldType = cProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "saddr") {
                                    lastFieldType = cProgram->FlowIphdr.Saddr->getType();
                                } else if (currentField == "protocol") {
                                    lastFieldType = cProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    lastFieldType = cProgram->FlowUdphdr.Dest->getType();
                                } else if (currentField == "source") {
                                    lastFieldType = cProgram->FlowUdphdr.Source->getType();
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                if (currentField == "vni") {
                                    lastFieldType = cProgram->FlowTun.VxlanTunId->getType();
                                }
                            }
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    case State::NestedHeaderParsed:
                        if (isValidField(currentHeader, component)) {
                            currentState = State::FieldParsed;
                        } else {
                            currentState = State::Error;
                        }
                        break;

                    default:
                        currentState = State::Error;
                }

                if (currentState == State::Error) {
                    std::cerr << "Error: Invalid target field: " << component << std::endl;
                    break;
                }
            }

            currentState = State::Success;
            return lastFieldType;
        }

        State currentState;
        std::string currentHeader;
        std::string currentField;

        // Header map to simulate available headers and fields
        const std::unordered_map<std::string, std::vector<std::string>> headerFields = {
            // Ethernet header
            {"eth_hdr", {"h_dest", "h_source", "h_proto"}},

            // Outer IPv4 and UDP headers
            {"outer_ipv4_hdr", {"saddr", "daddr", "ttl", "protocol"}},
            {"outer_udp_hdr", {"source", "dest", "len", "check"}},

            // Inner IPv4 and UDP headers (in case of encapsulation, like VXLAN)
            {"inner_ipv4_hdr", {"saddr", "daddr", "ttl", "protocol"}},
            {"inner_udp_hdr", {"source", "dest", "len", "check"}},

            // VXLAN header fields
            {"vxlan_hdr", {"vni"}},

            // Generic UDP and IPv4 headers
            {"udp_hdr", {"source", "dest", "len", "check"}},
            {"ipv4_hdr", {"saddr", "daddr", "ttl", "protocol"}}
        };

        bool isValidHeader(const std::string& header) {
            return headerFields.find(header) != headerFields.end();
        }

        bool isValidField(const std::string& header, const std::string& field) {
            auto it = headerFields.find(header);
            if (it != headerFields.end()) {
                const auto& fields = it->second;
                return std::find(fields.begin(), fields.end(), field) != fields.end();
            }
            std::cerr << "Field " << field << " is not valid in " << header << std::endl;
            return false;
        }
    };
    friend class ActionStateMachine;

private:
    PipeDecl        FlowPipe;

    MetaDecl        FlowMeta;
    FormatDecl      FlowFormat;

    /* Format */
    EthhdrDecl      FlowEthhdr;
    L3TypeDecl      FlowL3Type;
    IphdrDecl       FlowIphdr;
    L4TypeExtDecl   FlowL4TypeExt;
    UdphdrDecl      FlowUdphdr;
    TdphdrDecl      FlowTcphdr;
    L4HeaderDecl    FlowL4Header;
    TunTypeDecl     FlowTunType;
    TunDecl         FlowTun;

    MatchDecl       FlowMatch;

    EncapActionDecl FlowEncapAction;
    ActionsDecl     FlowActions;

    FwdTypeDecl     FlowFwdType;
    FwdDecl         FlowFwd;

    PipeTypeDecl    FlowPipeType;
    PipeDomainDecl  FlowPipeDomain;
    PipeAttrDecl    FlowPipeAttr;
    PipeCfgDecl     FlowPipeCfg;

    GetPipeDecl     FlowGetPipeByName;
    GetPipeIdDecl   FlowGetPipeIdByName;
    GetPipeIdDecl   FlowGetPipeId;
    CreatePipeDecl  FlowCreatePipe;
    ControlPipeAddEntryDecl  FlowControlPipeAddEntry;
    PipeAddEntryDecl  FlowPipeAddEntry;

    std::vector<Stmt*>  CreatePipeCall;

    // RecordDecl * FlowMetaDecl;

    // RecordDecl * FlowHeaderIP4Decl;

    // RecordDecl * FlowHeaderUDPDecl;

    // RecordDecl * FlowHeaderTCPDecl;

    // RecordDecl * FlowL4UnionDecl;
};

#endif  /* _BACKENDS_DOCA_DOCAPROGRAM_H_ */