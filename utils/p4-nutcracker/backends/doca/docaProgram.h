#ifndef _BACKENDS_DOCA_DOCAPROGRAM_H_
#define _BACKENDS_DOCA_DOCAPROGRAM_H_

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

class DocaProgram {
    friend class ASTContext;
public:
    const CallGraph& callGraph;
    json j;

    bool buildHeaderStructure();

    bool buildPipe(Pipeline::Kind K, const CallGraphNodeBase* node, bool isRoot);
    bool initPipe(Pipeline::Kind K, const CallGraphNodeBase* node);

    bool buildIngressActionPipe(const ActionNode* actionNode);
    bool buildEgressActionPipe(const ActionNode* actionNode);
    bool buildActionPipe(Pipeline::Kind K, const ActionNode* actionNode);

    bool buildIngressConditionalPipe(const ConditionalNode* conditionalNode, bool isRoot);
    bool buildEgressConditionalPipe(const ConditionalNode* conditionalNode, bool isRoot);
    bool buildConditionalPipe(Pipeline::Kind K, const ConditionalNode* conditionalNode, bool isRoot);
    bool initConditionalPipe(const ConditionalNode* conditionalNode);

    bool buildIngressSingleActionTablePipe(const TableNode* tableNode, bool isRoot);
    bool buildIngressMultiActionTablePipe(const TableNode* tableNode, bool isRoot);
    bool buildIngressTablePipe(const TableNode* tableNode, bool isRoot);
    bool buildEgressTablePipe(const TableNode* tableNode, bool isRoot);
    bool buildTablePipe(Pipeline::Kind K, const TableNode* tableNode, bool isRoot);
    bool initTablePipe(const TableNode* tableNode);
    bool buildAddActionPipeEntry(const ActionNode* actionNode);
    bool buildAddTablePipeEntry(const TableNode* tableNode);

    bool build();

    void processExpression(const std::shared_ptr<Expression>& expr, std::vector<Stmt*>& Stmts,  DeclRefExpr* MatchVarRef,  bool *ipProtocolSet, bool *transportProtocolSet, bool *tunnelSet);

    DocaProgram(const CallGraph& callGraph) : callGraph(callGraph),  ctx() {}

    void emit(void) { ctx.emit(); }

    void emitGeneratedComment(CodeBuilder *builder);
    void emitH(CodeBuilder *builder, const std::filesystem::path &headerFile);  // emits C headers
    void emitC(CodeBuilder *builder, const std::filesystem::path &headerFile);  // emits C program

    ASTContext ctx;

    class MetaDecl {
        friend class DocaProgram;

        RecordDecl          * Decl;
        RecordType          * Type;

        FieldDecl           * PktMeta;
        FieldDecl           * U32;
    };

    class EthhdrDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Dest;
        FieldDecl *Src;
        FieldDecl *Proto;
    };

    class L3TypeDecl {
        friend class DocaProgram;
        EnumDecl * Decl;
        EnumType * Type;
        EnumConstantDecl * TypeNone;
        EnumConstantDecl * TypeIpv4;
    };

    class IphdrDecl {
        friend class DocaProgram;

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
        friend class DocaProgram;
        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * ExtNone;
        EnumConstantDecl * ExtTcp;
        EnumConstantDecl * ExtUdp;
    };

    class UdphdrDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Source;
        FieldDecl *Dest;
        FieldDecl *Len;
        FieldDecl *Check;
    };

    class TdphdrDecl {
        friend class DocaProgram;

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
        friend class DocaProgram;
        friend class UdphdrDecl;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *Udp;
        FieldDecl *Tcp;
    };

    class TunTypeDecl {
        friend class DocaProgram;
        EnumDecl * Decl;
        EnumType * Type;
        EnumConstantDecl * TypeNone;
        EnumConstantDecl * TypeVxlan;
    };

    class TunDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl *TunType;
        FieldDecl *VxlanTunId;
    };

    class FormatDecl {
        friend class DocaProgram;

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
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Meta;
        FieldDecl   *Outer;
        FieldDecl   *Inner;
    };

    class EncapActionDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Outer;
        FieldDecl   *Tun;
    };

    class ActionsDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Meta;
        FieldDecl   *Outer;
        FieldDecl   *HasEncap;
        FieldDecl   *Encap;
    };

    class FwdTypeDecl {
        friend class DocaProgram;

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
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *FwdType;
        FieldDecl   *FwdNextPipe;
    };

    class PipeTypeDecl {
        friend class DocaProgram;

        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * Basic;
        EnumConstantDecl * Control;
    };

    class PipeDomainDecl {
        friend class DocaProgram;

        EnumDecl * Decl;
        EnumType * Type;

        EnumConstantDecl * Ingress;
        EnumConstantDecl * Egress;
    };

    class PipeAttrDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Name;
        FieldDecl   *PipeType;
        FieldDecl   *Domain;
        FieldDecl   *IsRoot;
        FieldDecl   *NbActions;
    };

    class PipeCfgDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;

        FieldDecl   *Attr;
        FieldDecl   *Match;
        FieldDecl   *Actions;
    };

    class PipeDecl {
        friend class DocaProgram;

        RecordDecl * Decl;
        RecordType * Type;
    };

    class GetPipeDecl {
        friend class DocaProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class GetPipeIdDecl {
        friend class DocaProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class CreatePipeDecl {
        friend class DocaProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class ControlPipeAddEntryDecl {
        friend class DocaProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class PipeAddEntryDecl {
        friend class DocaProgram;

        FunctionDecl * Decl;
        FunctionType * Type;
    };

    class MatchStateMachine {
        friend class DocaProgram;

        enum class State {
            Start,        // Initial state
            NestedHeaderParsed,
            HeaderParsed, // Header identified
            FieldParsed,  // Field identified
            Success,      // Successfully parsed
            Error         // Error in parsing
        };

        MatchStateMachine() : currentState(State::Start) {}

        MemberExpr * parseTarget(DocaProgram *docaProgram, std::vector<Stmt *> &Stmts, const std::vector<std::string>& target, 
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
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowMatch.Outer, docaProgram->FlowMatch.Outer->getDeclName(), docaProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.Eth, docaProgram->FlowFormat.Eth->getDeclName(), docaProgram->FlowFormat.Eth->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                            } else if (currentHeader == "ipv4_hdr") {
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowMatch.Outer, docaProgram->FlowMatch.Outer->getDeclName(), docaProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.Ip, docaProgram->FlowFormat.Ip->getDeclName(), docaProgram->FlowFormat.Ip->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*ipProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowMatch.Outer, docaProgram->FlowMatch.Outer->getDeclName(), docaProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.L3Type, docaProgram->FlowFormat.L3Type->getDeclName(), docaProgram->FlowFormat.L3Type->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L3TypeIpv4Id = const_cast<IdentifierInfo*>(docaProgram->FlowL3Type.TypeIpv4->getIdentifier());
                                    DeclRefExpr *L3TypeIpv4Ref = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowL3Type.TypeIpv4, L3TypeIpv4Id, QualType(docaProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L3TypeAssign = BinaryOperator::Create(docaProgram->ctx, expr, L3TypeIpv4Ref, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L3TypeAssign);
                                    *ipProtocolSet = true;
                                }
                            } else if (currentHeader == "udp_hdr") {
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowMatch.Outer, docaProgram->FlowMatch.Outer->getDeclName(), docaProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowL4Header.Udp, docaProgram->FlowL4Header.Udp->getDeclName(), docaProgram->FlowL4Header.Udp->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*transportProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowMatch.Outer, docaProgram->FlowMatch.Outer->getDeclName(), docaProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.L4TypeExt, docaProgram->FlowFormat.L4TypeExt->getDeclName(), docaProgram->FlowFormat.L4TypeExt->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L4TypeExtUdpId = const_cast<IdentifierInfo*>(docaProgram->FlowL4TypeExt.ExtUdp->getIdentifier());
                                    DeclRefExpr *L4TypeExtUdpRef = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowL4TypeExt.ExtUdp, L4TypeExtUdpId, QualType(docaProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L4TypeExtAssign = BinaryOperator::Create(docaProgram->ctx, expr, L4TypeExtUdpRef, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L4TypeExtAssign);
                                    *transportProtocolSet = true;
                                }
                                if (!(*ipProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowMatch.Outer, docaProgram->FlowMatch.Outer->getDeclName(), docaProgram->FlowMatch.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.L3Type, docaProgram->FlowFormat.L3Type->getDeclName(), docaProgram->FlowFormat.L3Type->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L3TypeIpv4Id = const_cast<IdentifierInfo*>(docaProgram->FlowL3Type.TypeIpv4->getIdentifier());
                                    DeclRefExpr *L3TypeIpv4Ref = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowL3Type.TypeIpv4, L3TypeIpv4Id, QualType(docaProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L3TypeAssign = BinaryOperator::Create(docaProgram->ctx, expr, L3TypeIpv4Ref, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
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
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowEthhdr.Dest->getDeclName(), docaProgram->FlowEthhdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowEthhdr.Src->getDeclName(), docaProgram->FlowEthhdr.Src->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowIphdr.Daddr->getDeclName(), docaProgram->FlowIphdr.Daddr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "protocol") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowIphdr.Protocol->getDeclName(), docaProgram->FlowIphdr.Protocol->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowUdphdr.Dest->getDeclName(), docaProgram->FlowUdphdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowUdphdr.Dest->getType();
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

        QualType getLastFieldTypeAndName(DocaProgram *docaProgram, const std::vector<std::string>& target) {
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
                                    lastFieldType = docaProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    lastFieldType = docaProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    lastFieldType = docaProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "saddr") {
                                    lastFieldType = docaProgram->FlowIphdr.Saddr->getType();
                                } else if (currentField == "protocol") {
                                    lastFieldType = docaProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    lastFieldType = docaProgram->FlowUdphdr.Dest->getType();
                                } else if (currentField == "source") {
                                    lastFieldType = docaProgram->FlowUdphdr.Source->getType();
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                if (currentField == "vni") {
                                    lastFieldType = docaProgram->FlowTun.VxlanTunId->getType();
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
        friend class DocaProgram;

        enum class State {
            Start,        // Initial state
            NestedHeaderParsed,
            HeaderParsed, // Header identified
            FieldParsed,  // Field identified
            Success,      // Successfully parsed
            Error         // Error in parsing
        };

        ActionStateMachine() : currentState(State::Start) {}

        MemberExpr * parseTarget(DocaProgram *docaProgram, std::vector<Stmt *> &Stmts, const std::vector<std::string>& target, 
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
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Outer, docaProgram->FlowActions.Outer->getDeclName(), docaProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.Eth, docaProgram->FlowFormat.Eth->getDeclName(), docaProgram->FlowFormat.Eth->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                            } else if (currentHeader == "ipv4_hdr") {
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Outer, docaProgram->FlowActions.Outer->getDeclName(), docaProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.Ip, docaProgram->FlowFormat.Ip->getDeclName(), docaProgram->FlowFormat.Ip->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*ipProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Outer, docaProgram->FlowActions.Outer->getDeclName(), docaProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.L3Type, docaProgram->FlowFormat.L3Type->getDeclName(), docaProgram->FlowFormat.L3Type->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L3TypeIpv4Id = const_cast<IdentifierInfo*>(docaProgram->FlowL3Type.TypeIpv4->getIdentifier());
                                    DeclRefExpr *L3TypeIpv4Ref = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowL3Type.TypeIpv4, L3TypeIpv4Id, QualType(docaProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L3TypeAssign = BinaryOperator::Create(docaProgram->ctx, expr, L3TypeIpv4Ref, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L3TypeAssign);
                                    *ipProtocolSet = true;
                                }
                            } else if (currentHeader == "udp_hdr") {
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Outer, docaProgram->FlowActions.Outer->getDeclName(), docaProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowL4Header.Udp, docaProgram->FlowL4Header.Udp->getDeclName(), docaProgram->FlowL4Header.Udp->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*transportProtocolSet)) {
                                    MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Outer, docaProgram->FlowActions.Outer->getDeclName(), docaProgram->FlowActions.Outer->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowFormat.L4TypeExt, docaProgram->FlowFormat.L4TypeExt->getDeclName(), docaProgram->FlowFormat.L4TypeExt->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    IdentifierInfo *L4TypeExtUdpId = const_cast<IdentifierInfo*>(docaProgram->FlowL4TypeExt.ExtUdp->getIdentifier());
                                    DeclRefExpr *L4TypeExtUdpRef = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowL4TypeExt.ExtUdp, L4TypeExtUdpId, QualType(docaProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                    BinaryOperator *L4TypeExtAssign = BinaryOperator::Create(docaProgram->ctx, expr, L4TypeExtUdpRef, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                    Stmts.push_back(L4TypeExtAssign);
                                    *transportProtocolSet = true;
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Encap, docaProgram->FlowActions.Encap->getDeclName(), docaProgram->FlowActions.Encap->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowEncapAction.Tun, docaProgram->FlowEncapAction.Tun->getDeclName(), docaProgram->FlowEncapAction.Tun->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                if (!(*tunnelSet)) {
                                    {
                                        MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.HasEncap, docaProgram->FlowActions.HasEncap->getDeclName(), docaProgram->FlowActions.HasEncap->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        // IdentifierInfo *ActionsHasEncapId = const_cast<IdentifierInfo*>(docaProgram->FlowActions.HasEncap->getIdentifier());
                                        // DeclRefExpr *ActionsHasEncapRef = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowActions.HasEncap, ActionsHasEncapId, QualType(docaProgram->ctx.BoolTy), ExprValueKind::VK_PRValue);
                                        IntegerLiteral *hasEncap = IntegerLiteral::Create(docaProgram->ctx, APInt(1, 1), IntegerLiteral::Dec, QualType(docaProgram->ctx.BoolTy));
                                        BinaryOperator *ActionsHasEncapAssign = BinaryOperator::Create(docaProgram->ctx, expr, hasEncap, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
                                        Stmts.push_back(ActionsHasEncapAssign);
                                    }
                                    {
                                        MemberExpr *expr = MemberExpr::Create(docaProgram->ctx, MatchVarRef, false, docaProgram->FlowActions.Encap, docaProgram->FlowActions.Encap->getDeclName(), docaProgram->FlowActions.Encap->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowEncapAction.Tun, docaProgram->FlowEncapAction.Tun->getDeclName(), docaProgram->FlowEncapAction.Tun->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        expr = MemberExpr::Create(docaProgram->ctx, expr, false, docaProgram->FlowTun.TunType, docaProgram->FlowTun.TunType->getDeclName(), docaProgram->FlowTun.TunType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                        IdentifierInfo *TunnelTypeVxlanId = const_cast<IdentifierInfo*>(docaProgram->FlowTunType.TypeVxlan->getIdentifier());
                                        DeclRefExpr *TunnelTypeVxlanRef = DeclRefExpr::Create(docaProgram->ctx, docaProgram->FlowTunType.TypeVxlan, TunnelTypeVxlanId, QualType(docaProgram->ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
                                        BinaryOperator *TunnelTypeAssign = BinaryOperator::Create(docaProgram->ctx, expr, TunnelTypeVxlanRef, BO_Assign, QualType(docaProgram->ctx.VoidTy), ExprValueKind::VK_LValue, OK_Ordinary);
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
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowEthhdr.Dest->getDeclName(), docaProgram->FlowEthhdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowEthhdr.Src->getDeclName(), docaProgram->FlowEthhdr.Src->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowIphdr.Daddr->getDeclName(), docaProgram->FlowIphdr.Daddr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "saddr") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowIphdr.Saddr->getDeclName(), docaProgram->FlowIphdr.Saddr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowIphdr.Saddr->getType();
                                } else if (currentField == "protocol") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowIphdr.Protocol->getDeclName(), docaProgram->FlowIphdr.Protocol->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowUdphdr.Dest->getDeclName(), docaProgram->FlowUdphdr.Dest->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowUdphdr.Dest->getType();
                                } else if (currentField == "source") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowUdphdr.Source->getDeclName(), docaProgram->FlowUdphdr.Source->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowUdphdr.Source->getType();
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                if (currentField == "vni") {
                                    expr = MemberExpr::Create(docaProgram->ctx, expr, false, nullptr, docaProgram->FlowTun.VxlanTunId->getDeclName(), docaProgram->FlowTun.VxlanTunId->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
                                    *MatchFieldType = docaProgram->FlowTun.VxlanTunId->getType();
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

        QualType getLastFieldTypeAndName(DocaProgram *docaProgram, const std::vector<std::string>& target) {
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
                                    lastFieldType = docaProgram->FlowEthhdr.Dest->getType();
                                } else if (currentField == "h_source") {
                                    lastFieldType = docaProgram->FlowEthhdr.Src->getType();
                                }
                            } else if (currentHeader == "ipv4_hdr") {
                                if (currentField == "daddr") {
                                    lastFieldType = docaProgram->FlowIphdr.Daddr->getType();
                                } else if (currentField == "saddr") {
                                    lastFieldType = docaProgram->FlowIphdr.Saddr->getType();
                                } else if (currentField == "protocol") {
                                    lastFieldType = docaProgram->FlowIphdr.Protocol->getType();
                                }
                            } else if (currentHeader == "udp_hdr") {
                                if (currentField == "dest") {
                                    lastFieldType = docaProgram->FlowUdphdr.Dest->getType();
                                } else if (currentField == "source") {
                                    lastFieldType = docaProgram->FlowUdphdr.Source->getType();
                                }
                            } else if (currentHeader == "vxlan_hdr") {
                                if (currentField == "vni") {
                                    lastFieldType = docaProgram->FlowTun.VxlanTunId->getType();
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