#include "AST/Attr.h"
#include "AST/Stmt.h"
#include "backends/target.h"
#include "docaProgram.h"

bool DocaProgram::buildHeaderStructure() {
    std::vector<Attr *> attrs;
    Attr *Packed = PackedAttr::Create(ctx);
    attrs.push_back(Packed);

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));

    /* <-------------------- Pipe --------------------> */
    FlowPipe.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_pipe"), nullptr);
    FlowPipe.Type = new RecordType(FlowPipe.Decl);
    ctx.getTranslationUnitDecl()->addDecl(FlowPipe.Decl);

    /* <-------------------- Match --------------------> */
    /* Create flow_meta */
    FlowMeta.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_meta"), nullptr);
    FlowMeta.Type = new RecordType(FlowMeta.Decl);
    FlowMeta.PktMeta = FieldDecl::Create(ctx, FlowMeta.Decl, &ctx.getIdentifierTable()->get("pkt_meta"), QualType(ctx.UnsignedIntTy), nullptr);
    ConstantArrayType * U32Type = new ConstantArrayType(QualType(ctx.UnsignedIntTy), APInt(APInt::APINT_BITS_PER_WORD, 4));
    FlowMeta.U32 = FieldDecl::Create(ctx, FlowMeta.Decl, &ctx.getIdentifierTable()->get("u32"), QualType(U32Type), nullptr);
    FlowMeta.Decl->addDecl(FlowMeta.PktMeta);
    FlowMeta.Decl->addDecl(FlowMeta.U32);
    ctx.getTranslationUnitDecl()->addDecl(FlowMeta.Decl);
    FlowMeta.Decl->setAttrs(attrs);

    /* Create flow_header_eth */
    FlowEthhdr.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_header_eth"), nullptr);
    FlowEthhdr.Type = new RecordType(FlowEthhdr.Decl);
    ConstantArrayType *MacAddrType = new ConstantArrayType(QualType(ctx.UnsignedCharTy), APInt(APInt::APINT_BITS_PER_WORD, 6));
    FlowEthhdr.Dest = FieldDecl::Create(ctx, FlowEthhdr.Decl, &ctx.getIdentifierTable()->get("h_dest"), QualType(MacAddrType), nullptr);
    FlowEthhdr.Src = FieldDecl::Create(ctx, FlowEthhdr.Decl, &ctx.getIdentifierTable()->get("h_source"), QualType(MacAddrType), nullptr);
    FlowEthhdr.Proto = FieldDecl::Create(ctx, FlowEthhdr.Decl, &ctx.getIdentifierTable()->get("h_proto"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowEthhdr.Decl->addDecl(FlowEthhdr.Dest);
    FlowEthhdr.Decl->addDecl(FlowEthhdr.Src);
    FlowEthhdr.Decl->addDecl(FlowEthhdr.Proto);
    ctx.getTranslationUnitDecl()->addDecl(FlowEthhdr.Decl);
    FlowEthhdr.Decl->setAttrs(attrs);

    /* Create flow_l3_type */
    FlowL3Type.Decl = EnumDecl::Create(ctx, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_l3_type"), nullptr);
    FlowL3Type.Type = new EnumType(FlowL3Type.Decl);
    FlowL3Type.TypeNone = EnumConstantDecl::Create(ctx, FlowL3Type.Decl, &ctx.getIdentifierTable()->get("FLOW_L3_TYPE_NONE"), QualType(ctx.UnsignedIntTy), Zero, APSInt(APInt(IntSize, 0)));
    FlowL3Type.TypeIpv4 = EnumConstantDecl::Create(ctx, FlowL3Type.Decl, &ctx.getIdentifierTable()->get("FLOW_L3_TYPE_IP4"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowL3Type.Decl->addDecl(FlowL3Type.TypeNone);
    FlowL3Type.Decl->addDecl(FlowL3Type.TypeIpv4);
    ctx.getTranslationUnitDecl()->addDecl(FlowL3Type.Decl);

    /* Create flow_header_ip */
    FlowIphdr.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_header_ip4"), nullptr);
    FlowIphdr.Type = new RecordType(FlowIphdr.Decl);
    IntegerLiteral * ipVersion = IntegerLiteral::Create(ctx, APInt(IntSize, 4), IntegerLiteral::Dec, QualType(ctx.UnsignedIntTy));
    FlowIphdr.Version = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("version"), QualType(ctx.UnsignedCharTy), ipVersion);
    IntegerLiteral * ipIhl = IntegerLiteral::Create(ctx, APInt(IntSize, 4), IntegerLiteral::Dec, QualType(ctx.UnsignedIntTy));
    FlowIphdr.Ihl = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("ihl"), QualType(ctx.UnsignedCharTy), ipIhl);
    FlowIphdr.Tos = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("tos"), QualType(ctx.UnsignedCharTy), nullptr);
    FlowIphdr.TotLen = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("tot_len"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowIphdr.Id = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("id"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowIphdr.FragOff = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("frag_off"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowIphdr.Ttl = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("ttl"), QualType(ctx.UnsignedCharTy), nullptr);
    FlowIphdr.Protocol = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("protocol"), QualType(ctx.UnsignedCharTy), nullptr);
    FlowIphdr.Check = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("check"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowIphdr.Saddr = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("saddr"), QualType(ctx.UnsignedIntTy), nullptr);
    FlowIphdr.Daddr = FieldDecl::Create(ctx, FlowIphdr.Decl, &ctx.getIdentifierTable()->get("daddr"), QualType(ctx.UnsignedIntTy), nullptr);
    FlowIphdr.Decl->addDecl(FlowIphdr.Version);
    FlowIphdr.Decl->addDecl(FlowIphdr.Ihl);
    FlowIphdr.Decl->addDecl(FlowIphdr.Tos);
    FlowIphdr.Decl->addDecl(FlowIphdr.TotLen);
    FlowIphdr.Decl->addDecl(FlowIphdr.Id);
    FlowIphdr.Decl->addDecl(FlowIphdr.FragOff);
    FlowIphdr.Decl->addDecl(FlowIphdr.Ttl);
    FlowIphdr.Decl->addDecl(FlowIphdr.Protocol);
    FlowIphdr.Decl->addDecl(FlowIphdr.Check);
    FlowIphdr.Decl->addDecl(FlowIphdr.Saddr);
    FlowIphdr.Decl->addDecl(FlowIphdr.Daddr);
    ctx.getTranslationUnitDecl()->addDecl(FlowIphdr.Decl);
    FlowIphdr.Decl->setAttrs(attrs);

    /* Create flow_l4_type_etx */
    FlowL4TypeExt.Decl = EnumDecl::Create(ctx, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_l4_type_ext"), nullptr);
    FlowL4TypeExt.Type = new EnumType(FlowL4TypeExt.Decl);
    FlowL4TypeExt.ExtNone = EnumConstantDecl::Create(ctx, FlowL4TypeExt.Decl, &ctx.getIdentifierTable()->get("FLOW_L4_TYPE_EXT_NONE"), QualType(ctx.UnsignedIntTy), Zero, APSInt(APInt(IntSize, 0)));
    FlowL4TypeExt.ExtTcp = EnumConstantDecl::Create(ctx, FlowL4TypeExt.Decl, &ctx.getIdentifierTable()->get("FLOW_L4_TYPE_EXT_TCP"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowL4TypeExt.ExtUdp = EnumConstantDecl::Create(ctx, FlowL4TypeExt.Decl, &ctx.getIdentifierTable()->get("FLOW_L4_TYPE_EXT_UDP"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowL4TypeExt.Decl->addDecl(FlowL4TypeExt.ExtNone);
    FlowL4TypeExt.Decl->addDecl(FlowL4TypeExt.ExtTcp);
    FlowL4TypeExt.Decl->addDecl(FlowL4TypeExt.ExtUdp);
    ctx.getTranslationUnitDecl()->addDecl(FlowL4TypeExt.Decl);

    /* Create flow_header_udp */
    FlowUdphdr.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_header_udp"), nullptr);
    FlowUdphdr.Type = new RecordType(FlowUdphdr.Decl);
    FlowUdphdr.Source = FieldDecl::Create(ctx, FlowUdphdr.Decl, &ctx.getIdentifierTable()->get("source"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowUdphdr.Dest = FieldDecl::Create(ctx, FlowUdphdr.Decl, &ctx.getIdentifierTable()->get("dest"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowUdphdr.Len = FieldDecl::Create(ctx, FlowUdphdr.Decl, &ctx.getIdentifierTable()->get("len"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowUdphdr.Check = FieldDecl::Create(ctx, FlowUdphdr.Decl, &ctx.getIdentifierTable()->get("check"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowUdphdr.Decl->addDecl(FlowUdphdr.Source);
    FlowUdphdr.Decl->addDecl(FlowUdphdr.Dest);
    FlowUdphdr.Decl->addDecl(FlowUdphdr.Len);
    FlowUdphdr.Decl->addDecl(FlowUdphdr.Check);
    ctx.getTranslationUnitDecl()->addDecl(FlowUdphdr.Decl);
    FlowUdphdr.Decl->setAttrs(attrs);

    /* Create flow_header_tcp */
    FlowTcphdr.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_header_tcp"), nullptr);
    FlowTcphdr.Type = new RecordType(FlowTcphdr.Decl);
    FlowTcphdr.Source = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("source"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowTcphdr.Dest = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("dest"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowTcphdr.Seq = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("seq"), QualType(ctx.UnsignedIntTy), nullptr);
    FlowTcphdr.AckSeq = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("ack_seq"), QualType(ctx.UnsignedIntTy), nullptr);
    IntegerLiteral * tcpDoff = IntegerLiteral::Create(ctx, APInt(IntSize, 4), IntegerLiteral::Dec, QualType(ctx.UnsignedIntTy));
    FlowTcphdr.Doff = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("doff"), QualType(ctx.UnsignedCharTy), tcpDoff);
    FlowTcphdr.Res1 = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("res1"), QualType(ctx.UnsignedCharTy), tcpDoff);
    FlowTcphdr.Flags = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("flags"), QualType(ctx.UnsignedCharTy), nullptr);
    FlowTcphdr.Window = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("window"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowTcphdr.Check = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("check"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowTcphdr.UrgPtr = FieldDecl::Create(ctx, FlowTcphdr.Decl, &ctx.getIdentifierTable()->get("urg_ptr"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Source);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Dest);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Seq);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.AckSeq);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Doff);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Res1);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Flags);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Window);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.Check);
    FlowTcphdr.Decl->addDecl(FlowTcphdr.UrgPtr);
    ctx.getTranslationUnitDecl()->addDecl(FlowTcphdr.Decl);
    FlowTcphdr.Decl->setAttrs(attrs);

    /* Create union for UDP and TCP */
    FlowL4Header.Decl = RecordDecl::Create(ctx, TTK_Union, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get(""), nullptr);
    FlowL4Header.Type = new RecordType(FlowL4Header.Decl);
    FlowL4Header.Udp = FieldDecl::Create(ctx, FlowL4Header.Decl, &ctx.getIdentifierTable()->get("udp"), QualType(FlowUdphdr.Type), nullptr);
    FlowL4Header.Decl->addDecl(FlowL4Header.Udp);
    FlowL4Header.Tcp = FieldDecl::Create(ctx, FlowL4Header.Decl, &ctx.getIdentifierTable()->get("tcp"), QualType(FlowTcphdr.Type), nullptr);
    FlowL4Header.Decl->addDecl(FlowL4Header.Tcp);

    /* Create flow_header_format */
    FlowFormat.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_header_format"), nullptr);
    FlowFormat.Type = new RecordType(FlowFormat.Decl);
    FlowFormat.Eth = FieldDecl::Create(ctx, FlowFormat.Decl, &ctx.getIdentifierTable()->get("eth"), QualType(FlowEthhdr.Type), nullptr);
    FlowFormat.EthValid = FieldDecl::Create(ctx, FlowFormat.Decl, &ctx.getIdentifierTable()->get("l2_valid_headers"), QualType(ctx.UnsignedShortTy), nullptr);
    FlowFormat.L3Type = FieldDecl::Create(ctx, FlowFormat.Decl, &ctx.getIdentifierTable()->get("l3_type"), QualType(FlowL3Type.Type), nullptr);
    FlowFormat.Ip = FieldDecl::Create(ctx, FlowFormat.Decl, &ctx.getIdentifierTable()->get("ip4"), QualType(FlowIphdr.Type), nullptr);
    FlowFormat.L4TypeExt = FieldDecl::Create(ctx, FlowFormat.Decl, &ctx.getIdentifierTable()->get("l4_type_ext"), QualType(FlowL4TypeExt.Type), nullptr);
    FlowFormat.Udp = FieldDecl::Create(ctx, FlowFormat.Decl, &ctx.getIdentifierTable()->get(""), QualType(FlowL4Header.Type), nullptr);
    FlowFormat.Decl->addDecl(FlowFormat.Eth);
    FlowFormat.Decl->addDecl(FlowFormat.EthValid);
    FlowFormat.Decl->addDecl(FlowFormat.L3Type);
    FlowFormat.Decl->addDecl(FlowFormat.Ip);
    FlowFormat.Decl->addDecl(FlowFormat.L4TypeExt);
    FlowFormat.Decl->addDecl(FlowFormat.Udp);
    ctx.getTranslationUnitDecl()->addDecl(FlowFormat.Decl);
    FlowFormat.Decl->setAttrs(attrs);

    /* Create flow_l3_type */
    FlowTunType.Decl = EnumDecl::Create(ctx, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_tun_type"), nullptr);
    FlowTunType.Type = new EnumType(FlowTunType.Decl);
    FlowTunType.TypeNone = EnumConstantDecl::Create(ctx, FlowTunType.Decl, &ctx.getIdentifierTable()->get("FLOW_TUN_NONE"), QualType(ctx.UnsignedIntTy), Zero, APSInt(APInt(IntSize, 0)));
    FlowTunType.TypeVxlan = EnumConstantDecl::Create(ctx, FlowTunType.Decl, &ctx.getIdentifierTable()->get("FLOW_TUN_VXLAN"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowTunType.Decl->addDecl(FlowTunType.TypeNone);
    FlowTunType.Decl->addDecl(FlowTunType.TypeVxlan);
    ctx.getTranslationUnitDecl()->addDecl(FlowTunType.Decl);

    /* Create flow_tun */
    FlowTun.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_tun"), nullptr);
    FlowTun.Type = new RecordType(FlowTun.Decl);
    FlowTun.TunType = FieldDecl::Create(ctx, FlowTun.Decl, &ctx.getIdentifierTable()->get("type"), QualType(FlowTunType.Type), nullptr);
    FlowTun.VxlanTunId = FieldDecl::Create(ctx, FlowTun.Decl, &ctx.getIdentifierTable()->get("vxlan_tun_id"), QualType(ctx.UnsignedIntTy), nullptr);
    FlowTun.Decl->addDecl(FlowTun.TunType);
    FlowTun.Decl->addDecl(FlowTun.VxlanTunId);
    ctx.getTranslationUnitDecl()->addDecl(FlowTun.Decl);
    FlowTun.Decl->setAttrs(attrs);

    /* Create flow_match */
    FlowMatch.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_match"), nullptr);
    FlowMatch.Type = new RecordType(FlowMatch.Decl);
    FlowMatch.Meta = FieldDecl::Create(ctx, FlowMatch.Decl, &ctx.getIdentifierTable()->get("meta"), QualType(FlowMeta.Type), nullptr);
    FlowMatch.Outer = FieldDecl::Create(ctx, FlowMatch.Decl, &ctx.getIdentifierTable()->get("outer"), QualType(FlowFormat.Type), nullptr);
    FlowMatch.Decl->addDecl(FlowMatch.Meta);
    FlowMatch.Decl->addDecl(FlowMatch.Outer);
    ctx.getTranslationUnitDecl()->addDecl(FlowMatch.Decl);
    FlowMatch.Decl->setAttrs(attrs);

    /* <-------------------- Action --------------------> */
    /* Create flow_encap_action */
    FlowEncapAction.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_encap_action"), nullptr);
    FlowEncapAction.Type = new RecordType(FlowEncapAction.Decl);
    FlowEncapAction.Outer = FieldDecl::Create(ctx, FlowEncapAction.Decl, &ctx.getIdentifierTable()->get("outer"), QualType(FlowFormat.Type), nullptr);
    FlowEncapAction.Tun = FieldDecl::Create(ctx, FlowEncapAction.Decl, &ctx.getIdentifierTable()->get("tun"), QualType(FlowTun.Type), nullptr);
    FlowEncapAction.Decl->addDecl(FlowEncapAction.Outer);
    FlowEncapAction.Decl->addDecl(FlowEncapAction.Tun);
    ctx.getTranslationUnitDecl()->addDecl(FlowEncapAction.Decl);
    FlowEncapAction.Decl->setAttrs(attrs);

    /* Create flow_actions */
    FlowActions.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_actions"), nullptr);
    FlowActions.Type = new RecordType(FlowActions.Decl);
    FlowActions.Meta = FieldDecl::Create(ctx, FlowActions.Decl, &ctx.getIdentifierTable()->get("meta"), QualType(FlowMeta.Type), nullptr);
    FlowActions.Outer = FieldDecl::Create(ctx, FlowActions.Decl, &ctx.getIdentifierTable()->get("outer"), QualType(FlowFormat.Type), nullptr);
    FlowActions.HasEncap = FieldDecl::Create(ctx, FlowActions.Decl, &ctx.getIdentifierTable()->get("has_encap"), QualType(ctx.BoolTy), nullptr);
    FlowActions.Encap = FieldDecl::Create(ctx, FlowActions.Decl, &ctx.getIdentifierTable()->get("encap"), QualType(FlowEncapAction.Type), nullptr);
    FlowActions.Decl->addDecl(FlowActions.Meta);
    FlowActions.Decl->addDecl(FlowActions.Outer);
    FlowActions.Decl->addDecl(FlowActions.HasEncap);
    FlowActions.Decl->addDecl(FlowActions.Encap);
    ctx.getTranslationUnitDecl()->addDecl(FlowActions.Decl);
    FlowActions.Decl->setAttrs(attrs);

    /* <-------------------- Forward --------------------> */
    /* Create flow_l4_type_etx */
    FlowFwdType.Decl = EnumDecl::Create(ctx, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_fwd_type"), nullptr);
    FlowFwdType.Type = new EnumType(FlowFwdType.Decl);
    FlowFwdType.FwdNone = EnumConstantDecl::Create(ctx, FlowFwdType.Decl, &ctx.getIdentifierTable()->get("FLOW_FWD_NONE"), QualType(ctx.UnsignedIntTy), Zero, APSInt(APInt(IntSize, 0)));
    FlowFwdType.FwdRss = EnumConstantDecl::Create(ctx, FlowFwdType.Decl, &ctx.getIdentifierTable()->get("FLOW_FWD_RSS"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowFwdType.FwdHairpin = EnumConstantDecl::Create(ctx, FlowFwdType.Decl, &ctx.getIdentifierTable()->get("FLOW_FWD_HAIRPIN"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowFwdType.FwdPort = EnumConstantDecl::Create(ctx, FlowFwdType.Decl, &ctx.getIdentifierTable()->get("FLOW_FWD_PORT"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowFwdType.FwdPipe = EnumConstantDecl::Create(ctx, FlowFwdType.Decl, &ctx.getIdentifierTable()->get("FLOW_FWD_PIPE"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowFwdType.FwdDrop = EnumConstantDecl::Create(ctx, FlowFwdType.Decl, &ctx.getIdentifierTable()->get("FLOW_FWD_DROP"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowFwdType.Decl->addDecl(FlowFwdType.FwdNone);
    FlowFwdType.Decl->addDecl(FlowFwdType.FwdRss);
    FlowFwdType.Decl->addDecl(FlowFwdType.FwdHairpin);
    FlowFwdType.Decl->addDecl(FlowFwdType.FwdPort);
    FlowFwdType.Decl->addDecl(FlowFwdType.FwdPipe);
    FlowFwdType.Decl->addDecl(FlowFwdType.FwdDrop);
    ctx.getTranslationUnitDecl()->addDecl(FlowFwdType.Decl);
    
    FlowFwd.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_fwd"), nullptr);
    FlowFwd.Type = new RecordType(FlowFwd.Decl);
    FlowFwd.FwdType = FieldDecl::Create(ctx, FlowFwd.Decl, &ctx.getIdentifierTable()->get("type"), QualType(FlowFwdType.Type), nullptr);
    PointerType *FlowPipePtr = new PointerType(QualType(FlowPipe.Type));
    FlowFwd.FwdNextPipe = FieldDecl::Create(ctx, FlowFwd.Decl, &ctx.getIdentifierTable()->get("next_pipe"), QualType(FlowPipePtr), nullptr);
    FlowFwd.Decl->addDecl(FlowFwd.FwdType);
    FlowFwd.Decl->addDecl(FlowFwd.FwdNextPipe);
    ctx.getTranslationUnitDecl()->addDecl(FlowFwd.Decl);
    FlowFwd.Decl->setAttrs(attrs);

    /* <-------------------- Pipe Attr --------------------> */
    /* Create flow_pipe_type */
    FlowPipeType.Decl = EnumDecl::Create(ctx, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_pipe_type"), nullptr);
    FlowPipeType.Type = new EnumType(FlowPipeType.Decl);
    FlowPipeType.Basic = EnumConstantDecl::Create(ctx, FlowPipeType.Decl, &ctx.getIdentifierTable()->get("FLOW_PIPE_BASIC"), QualType(ctx.UnsignedIntTy), Zero, APSInt(APInt(IntSize, 0)));
    FlowPipeType.Control = EnumConstantDecl::Create(ctx, FlowPipeType.Decl, &ctx.getIdentifierTable()->get("FLOW_PIPE_CONTROL"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowPipeType.Decl->addDecl(FlowPipeType.Basic);
    FlowPipeType.Decl->addDecl(FlowPipeType.Control);
    ctx.getTranslationUnitDecl()->addDecl(FlowPipeType.Decl);

    /* Create flow_pipe_domain */
    FlowPipeDomain.Decl = EnumDecl::Create(ctx, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_pipe_domain"), nullptr);
    FlowPipeDomain.Type = new EnumType(FlowPipeDomain.Decl);
    FlowPipeDomain.Ingress = EnumConstantDecl::Create(ctx, FlowPipeDomain.Decl, &ctx.getIdentifierTable()->get("FLOW_PIPE_DOMAIN_INGRESS"), QualType(ctx.UnsignedIntTy), Zero, APSInt(APInt(IntSize, 0)));
    FlowPipeDomain.Egress = EnumConstantDecl::Create(ctx, FlowPipeDomain.Decl, &ctx.getIdentifierTable()->get("FLOW_PIPE_DOMAIN_EGRESS"), QualType(ctx.UnsignedIntTy), nullptr, APSInt(APInt(IntSize, 0)));
    FlowPipeDomain.Decl->addDecl(FlowPipeDomain.Ingress);
    FlowPipeDomain.Decl->addDecl(FlowPipeDomain.Egress);
    ctx.getTranslationUnitDecl()->addDecl(FlowPipeDomain.Decl);

    FlowPipeAttr.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_pipe_attr"), nullptr);
    FlowPipeAttr.Type = new RecordType(FlowPipeAttr.Decl);
    PointerType *charptr = new PointerType(QualType(ctx.CharTy));
    FlowPipeAttr.Name = FieldDecl::Create(ctx, FlowPipeAttr.Decl, &ctx.getIdentifierTable()->get("name"), QualType(charptr), nullptr);
    FlowPipeAttr.PipeType = FieldDecl::Create(ctx, FlowPipeAttr.Decl, &ctx.getIdentifierTable()->get("type"), QualType(FlowPipeType.Type), nullptr);
    FlowPipeAttr.Domain = FieldDecl::Create(ctx, FlowPipeAttr.Decl, &ctx.getIdentifierTable()->get("domain"), QualType(FlowPipeDomain.Type), nullptr);
    FlowPipeAttr.IsRoot = FieldDecl::Create(ctx, FlowPipeAttr.Decl, &ctx.getIdentifierTable()->get("is_root"), QualType(ctx.BoolTy), nullptr);
    FlowPipeAttr.NbActions = FieldDecl::Create(ctx, FlowPipeAttr.Decl, &ctx.getIdentifierTable()->get("nb_actions"), QualType(ctx.UnsignedCharTy), nullptr);
    FlowPipeAttr.Decl->addDecl(FlowPipeAttr.Name);
    FlowPipeAttr.Decl->addDecl(FlowPipeAttr.PipeType);
    FlowPipeAttr.Decl->addDecl(FlowPipeAttr.Domain);
    FlowPipeAttr.Decl->addDecl(FlowPipeAttr.IsRoot);
    FlowPipeAttr.Decl->addDecl(FlowPipeAttr.NbActions);
    ctx.getTranslationUnitDecl()->addDecl(FlowPipeAttr.Decl);
    FlowPipeAttr.Decl->setAttrs(attrs);

    /* <-------------------- Pipe Cfg --------------------> */
    FlowPipeCfg.Decl = RecordDecl::Create(ctx, TTK_Struct, ctx.getTranslationUnitDecl(), &ctx.getIdentifierTable()->get("flow_pipe_cfg"), nullptr);
    FlowPipeCfg.Type = new RecordType(FlowPipeCfg.Decl);
    FlowPipeCfg.Attr = FieldDecl::Create(ctx, FlowPipeCfg.Decl, &ctx.getIdentifierTable()->get("attr"), FlowPipeAttr.Type, nullptr);
    PointerType * MatchPtr = new PointerType(QualType(FlowMatch.Type));
    FlowPipeCfg.Match = FieldDecl::Create(ctx, FlowPipeCfg.Decl, &ctx.getIdentifierTable()->get("match"), QualType(MatchPtr), nullptr);
    PointerType * ActionsPtr = new PointerType(QualType(FlowActions.Type));
    PointerType * ActionsPtrPtr = new PointerType(QualType(ActionsPtr));
    FlowPipeCfg.Actions = FieldDecl::Create(ctx, FlowPipeCfg.Decl, &ctx.getIdentifierTable()->get("actions"), QualType(ActionsPtrPtr), nullptr);
    FlowPipeCfg.Decl->addDecl(FlowPipeCfg.Attr);
    FlowPipeCfg.Decl->addDecl(FlowPipeCfg.Match);
    FlowPipeCfg.Decl->addDecl(FlowPipeCfg.Actions);
    ctx.getTranslationUnitDecl()->addDecl(FlowPipeCfg.Decl);
    FlowPipeCfg.Decl->setAttrs(attrs);

    PointerType * CfgPtr = new PointerType(QualType(FlowPipeCfg.Type));
    PointerType * FwdPtr = new PointerType(QualType(FlowFwd.Type));
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    /* <-------------------- Create Pipe --------------------> */
    {
        QualType CreatePipeRetType = QualType(ctx.IntTy);

        std::vector<QualType> CreatePipeArgsType;
        CreatePipeArgsType.push_back(QualType(CfgPtr));
        CreatePipeArgsType.push_back(QualType(FwdPtr));
        CreatePipeArgsType.push_back(QualType(FwdPtr));
        CreatePipeArgsType.push_back(QualType(PipePtr));
        QualType CreatePipeType = ctx.getFunctionType(CreatePipeRetType, CreatePipeArgsType);

        IdentifierInfo *CreatePipeId = &ctx.getIdentifierTable()->get("flow_create_hw_pipe");
        FlowCreatePipe.Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), CreatePipeId, CreatePipeType, SC_Extern);

        std::vector<ParmVarDecl *> FuncDecls;
        IdentifierInfo *CfgId = &ctx.getIdentifierTable()->get("pipe_cfg");
        ParmVarDecl *CfgPtrParmVar = ParmVarDecl::Create(ctx, FlowCreatePipe.Decl, CfgId, QualType(CfgPtr), SC_None);
        IdentifierInfo *FwdId = &ctx.getIdentifierTable()->get("fwd");
        ParmVarDecl *FwdPtrParmVar = ParmVarDecl::Create(ctx, FlowCreatePipe.Decl, FwdId, QualType(FwdPtr), SC_None);
        IdentifierInfo *FwdMissId = &ctx.getIdentifierTable()->get("fwd_miss");
        ParmVarDecl *FwdMissPtrParmVar = ParmVarDecl::Create(ctx, FlowCreatePipe.Decl, FwdMissId, QualType(FwdPtr), SC_None);
        IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get("pipe");
        ParmVarDecl *PipeParmVar = ParmVarDecl::Create(ctx, FlowCreatePipe.Decl, PipeId, QualType(PipePtr), SC_None);
        FuncDecls.push_back(CfgPtrParmVar);
        FuncDecls.push_back(FwdPtrParmVar);
        FuncDecls.push_back(FwdMissPtrParmVar);
        FuncDecls.push_back(PipeParmVar);
        FlowCreatePipe.Decl->setParams(ctx, FuncDecls);
        ctx.getTranslationUnitDecl()->addDecl(FlowCreatePipe.Decl);
    }

    /* <-------------------- Get Pipe --------------------> */
    {
        QualType GetPipeRetType = QualType(PipePtr);
        PointerType *charptr = new PointerType(QualType(ctx.CharTy));
        QualType PipeNameType =  QualType(charptr, QualType::Qualifiers::Const);

        std::vector<QualType> GetPipeArgsType;
        GetPipeArgsType.push_back(PipeNameType);
        QualType GetPipeType = ctx.getFunctionType(GetPipeRetType, GetPipeArgsType);

        IdentifierInfo *GetPipeId = &ctx.getIdentifierTable()->get("flow_get_pipe");
        FlowGetPipeByName.Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), GetPipeId, GetPipeType, SC_Extern);

        std::vector<ParmVarDecl *> FuncDecls;
        IdentifierInfo *PipeNameId = &ctx.getIdentifierTable()->get("pipe_name");
        ParmVarDecl *PipeNameParmVar = ParmVarDecl::Create(ctx, FlowGetPipeByName.Decl, PipeNameId, PipeNameType, SC_None);
        FuncDecls.push_back(PipeNameParmVar);
        FlowGetPipeByName.Decl->setParams(ctx, FuncDecls);
        ctx.getTranslationUnitDecl()->addDecl(FlowGetPipeByName.Decl);
    }

    /* <-------------------- Get Pipe Id By NAme --------------------> */
    {
        QualType GetPipeIdByNameRetType = QualType(ctx.IntTy);
        PointerType *charptr = new PointerType(QualType(ctx.CharTy));
        QualType PipeNameType =  QualType(charptr, QualType::Qualifiers::Const);

        std::vector<QualType> GetPipeIdByNameArgsType;
        GetPipeIdByNameArgsType.push_back(QualType(charptr, QualType::Qualifiers::Const));
        QualType GetPipeIdByNameType = ctx.getFunctionType(GetPipeIdByNameRetType, GetPipeIdByNameArgsType);

        IdentifierInfo *GetPipeIdByNameId = &ctx.getIdentifierTable()->get("flow_get_pipe_id_by_name");
        FlowGetPipeIdByName.Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), GetPipeIdByNameId, GetPipeIdByNameType, SC_Extern);

        std::vector<ParmVarDecl *> FuncDecls;
        IdentifierInfo *PipeNameId = &ctx.getIdentifierTable()->get("pipe_name");
        ParmVarDecl *PipeNameParmVar = ParmVarDecl::Create(ctx, FlowGetPipeIdByName.Decl, PipeNameId, PipeNameType, SC_None);
        FuncDecls.push_back(PipeNameParmVar);
        FlowGetPipeIdByName.Decl->setParams(ctx, FuncDecls);
        ctx.getTranslationUnitDecl()->addDecl(FlowGetPipeIdByName.Decl);
    }

    // /* <-------------------- Get Pipe Id --------------------> */
    // {
    //     QualType GetPipeIdRetType = QualType(ctx.IntTy);
    //     PointerType *charptr = new PointerType(QualType(ctx.CharTy));
    //     QualType PipeNameType =  QualType(charptr, QualType::Qualifiers::Const);

    //     std::vector<QualType> GetPipeIdArgsType;
    //     GetPipeIdArgsType.push_back(QualType(charptr, QualType::Qualifiers::Const));
    //     QualType GetPipeIdType = ctx.getFunctionType(GetPipeIdRetType, GetPipeIdArgsType);

    //     IdentifierInfo *GetPipeIdId = &ctx.getIdentifierTable()->get("flow_get_pipe_id");
    //     FlowGetPipeId.Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), GetPipeIdId, GetPipeIdType, SC_Extern);

    //     std::vector<ParmVarDecl *> FuncDecls;
    //     IdentifierInfo *PipeNameId = &ctx.getIdentifierTable()->get("pipe_name");
    //     ParmVarDecl *PipeNameParmVar = ParmVarDecl::Create(ctx, FlowGetPipeId.Decl, PipeNameId, PipeNameType, SC_None);
    //     FuncDecls.push_back(PipeNameParmVar);
    //     FlowGetPipeId.Decl->setParams(ctx, FuncDecls);
    //     ctx.getTranslationUnitDecl()->addDecl(FlowGetPipeId.Decl);
    // }

    /* <-------------------- Add Control Pipe Entry --------------------> */
    /* Create flow_control_pipe_add_entry */
    {
        QualType ControlPipeAddEntryRetType = QualType(ctx.IntTy);

        std::vector<QualType> ControlPipeAddEntryArgsType;
        ControlPipeAddEntryArgsType.push_back(QualType(PipePtr));
        ControlPipeAddEntryArgsType.push_back(QualType(ctx.UnsignedCharTy));
        ControlPipeAddEntryArgsType.push_back(QualType(MatchPtr));
        ControlPipeAddEntryArgsType.push_back(QualType(ActionsPtr));
        ControlPipeAddEntryArgsType.push_back(QualType(FwdPtr));
        QualType ControlPipeAddEntryType = ctx.getFunctionType(ControlPipeAddEntryRetType, ControlPipeAddEntryArgsType);

        IdentifierInfo *ControlPipeAddEntryId = &ctx.getIdentifierTable()->get("flow_hw_control_pipe_add_entry");
        FlowControlPipeAddEntry.Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), ControlPipeAddEntryId, ControlPipeAddEntryType, SC_Extern);

        std::vector<ParmVarDecl *> FuncDecls;
        IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get("pipe");
        ParmVarDecl *CurrentPipePtrParmVar = ParmVarDecl::Create(ctx, FlowControlPipeAddEntry.Decl, PipeId, QualType(PipePtr), SC_None);
        IdentifierInfo *PriorityId = &ctx.getIdentifierTable()->get("priority");
        ParmVarDecl *PriorityParmVar = ParmVarDecl::Create(ctx, FlowControlPipeAddEntry.Decl, PriorityId, QualType(ctx.UnsignedCharTy), SC_None);
        IdentifierInfo *MatchPtrId = &ctx.getIdentifierTable()->get("match");
        ParmVarDecl *MatchPtrParmVar = ParmVarDecl::Create(ctx, FlowControlPipeAddEntry.Decl, MatchPtrId, QualType(MatchPtr), SC_None);
        IdentifierInfo *ActionsId = &ctx.getIdentifierTable()->get("actions");
        ParmVarDecl *ActionsParmVar = ParmVarDecl::Create(ctx, FlowControlPipeAddEntry.Decl, ActionsId, QualType(ActionsPtr), SC_None);
        IdentifierInfo *FwdId = &ctx.getIdentifierTable()->get("fwd");
        ParmVarDecl *FwdParmVar = ParmVarDecl::Create(ctx, FlowControlPipeAddEntry.Decl, FwdId, QualType(FwdPtr), SC_None);

        FuncDecls.push_back(CurrentPipePtrParmVar);
        FuncDecls.push_back(PriorityParmVar);
        FuncDecls.push_back(MatchPtrParmVar);
        FuncDecls.push_back(ActionsParmVar);
        FuncDecls.push_back(FwdParmVar);

        FlowControlPipeAddEntry.Decl->setParams(ctx, FuncDecls);
        ctx.getTranslationUnitDecl()->addDecl(FlowControlPipeAddEntry.Decl);
    }

    /* <-------------------- Add Pipe Entry --------------------> */
    {
        PointerType * ActionPtr = new PointerType(QualType(FlowActions.Type));

        QualType PipeAddEntryRetType = QualType(ctx.IntTy);

        std::vector<QualType> PipeAddEntryArgsType;
        PipeAddEntryArgsType.push_back(QualType(PipePtr));
        PipeAddEntryArgsType.push_back(QualType(MatchPtr));
        PipeAddEntryArgsType.push_back(QualType(ActionPtr));
        PipeAddEntryArgsType.push_back(QualType(FwdPtr));
        QualType PipeAddEntryType = ctx.getFunctionType(PipeAddEntryRetType, PipeAddEntryArgsType);

        IdentifierInfo *PipeAddEntryId = &ctx.getIdentifierTable()->get("flow_hw_pipe_add_entry");
        FlowPipeAddEntry.Decl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeAddEntryId, PipeAddEntryType, SC_Extern);

        std::vector<ParmVarDecl *> FuncDecls;
        IdentifierInfo *CurrentPipeId = &ctx.getIdentifierTable()->get("pipe");
        ParmVarDecl *CurrentPipePtrParmVar = ParmVarDecl::Create(ctx, FlowPipeAddEntry.Decl, CurrentPipeId, QualType(PipePtr), SC_None);
        IdentifierInfo *MatchPtrId = &ctx.getIdentifierTable()->get("match");
        ParmVarDecl *MatchPtrParmVar = ParmVarDecl::Create(ctx, FlowPipeAddEntry.Decl, MatchPtrId, QualType(MatchPtr), SC_None);
        IdentifierInfo *ActionsPtrId = &ctx.getIdentifierTable()->get("actions");
        ParmVarDecl *ActionsPtrParmVar = ParmVarDecl::Create(ctx, FlowPipeAddEntry.Decl, ActionsPtrId, QualType(ActionPtr), SC_None);
        IdentifierInfo *FwdId = &ctx.getIdentifierTable()->get("fwd");
        ParmVarDecl *FwdParmVar = ParmVarDecl::Create(ctx, FlowPipeAddEntry.Decl, FwdId, QualType(FwdPtr), SC_None);

        FuncDecls.push_back(CurrentPipePtrParmVar);
        FuncDecls.push_back(MatchPtrParmVar);
        FuncDecls.push_back(ActionsPtrParmVar);
        FuncDecls.push_back(FwdParmVar);

        FlowPipeAddEntry.Decl->setParams(ctx, FuncDecls);
        ctx.getTranslationUnitDecl()->addDecl(FlowPipeAddEntry.Decl);
    }

    return true;
}

bool DocaProgram::initPipe(Pipeline::Kind K, const CallGraphNodeBase* node) {
    if (const auto* actionNode = dynamic_cast<const ActionNode*>(node)) {
        std::cout << "  Generating initialization for " <<  "\033[32m" << " action " << "\033[0m" << " pipe: " << actionNode->getName();
        buildAddActionPipeEntry(actionNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    } else if (const auto* tableNode = dynamic_cast<const TableNode*>(node)) {
        std::cout << "  Generating initialization for " <<  "\033[32m" << " table " << "\033[0m" << " pipe: " << tableNode->getName();
        // initTablePipe(tableNode);
        buildAddTablePipeEntry(tableNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    } else if (const auto* conditionalNode = dynamic_cast<const ConditionalNode*>(node)) {
        std::cout << "  Generating initialization for " <<  "\033[32m" << " conditional " << "\033[0m" << " pipe: " << conditionalNode->getName();
        initConditionalPipe(conditionalNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    } else {
        std::cerr << "  Unknown node type" << std::endl;
        return false;
    }

    return true;
}

bool DocaProgram::buildPipe(Pipeline::Kind K, const CallGraphNodeBase* node, bool isRoot) {
    if (const auto* actionNode = dynamic_cast<const ActionNode*>(node)) {
        std::cout << "  Generating creation for " <<  "\033[32m" << " action " << "\033[0m" << " pipe: " << actionNode->getName(); 
        buildActionPipe(K, actionNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    } else if (const auto* tableNode = dynamic_cast<const TableNode*>(node)) {
        std::cout << "  Generating creation for " <<  "\033[32m" << " table " << "\033[0m" << " pipe: " << tableNode->getName();
        buildTablePipe(K, tableNode, isRoot);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    } else if (const auto* conditionalNode = dynamic_cast<const ConditionalNode*>(node)) {
        std::cout << "  Generating creation for " <<  "\033[32m" << " conditional " << "\033[0m" << " pipe: " << conditionalNode->getName();
        buildConditionalPipe(K, conditionalNode, isRoot);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
        // Add logic specific to ConditionalNode here
    } else {
        std::cerr << "  Unknown node type" << std::endl;
        return false;
    }

    return true;
}

bool DocaProgram::buildIngressActionPipe(const ActionNode* actionNode) {
    Action *action = actionNode->data;
    std::string pipeName = absl::StrCat(action->getName(), "_", action->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');
    std::vector<Primitive> primitives = action->getPrimitives();

    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    std::vector<Stmt *> Stmts;

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );
    
    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    // ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Create SET_MAC_ADDR */
    DECLARE_EXTERN_FUNC(ctx, SetMacAddr, "SET_MAC_ADDR", ctx.IntTy,  QualType(ctx.VoidPtrTy), 
                    QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), 
                    QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Actions Array */
    PointerType * ActionsPtr = new PointerType(QualType(FlowActions.Type));
    ConstantArrayType *ActionsArrType = new ConstantArrayType(QualType(ActionsPtr), APInt(APInt::APINT_BITS_PER_WORD, 1));
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, ActionsArr, "actions_arr", QualType(ActionsArrType), Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* Fwd Miss */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, FwdMiss, "fwd_miss", FlowFwd.Type, Stmts);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, VARREF(PipeCfg), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(PipeCfg)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(PipeCfg), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    /* memset(&fwd_miss,0,sizeof(fwd_miss)) */
    UnaryOperator *GetFwdMissAddr = UnaryOperator::Create(ctx, VARREF(FwdMiss), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(FwdMiss)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdMissSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(FwdMiss), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdMissAddr, Zero, GetFwdMissSize);

    /* actions_arr[0] = &actions */
    ArraySubscriptExpr *ActionsArrayElement = ArraySubscriptExpr::Create(ctx, ActionsArrVarRef, Zero, QualType(ctx.UnsignedIntTy), VK_PRValue, OK_Ordinary);
    BinaryOperator *ActionsArrayElementAssign = BinaryOperator::Create(ctx, ActionsArrayElement, GetActionsAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsArrayElementAssign);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeBasicId = const_cast<IdentifierInfo*>(FlowPipeType.Basic->getIdentifier());
    DeclRefExpr *PipeTypeBasicRef = DeclRefExpr::Create(ctx, FlowPipeType.Basic, PipeTypeBasicId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeBasicRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NonRoot = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, NonRoot, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    /* Set cfg.attr.domain */
    MemberExpr *AttrDomain = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Domain->getDeclName(), FlowPipeAttr.Domain->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    DeclRefExpr *PipeDomainRef = nullptr;
    IdentifierInfo *PipeDomainIngressId = const_cast<IdentifierInfo*>(FlowPipeDomain.Ingress->getIdentifier());
    PipeDomainRef = DeclRefExpr::Create(ctx, FlowPipeDomain.Ingress, PipeDomainIngressId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrDomainAssign = BinaryOperator::Create(ctx, AttrDomain, PipeDomainRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrDomainAssign);

    /* Set cfg.match */
    // IdentifierInfo *CfgMatchId = const_cast<IdentifierInfo*>(FlowPipeCfg.Match->getIdentifier());
    MemberExpr *CfgMatch = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Match->getDeclName(), FlowPipeCfg.Match->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeMatchAssign = BinaryOperator::Create(ctx, CfgMatch, GetMatchAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeMatchAssign);

    /* Set cfg.actions */
    MemberExpr *CfgActions = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Actions->getDeclName(), FlowPipeCfg.Actions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeActionsAssign = BinaryOperator::Create(ctx, CfgActions, ActionsArrVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeActionsAssign);

    /* Set cfg.attr.nb_actions */
    MemberExpr *AttrNbActions = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.NbActions->getDeclName(), FlowPipeAttr.NbActions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NbActions = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
    BinaryOperator *FlowPipeAttrNbActionsAssign = BinaryOperator::Create(ctx, AttrNbActions, NbActions, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNbActionsAssign);

    /* Set match.meta.pkt_meta */
    // MemberExpr *MatchMetaAttr = MemberExpr::Create(ctx, MatchVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // MemberExpr *MetaPktmetaName = MemberExpr::Create(ctx, MatchMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IntegerLiteral *MetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    // BinaryOperator *MatchMetaAssign = BinaryOperator::Create(ctx, MetaPktmetaName, MetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    // Stmts.push_back(MatchMetaAssign);

    /* Set actions.meta.pkt_meta */
    // MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IntegerLiteral *ActionMetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    // BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, ActionMetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    // Stmts.push_back(ActionsMetaAssign);
    CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, PipeName);
    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;
    bool fwdUndefined = true;

    for (auto primitive : primitives) {
        // primitive.print();
        auto op = primitive.getOp();
        /*if (op == "mark_to_pass") {
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
            DeclRefExpr *FwdRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else */if (op == "mark_to_drop") {
            /* Set fwd to DROP */
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypeDropId = const_cast<IdentifierInfo*>(FlowFwdType.FwdDrop->getIdentifier());
            DeclRefExpr *FwdDropRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdDrop, FwdTypeDropId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdDropRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "mark_to_rss") {
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
            DeclRefExpr *FwdRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "mark_to_hairpin") {
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypeHairpinId = const_cast<IdentifierInfo*>(FlowFwdType.FwdHairpin->getIdentifier());
            DeclRefExpr *FwdHairpinRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdHairpin, FwdTypeHairpinId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdHairpinRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "assign") {
            QualType ActionFieldType;
            auto parameters = primitive.getParameter();
            auto leftParam = parameters[0];
            std::shared_ptr<FieldParameter> fieldParam = std::dynamic_pointer_cast<FieldParameter>(leftParam);
            std::vector<std::string> fields = fieldParam->value();
            ActionStateMachine stateMachine;
            MemberExpr *ActionFieldExpr = stateMachine.parseTarget(this, Stmts, fields, ActionsVarRef, &ActionFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
            auto type = ActionFieldType.getTypePtr();
            if (/*auto * constantArrayType = */dynamic_cast<ConstantArrayType *>(type)) {
                // IntegerLiteral *SetMacAddrMask = IntegerLiteral::Create(ctx, APInt(1, 0xff), IntegerLiteral::Hex, QualType(ctx.UnsignedCharTy));
                // std::vector<Expr *> SetMacAddrArgs;
                // SetMacAddrArgs.push_back(ActionFieldExpr);
                // for (int i = 0; i < 6; i++) {
                //     SetMacAddrArgs.push_back(SetMacAddrMask);
                // }
                // CallExpr *MemcpyArrayCall = CallExpr::Create(ctx, SetMacAddrRef, SetMacAddrArgs, FuncDecl->getReturnType(), ExprValueKind::VK_PRValue);
                // Stmts.push_back(MemcpyArrayCall);

                IntegerLiteral *SetMacAddrMask = IntegerLiteral::Create(ctx, APInt(1, 0xff), IntegerLiteral::Hex, QualType(ctx.UnsignedCharTy));
                CALL_FUNC_AND_STMT(ctx, FuncDecl, SetMacAddr, Stmts, ActionFieldExpr, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask);
            } else {
                IntegerLiteral *ActionFieldMask = nullptr;
                uint32_t bitWidth = ActionFieldType.getTypeWidth() * 8;
                uint64_t mask = (1ULL << bitWidth) - 1;
                ActionFieldMask = IntegerLiteral::Create(ctx, APInt(ActionFieldType.getTypeWidth(), mask), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
                BinaryOperator *FlowActionAssign = BinaryOperator::Create(ctx, ActionFieldExpr, ActionFieldMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
                Stmts.push_back(FlowActionAssign);
            }
        }
    }

    /* Set fwd_miss to RSS */
    MemberExpr *FwdMissFwdType = MemberExpr::Create(ctx, FwdMissVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdMissTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
    DeclRefExpr *FwdMissRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdMissTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdMissRssAssign = BinaryOperator::Create(ctx, FwdMissFwdType, FwdMissRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdMissRssAssign);

    /* Flush to hairpin */  
    if (fwdUndefined) {
        MemberExpr *FwdFwdType = MemberExpr::Create(ctx, VARREF(Fwd), false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IdentifierInfo *FwdTypeHairpinId = const_cast<IdentifierInfo*>(FlowFwdType.FwdHairpin->getIdentifier());
        DeclRefExpr *FwdHairpinRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdHairpin, FwdTypeHairpinId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
        BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdHairpinRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FwdTypeAssign);
    }

    /* flow_create_pipe(&pipe_cfg,pipe) */
   CALL_FUNC_WITH_REF(ctx, FlowCreatePipe, CreatePipe, GetPipeCfgAddr, GetFwdAddr, GetFwdMissAddr, REF(ArgPipe));

    ReturnStmt *Result = ReturnStmt::Create(ctx, CALL(CreatePipe));
    Stmts.push_back(Result);
    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json ops = {
        {"create_pipe", funcName}
    };
    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops}
    });

    return true;
}

bool DocaProgram::buildEgressActionPipe(const ActionNode* actionNode) {
    Action *action = actionNode->data;
    std::string pipeName = absl::StrCat(action->getName(), "_", action->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');
    std::vector<Primitive> primitives = action->getPrimitives();

    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    std::vector<Stmt *> Stmts;

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    // DeclRefExpr *PipeVarRef = DeclRefExpr::Create(ctx, PipeVar, PipeId, QualType(PipePtr), ExprValueKind::VK_LValue);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );
    
    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Create SET_MAC_ADDR */
    DECLARE_EXTERN_FUNC(ctx, SetMacAddr, "SET_MAC_ADDR", ctx.IntTy,  QualType(ctx.VoidPtrTy), 
                    QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), 
                    QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Actions Array */
    PointerType * ActionsPtr = new PointerType(QualType(FlowActions.Type));
    ConstantArrayType *ActionsArrType = new ConstantArrayType(QualType(ActionsPtr), APInt(APInt::APINT_BITS_PER_WORD, 1));
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, ActionsArr, "actions_arr", QualType(ActionsArrType), Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, VARREF(PipeCfg), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(PipeCfg)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(PipeCfg), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    /* actions_arr[0] = &actions */
    ArraySubscriptExpr *ActionsArrayElement = ArraySubscriptExpr::Create(ctx, ActionsArrVarRef, Zero, QualType(ctx.UnsignedIntTy), VK_PRValue, OK_Ordinary);
    BinaryOperator *ActionsArrayElementAssign = BinaryOperator::Create(ctx, ActionsArrayElement, GetActionsAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsArrayElementAssign);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeBasicId = const_cast<IdentifierInfo*>(FlowPipeType.Basic->getIdentifier());
    DeclRefExpr *PipeTypeBasicRef = DeclRefExpr::Create(ctx, FlowPipeType.Basic, PipeTypeBasicId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeBasicRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NonRoot = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, NonRoot, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    /* Set cfg.attr.domain */
    MemberExpr *AttrDomain = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Domain->getDeclName(), FlowPipeAttr.Domain->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    DeclRefExpr *PipeDomainRef = nullptr;
    IdentifierInfo *PipeDomainEgressId = const_cast<IdentifierInfo*>(FlowPipeDomain.Egress->getIdentifier());
    PipeDomainRef = DeclRefExpr::Create(ctx, FlowPipeDomain.Egress, PipeDomainEgressId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrDomainAssign = BinaryOperator::Create(ctx, AttrDomain, PipeDomainRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrDomainAssign);

    /* Set cfg.match */
    // IdentifierInfo *CfgMatchId = const_cast<IdentifierInfo*>(FlowPipeCfg.Match->getIdentifier());
    MemberExpr *CfgMatch = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Match->getDeclName(), FlowPipeCfg.Match->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeMatchAssign = BinaryOperator::Create(ctx, CfgMatch, GetMatchAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeMatchAssign);

    /* Set cfg.actions */
    MemberExpr *CfgActions = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Actions->getDeclName(), FlowPipeCfg.Actions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeActionsAssign = BinaryOperator::Create(ctx, CfgActions, ActionsArrVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeActionsAssign);

    /* Set cfg.attr.nb_actions */
    MemberExpr *AttrNbActions = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.NbActions->getDeclName(), FlowPipeAttr.NbActions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NbActions = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
    BinaryOperator *FlowPipeAttrNbActionsAssign = BinaryOperator::Create(ctx, AttrNbActions, NbActions, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNbActionsAssign);

    /* Set match.meta.pkt_meta */
    MemberExpr *MatchMetaAttr = MemberExpr::Create(ctx, MatchVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *MetaPktmetaName = MemberExpr::Create(ctx, MatchMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *MetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    BinaryOperator *MatchMetaAssign = BinaryOperator::Create(ctx, MetaPktmetaName, MetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(MatchMetaAssign);

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;
    bool fwdUndefined = true;

    for (auto primitive : primitives) {
        // primitive.print();
        auto op = primitive.getOp();
        if (op == "mark_to_pass") {
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypePortId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPort->getIdentifier());
            DeclRefExpr *FwdPortRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPort, FwdTypePortId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPortRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "mark_to_drop") {
            /* Set fwd to DROP */
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypeDropId = const_cast<IdentifierInfo*>(FlowFwdType.FwdDrop->getIdentifier());
            DeclRefExpr *FwdDropRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdDrop, FwdTypeDropId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdDropRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "mark_to_rss") {
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
            DeclRefExpr *FwdRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "mark_to_fwd_port") {
            MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
            IdentifierInfo *FwdTypePortId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPort->getIdentifier());
            DeclRefExpr *FwdPortRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPort, FwdTypePortId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
            BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPortRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FwdTypeAssign);
            fwdUndefined = false;
        } else if (op == "assign") {
            QualType ActionFieldType;
            auto parameters = primitive.getParameter();
            auto leftParam = parameters[0];
            std::shared_ptr<FieldParameter> fieldParam = std::dynamic_pointer_cast<FieldParameter>(leftParam);
            std::vector<std::string> fields = fieldParam->value();
            ActionStateMachine stateMachine;
            MemberExpr *ActionFieldExpr = stateMachine.parseTarget(this, Stmts, fields, ActionsVarRef, &ActionFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
            auto type = ActionFieldType.getTypePtr();
            if (/*auto * constantArrayType = */dynamic_cast<ConstantArrayType *>(type)) {
                // IntegerLiteral *SetMacAddrMask = IntegerLiteral::Create(ctx, APInt(1, 0xff), IntegerLiteral::Hex, QualType(ctx.UnsignedCharTy));
                // CALL_FUNC_AND_STMT(ctx, FuncDecl, SetMacAddr, Stmts, REF(SetMacAddr), 
                //                         SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask);
                IntegerLiteral *SetMacAddrMask = IntegerLiteral::Create(ctx, APInt(1, 0xff), IntegerLiteral::Hex, QualType(ctx.UnsignedCharTy));
                CALL_FUNC_AND_STMT(ctx, FuncDecl, SetMacAddr, Stmts, ActionFieldExpr, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask);
            } else {
                IntegerLiteral *ActionFieldMask = nullptr;
                uint32_t bitWidth = ActionFieldType.getTypeWidth() * 8;
                uint64_t mask = (1ULL << bitWidth) - 1;
                ActionFieldMask = IntegerLiteral::Create(ctx, APInt(ActionFieldType.getTypeWidth(), mask), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
                BinaryOperator *FlowActionAssign = BinaryOperator::Create(ctx, ActionFieldExpr, ActionFieldMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
                Stmts.push_back(FlowActionAssign);
            }
        }
    }

    /* Set actions.meta.pkt_meta */
    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *ActionMetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, ActionMetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    if (fwdUndefined) {
        MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IdentifierInfo *FwdTypePortId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPort->getIdentifier());
        DeclRefExpr *FwdPortRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPort, FwdTypePortId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
        BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPortRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FwdTypeAssign);
    }

    /* flow_create_pipe(&pipe_cfg,pipe) */
    CALL_FUNC_WITH_REF_AND_STMT(ctx, FlowCreatePipe, CreatePipe, Stmts, GetPipeCfgAddr, GetFwdAddr, Zero, REF(ArgPipe));

    Stmts.push_back(Result);
    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json ops = {
        {"create_pipe", funcName}
    };
    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops}
    });

    return true;
}

bool DocaProgram::buildActionPipe(Pipeline::Kind K, const ActionNode* actionNode) {
    bool ret = false;
    switch (K)
    {
    case Pipeline::Kind::INGRESS:
        ret = buildIngressActionPipe(actionNode);
        break;

    case Pipeline::Kind::EGRESS:
        ret = buildEgressActionPipe(actionNode);
        break;
    
    default:
        break;
    }

    return ret;
}

bool DocaProgram::buildIngressSingleActionTablePipe(const TableNode* tableNode, bool isRoot) {
    std::vector<Stmt *> Stmts;

    Table *table = tableNode->data;
    std::string pipeName = absl::StrCat(table->getName(), "_", table->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Actions Array */
    PointerType * ActionsPtr = new PointerType(QualType(FlowActions.Type));
    ConstantArrayType *ActionsArrType = new ConstantArrayType(QualType(ActionsPtr), APInt(APInt::APINT_BITS_PER_WORD, 1));
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, ActionsArr, "actions_arr", QualType(ActionsArrType), Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* Fwd Miss */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, FwdMiss, "fwd_miss", FlowFwd.Type, Stmts);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, VARREF(PipeCfg), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(PipeCfg)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(PipeCfg), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(FwdMiss)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    /* memset(&fwd_miss,0,sizeof(fwd_miss)) */
    UnaryOperator *GetFwdMissAddr = UnaryOperator::Create(ctx, VARREF(FwdMiss), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(FwdMiss)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdMissSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(FwdMiss), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdMissAddr, Zero, GetFwdMissSize);

    /* actions_arr[0] = &actions */
    ArraySubscriptExpr *ActionsArrayElement = ArraySubscriptExpr::Create(ctx, ActionsArrVarRef, Zero, QualType(ctx.UnsignedIntTy), VK_PRValue, OK_Ordinary);
    BinaryOperator *ActionsArrayElementAssign = BinaryOperator::Create(ctx, ActionsArrayElement, GetActionsAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsArrayElementAssign);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    // IdentifierInfo *CfgAttrId = const_cast<IdentifierInfo*>(FlowPipeCfg.Attr->getIdentifier());
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IdentifierInfo *AttrNameId = const_cast<IdentifierInfo*>(FlowPipeAttr.Name->getIdentifier());
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    // IdentifierInfo *AttrTypeId = const_cast<IdentifierInfo*>(FlowPipeAttr.PipeType->getIdentifier());
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeBasiclId = const_cast<IdentifierInfo*>(FlowPipeType.Basic->getIdentifier());
    DeclRefExpr *PipeTypeBasicRef = DeclRefExpr::Create(ctx, FlowPipeType.Basic, PipeTypeBasiclId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeBasicRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *IsRoot = (isRoot)? IntegerLiteral::Create(ctx, APInt(1, 1), IntegerLiteral::Dec, QualType(ctx.BoolTy)) :
                                IntegerLiteral::Create(ctx, APInt(1, 0), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, IsRoot, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    /* Set cfg.attr.domain */
    MemberExpr *AttrDomain = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Domain->getDeclName(), FlowPipeAttr.Domain->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    DeclRefExpr *PipeDomainRef = nullptr;
    IdentifierInfo *PipeDomainIngressId = const_cast<IdentifierInfo*>(FlowPipeDomain.Ingress->getIdentifier());
    PipeDomainRef = DeclRefExpr::Create(ctx, FlowPipeDomain.Ingress, PipeDomainIngressId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrDomainAssign = BinaryOperator::Create(ctx, AttrDomain, PipeDomainRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrDomainAssign);

    /* Set cfg.match */
    MemberExpr *CfgMatch = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Match->getDeclName(), FlowPipeCfg.Match->getType(), ExprValueKind::VK_LValue, OK_Ordinary);    
    BinaryOperator *FlowPipeMatchAssign = BinaryOperator::Create(ctx, CfgMatch, GetMatchAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeMatchAssign);

    Expr *NullPtr = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.VoidPtrTy));

    /* Set cfg.actions */
    MemberExpr *CfgActions = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Actions->getDeclName(), FlowPipeCfg.Actions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeActionsAssign = BinaryOperator::Create(ctx, CfgActions, ActionsArrVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeActionsAssign);

    /* Set cfg.attr.nb_actions */
    MemberExpr *AttrNbActions = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.NbActions->getDeclName(), FlowPipeAttr.NbActions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NbActions = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
    BinaryOperator *FlowPipeAttrNbActionsAssign = BinaryOperator::Create(ctx, AttrNbActions, NbActions, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNbActionsAssign);

    /* Set match.meta.pkt_meta */
    MemberExpr *MatchMetaAttr = MemberExpr::Create(ctx, MatchVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *MetaPktmetaName = MemberExpr::Create(ctx, MatchMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *MetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    BinaryOperator *MatchMetaAssign = BinaryOperator::Create(ctx, MetaPktmetaName, MetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(MatchMetaAssign);

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;

    /* Set match fields */
    for (auto key : table->getKeys()) {
        MatchStateMachine stateMachine;
        QualType MatchFieldType;
        MemberExpr *MatchFieldExpr = stateMachine.parseTarget(this, Stmts, key.target, MatchVarRef, &MatchFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
        IntegerLiteral *MatchFieldMask = nullptr;
        if (key.match_type == "exact") {
            uint32_t bitWidth = MatchFieldType.getTypeWidth() * 8;
            uint64_t mask = (1ULL << bitWidth) - 1;
            MatchFieldMask = IntegerLiteral::Create(ctx, APInt(MatchFieldType.getTypeWidth(), mask), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
        }
        BinaryOperator *FlowMatchAssign = BinaryOperator::Create(ctx, MatchFieldExpr, MatchFieldMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FlowMatchAssign);
    }

    /* Set actions.meta.pkt_meta */
    const Table::ActionEntry* defaultAction = table->getDefaultActionEntry();
    std::vector<Expr *> GetActionPipeIdArgs;
    std::string defaultActionName = absl::StrCat(defaultAction->actionName(), "_", defaultAction->actionId());
    std::replace(defaultActionName.begin(), defaultActionName.end(), '.', '_');
    ConstantArrayType * defaultActionNameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, defaultAction->actionName().size()));
    QualType defaultActionNameStrTy = QualType(defaultActionNameArray);
    CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, StringLiteral::Create(ctx, defaultActionName, StringLiteral::Ascii, defaultActionNameStrTy));

    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    /* set fwd to pipe */
    MemberExpr *FwdFwdType = MemberExpr::Create(ctx, VARREF(Fwd), false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdTypePipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
    DeclRefExpr *FwdPipeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypePipeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPipeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdTypeAssign);

    CALL_FUNC_WITH_REF(ctx, FlowGetPipeByName, GetTablePipe, StringLiteral::Create(ctx, defaultActionName, StringLiteral::Ascii, defaultActionNameStrTy));

    MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, VARREF(Fwd), false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, CALL(GetTablePipe), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdNextPipeAssign);

    /* set fwd_miss to RSS pipe */
    MemberExpr *FwdMissFwdType = MemberExpr::Create(ctx, FwdMissVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdMissTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
    DeclRefExpr *FwdMissRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdMissTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdMissTypeAssign = BinaryOperator::Create(ctx, FwdMissFwdType, FwdMissRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdMissTypeAssign);

    /* flow_create_pipe(&pipe_cfg,pipe) */
    std::vector<Expr *> CreatePipeArgs;
    CreatePipeArgs.push_back(GetPipeCfgAddr);
    CreatePipeArgs.push_back(GetFwdAddr);
    CreatePipeArgs.push_back(GetFwdMissAddr);
    CreatePipeArgs.push_back(REF(ArgPipe));
    IdentifierInfo *CreatePipeId = const_cast<IdentifierInfo*>(FlowCreatePipe.Decl->getIdentifier());
    DeclRefExpr *CreatePipeRef = DeclRefExpr::Create(ctx, FlowCreatePipe.Decl, CreatePipeId, FlowCreatePipe.Type, ExprValueKind::VK_PRValue);
    CallExpr *CreatePipeCall = CallExpr::Create(ctx, CreatePipeRef, CreatePipeArgs, FlowCreatePipe.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    Stmts.push_back(CreatePipeCall);

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json actions;
    for (const auto& action : table->getActions()) {
        actions["actions"].push_back({
            {"name", action.actionName()},
            {"id", action.actionId()}
        });
    }

    json ops = {
        {"create_pipe", funcName}
    };

    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops},
        {"actions", actions["actions"]}
    });

    return true;
}

bool DocaProgram::buildIngressMultiActionTablePipe(const TableNode* tableNode, bool isRoot) {
    std::vector<Stmt *> Stmts;

    Table *table = tableNode->data;
    std::string pipeName = absl::StrCat(table->getName(), "_", table->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Actions Array */
    PointerType * ActionsPtr = new PointerType(QualType(FlowActions.Type));
    ConstantArrayType *ActionsArrType = new ConstantArrayType(QualType(ActionsPtr), APInt(APInt::APINT_BITS_PER_WORD, 1));
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, ActionsArr, "actions_arr", QualType(ActionsArrType), Stmts);

    /* Fwd Miss */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, FwdMiss, "fwd_miss", FlowFwd.Type, Stmts);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, VARREF(PipeCfg), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(PipeCfg)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(PipeCfg), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd_miss,0,sizeof(fwd_miss)) */
    UnaryOperator *GetFwdMissAddr = UnaryOperator::Create(ctx, VARREF(FwdMiss), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(FwdMiss)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdMissSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(FwdMiss), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdMissAddr, Zero, GetFwdMissSize);

    /* actions_arr[0] = &actions */
    ArraySubscriptExpr *ActionsArrayElement = ArraySubscriptExpr::Create(ctx, ActionsArrVarRef, Zero, QualType(ctx.UnsignedIntTy), VK_PRValue, OK_Ordinary);
    BinaryOperator *ActionsArrayElementAssign = BinaryOperator::Create(ctx, ActionsArrayElement, GetActionsAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsArrayElementAssign);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    // IdentifierInfo *CfgAttrId = const_cast<IdentifierInfo*>(FlowPipeCfg.Attr->getIdentifier());
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IdentifierInfo *AttrNameId = const_cast<IdentifierInfo*>(FlowPipeAttr.Name->getIdentifier());
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    // IdentifierInfo *AttrTypeId = const_cast<IdentifierInfo*>(FlowPipeAttr.PipeType->getIdentifier());
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeBasiclId = const_cast<IdentifierInfo*>(FlowPipeType.Basic->getIdentifier());
    DeclRefExpr *PipeTypeBasicRef = DeclRefExpr::Create(ctx, FlowPipeType.Basic, PipeTypeBasiclId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeBasicRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *IsRoot = (isRoot)? IntegerLiteral::Create(ctx, APInt(1, 1), IntegerLiteral::Dec, QualType(ctx.BoolTy)) :
                                IntegerLiteral::Create(ctx, APInt(1, 0), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, IsRoot, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    /* Set cfg.attr.domain */
    MemberExpr *AttrDomain = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Domain->getDeclName(), FlowPipeAttr.Domain->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    DeclRefExpr *PipeDomainRef = nullptr;
    IdentifierInfo *PipeDomainIngressId = const_cast<IdentifierInfo*>(FlowPipeDomain.Ingress->getIdentifier());
    PipeDomainRef = DeclRefExpr::Create(ctx, FlowPipeDomain.Ingress, PipeDomainIngressId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrDomainAssign = BinaryOperator::Create(ctx, AttrDomain, PipeDomainRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrDomainAssign);

    /* Set cfg.match */
    MemberExpr *CfgMatch = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Match->getDeclName(), FlowPipeCfg.Match->getType(), ExprValueKind::VK_LValue, OK_Ordinary);    
    BinaryOperator *FlowPipeMatchAssign = BinaryOperator::Create(ctx, CfgMatch, GetMatchAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeMatchAssign);

    Expr *NullPtr = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.VoidPtrTy));

    /* Set cfg.actions */
    MemberExpr *CfgActions = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Actions->getDeclName(), FlowPipeCfg.Actions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeActionsAssign = BinaryOperator::Create(ctx, CfgActions, ActionsArrVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeActionsAssign);

    /* Set cfg.attr.nb_actions */
    MemberExpr *AttrNbActions = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.NbActions->getDeclName(), FlowPipeAttr.NbActions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NbActions = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
    BinaryOperator *FlowPipeAttrNbActionsAssign = BinaryOperator::Create(ctx, AttrNbActions, NbActions, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNbActionsAssign);

    /* Set match.meta.pkt_meta */
    MemberExpr *MatchMetaAttr = MemberExpr::Create(ctx, MatchVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *MetaPktmetaName = MemberExpr::Create(ctx, MatchMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *MetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    BinaryOperator *MatchMetaAssign = BinaryOperator::Create(ctx, MetaPktmetaName, MetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(MatchMetaAssign);

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;

    /* Set match fields */
    for (auto key : table->getKeys()) {
        MatchStateMachine stateMachine;
        QualType MatchFieldType;
        MemberExpr *MatchFieldExpr = stateMachine.parseTarget(this, Stmts, key.target, MatchVarRef, &MatchFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
        IntegerLiteral *MatchFieldMask = nullptr;
        if (key.match_type == "exact") {
            uint32_t bitWidth = MatchFieldType.getTypeWidth() * 8;
            uint64_t mask = (1ULL << bitWidth) - 1;
            MatchFieldMask = IntegerLiteral::Create(ctx, APInt(MatchFieldType.getTypeWidth(), mask), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
        }
        BinaryOperator *FlowMatchAssign = BinaryOperator::Create(ctx, MatchFieldExpr, MatchFieldMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FlowMatchAssign);
    }

    /* Set actions.meta.pkt_meta */
    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *ActionMetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, ActionMetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    /* set fwd_miss to RSS pipe */
    MemberExpr *FwdMissFwdType = MemberExpr::Create(ctx, FwdMissVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdMissTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
    DeclRefExpr *FwdMissRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdMissTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdMissTypeAssign = BinaryOperator::Create(ctx, FwdMissFwdType, FwdMissRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdMissTypeAssign);

    /* flow_create_pipe(&pipe_cfg,pipe) */
    std::vector<Expr *> CreatePipeArgs;
    CreatePipeArgs.push_back(GetPipeCfgAddr);
    CreatePipeArgs.push_back(NullPtr);
    CreatePipeArgs.push_back(GetFwdMissAddr);
    CreatePipeArgs.push_back(REF(ArgPipe));
    IdentifierInfo *CreatePipeId = const_cast<IdentifierInfo*>(FlowCreatePipe.Decl->getIdentifier());
    DeclRefExpr *CreatePipeRef = DeclRefExpr::Create(ctx, FlowCreatePipe.Decl, CreatePipeId, FlowCreatePipe.Type, ExprValueKind::VK_PRValue);
    CallExpr *CreatePipeCall = CallExpr::Create(ctx, CreatePipeRef, CreatePipeArgs, FlowCreatePipe.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    Stmts.push_back(CreatePipeCall);

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json actions;
    for (const auto& action : table->getActions()) {
        actions["actions"].push_back({
            {"name", action.actionName()},
            {"id", action.actionId()}
        });
    }

    json ops = {
        {"create_pipe", funcName}
    };

    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops},
        {"actions", actions["actions"]}
    });

    return true;
}

bool DocaProgram::buildEgressTablePipe(const TableNode* tableNode, bool isRoot) {
    std::vector<Stmt *> Stmts;

    bool fwdMissUndefined = true;

    Table *table = tableNode->data;
    std::string pipeName = absl::StrCat(table->getName(), "_", table->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));
    // PointerType * PipePtrPtr = new PointerType(QualType(PipePtr));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    // DeclRefExpr *PipeVarRef = DeclRefExpr::Create(ctx, PipeVar, PipeId, QualType(PipePtr), ExprValueKind::VK_LValue);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Actions Array */
    PointerType * ActionsPtr = new PointerType(QualType(FlowActions.Type));
    ConstantArrayType *ActionsArrType = new ConstantArrayType(QualType(ActionsPtr), APInt(APInt::APINT_BITS_PER_WORD, 1));
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, ActionsArr, "actions_arr", QualType(ActionsArrType), Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, PipeCfgVarRef, UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(PipeCfgVar->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, PipeCfgVarRef, QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, MatchVarRef, UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(MatchVar->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, MatchVarRef, QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    /* actions_arr[0] = &actions */
    ArraySubscriptExpr *ActionsArrayElement = ArraySubscriptExpr::Create(ctx, ActionsArrVarRef, Zero, QualType(ctx.UnsignedIntTy), VK_PRValue, OK_Ordinary);
    BinaryOperator *ActionsArrayElementAssign = BinaryOperator::Create(ctx, ActionsArrayElement, GetActionsAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsArrayElementAssign);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    // IdentifierInfo *CfgAttrId = const_cast<IdentifierInfo*>(FlowPipeCfg.Attr->getIdentifier());
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IdentifierInfo *AttrNameId = const_cast<IdentifierInfo*>(FlowPipeAttr.Name->getIdentifier());
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    // IdentifierInfo *AttrTypeId = const_cast<IdentifierInfo*>(FlowPipeAttr.PipeType->getIdentifier());
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeBasiclId = const_cast<IdentifierInfo*>(FlowPipeType.Basic->getIdentifier());
    DeclRefExpr *PipeTypeBasicRef = DeclRefExpr::Create(ctx, FlowPipeType.Basic, PipeTypeBasiclId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeBasicRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    // IdentifierInfo *AttrIsRootId = const_cast<IdentifierInfo*>(FlowPipeAttr.IsRoot->getIdentifier());
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *IsRoot = (isRoot)? IntegerLiteral::Create(ctx, APInt(1, 1), IntegerLiteral::Dec, QualType(ctx.BoolTy)) :
                                IntegerLiteral::Create(ctx, APInt(1, 0), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, IsRoot, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    /* Set cfg.attr.domain */
    MemberExpr *AttrDomain = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Domain->getDeclName(), FlowPipeAttr.Domain->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    DeclRefExpr *PipeDomainRef = nullptr;
    IdentifierInfo *PipeDomainEgressId = const_cast<IdentifierInfo*>(FlowPipeDomain.Egress->getIdentifier());
    PipeDomainRef = DeclRefExpr::Create(ctx, FlowPipeDomain.Egress, PipeDomainEgressId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrDomainAssign = BinaryOperator::Create(ctx, AttrDomain, PipeDomainRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrDomainAssign);

    /* Set cfg.match */
    MemberExpr *CfgMatch = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Match->getDeclName(), FlowPipeCfg.Match->getType(), ExprValueKind::VK_LValue, OK_Ordinary);    
    BinaryOperator *FlowPipeMatchAssign = BinaryOperator::Create(ctx, CfgMatch, GetMatchAddr, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeMatchAssign);

    /* Set cfg.actions */
    MemberExpr *CfgActions = MemberExpr::Create(ctx, PipeCfgVarRef, false, nullptr, FlowPipeCfg.Actions->getDeclName(), FlowPipeCfg.Actions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FlowPipeActionsAssign = BinaryOperator::Create(ctx, CfgActions, ActionsArrVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeActionsAssign);

    /* Set cfg.attr.nb_actions */
    MemberExpr *AttrNbActions = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.NbActions->getDeclName(), FlowPipeAttr.NbActions->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *NbActions = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
    BinaryOperator *FlowPipeAttrNbActionsAssign = BinaryOperator::Create(ctx, AttrNbActions, NbActions, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNbActionsAssign);

    /* Set match.meta.pkt_meta */
    MemberExpr *MatchMetaAttr = MemberExpr::Create(ctx, MatchVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *MetaPktmetaName = MemberExpr::Create(ctx, MatchMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *MetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
    BinaryOperator *MatchMetaAssign = BinaryOperator::Create(ctx, MetaPktmetaName, MetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(MatchMetaAssign);

    Expr *NullPtr = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.VoidPtrTy));

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;

    /* Set match fields */
    for (auto key : table->getKeys()) {
        MatchStateMachine stateMachine;
        QualType MatchFieldType;
        MemberExpr *MatchFieldExpr = stateMachine.parseTarget(this, Stmts, key.target, MatchVarRef, &MatchFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
        IntegerLiteral *MatchFieldMask = nullptr;
        if (key.match_type == "exact") {
            uint32_t bitWidth = MatchFieldType.getTypeWidth() * 8;
            uint64_t mask = (1ULL << bitWidth) - 1;
            MatchFieldMask = IntegerLiteral::Create(ctx, APInt(MatchFieldType.getTypeWidth(), mask), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
        }
        BinaryOperator *FlowMatchAssign = BinaryOperator::Create(ctx, MatchFieldExpr, MatchFieldMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FlowMatchAssign);
    }

    if (tableNode->actionNextTable.size() > 1) {
        /* Set actions.meta.pkt_meta */
        MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IntegerLiteral *ActionMetaMask = IntegerLiteral::Create(ctx, APInt(IntSize, 0xffffffff), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
        BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, ActionMetaMask, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(ActionsMetaAssign);

        /* set fwd to port */
        MemberExpr *FwdFwdType = MemberExpr::Create(ctx, VARREF(Fwd), false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IdentifierInfo *FwdTypePortId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPort->getIdentifier());
        DeclRefExpr *FwdPortRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPort, FwdTypePortId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
        BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPortRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FwdTypeAssign);
    } else {
        /* Set actions.meta.pkt_meta */
        const Table::ActionEntry* defaultAction = table->getDefaultActionEntry();
        std::vector<Expr *> GetActionPipeIdArgs;
        std::string defaultActionName = absl::StrCat(defaultAction->actionName(), "_", defaultAction->actionId());
        std::replace(defaultActionName.begin(), defaultActionName.end(), '.', '_');
        ConstantArrayType * defaultActionNameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, defaultAction->actionName().size()));
        QualType defaultActionNameStrTy = QualType(defaultActionNameArray);
        CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, StringLiteral::Create(ctx, defaultActionName, StringLiteral::Ascii, defaultActionNameStrTy));

        MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(ActionsMetaAssign);

        /* set fwd to pipe */
        MemberExpr *FwdFwdType = MemberExpr::Create(ctx, VARREF(Fwd), false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IdentifierInfo *FwdTypePipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
        DeclRefExpr *FwdPipeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypePipeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
        BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPipeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FwdTypeAssign);

        CALL_FUNC_WITH_REF(ctx, FlowGetPipeByName, GetTablePipe, StringLiteral::Create(ctx, defaultActionName, StringLiteral::Ascii, defaultActionNameStrTy));

        MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, VARREF(Fwd), false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, CALL(GetTablePipe), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        Stmts.push_back(FwdNextPipeAssign);
    }

    /* flow_create_pipe(&pipe_cfg,pipe) */
    std::vector<Expr *> CreatePipeArgs;
    CreatePipeArgs.push_back(GetPipeCfgAddr);
    CreatePipeArgs.push_back(GetFwdAddr);
    CreatePipeArgs.push_back(NullPtr);
    CreatePipeArgs.push_back(REF(ArgPipe));
    IdentifierInfo *CreatePipeId = const_cast<IdentifierInfo*>(FlowCreatePipe.Decl->getIdentifier());
    DeclRefExpr *CreatePipeRef = DeclRefExpr::Create(ctx, FlowCreatePipe.Decl, CreatePipeId, FlowCreatePipe.Type, ExprValueKind::VK_PRValue);
    CallExpr *CreatePipeCall = CallExpr::Create(ctx, CreatePipeRef, CreatePipeArgs, FlowCreatePipe.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    Stmts.push_back(CreatePipeCall);

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json actions;
    for (const auto& action : table->getActions()) {
        actions["actions"].push_back({
            {"name", action.actionName()},
            {"id", action.actionId()}
        });
    }

    json ops = {
        {"create_pipe", funcName}
    };

    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops},
        {"actions", actions["actions"]}
    });
    return true;
}

bool DocaProgram::buildTablePipe(Pipeline::Kind K, const TableNode* tableNode, bool isRoot) {
    bool ret = false;
    switch (K)
    {
    case Pipeline::Kind::INGRESS:
        if (tableNode->actionNextTable.size() > 1) {
            ret = buildIngressMultiActionTablePipe(tableNode, isRoot);
        } else {
            ret = buildIngressSingleActionTablePipe(tableNode, isRoot);
        }
        break;

    case Pipeline::Kind::EGRESS:
        ret = buildEgressTablePipe(tableNode, false);
        break;
    
    default:
        break;
    }

    return ret;
}

bool DocaProgram::initTablePipe(const TableNode* tableNode) {
#if 0
    auto actions = tableNode->actionNextTable;
    if (actions.size() > 1) {
        return false;
    }

    std::cout << std::endl;
    std::vector<Stmt *> Stmts;
    Table *table = tableNode->data;
    std::string pipeName = absl::StrCat(table->getName(), "_", table->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    ActionNode *actionNode = tableNode->getDefaultActionNode();
    Action *action = actionNode->data;
    std::string actionNodeName = absl::StrCat(action->getName(), "_", action->getId());
    std::cout << "Default action name: " << actionNodeName << std::endl;
    std::replace(actionNodeName.begin(), actionNodeName.end(), '.', '_');

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("init_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* Next Pipe */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, NextPipe, "next_pipe", FlowPipe.Type, Stmts);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(MatchVar->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(ActionsVar->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    IdentifierInfo *DefaultPipeId = &ctx.getIdentifierTable()->get("default_pipe");
    VarDecl *DefaultPipeVar = VarDecl::Create(ctx, FuncDecl, DefaultPipeId, QualType(PipePtr), SC_None);
    Stmt *DefaultPipeStmt = DeclStmt::Create(ctx, DeclGroupRef(DefaultPipeVar));
    DeclRefExpr *DefaultPipeVarRef = DeclRefExpr::Create(ctx, DefaultPipeVar, DefaultPipeId, QualType(PipePtr), ExprValueKind::VK_LValue);
    Stmts.push_back(DefaultPipeStmt);

    const Table::ActionEntry* defaultAction = table->getDefaultActionEntry();
    ConstantArrayType * TableNameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType TableStrTy = QualType(TableNameArray);
    CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, TableStrTy));

    std::vector<Expr *> GetActionPipeIdArgs;
    std::string defaultActionName = absl::StrCat(defaultAction->actionName(), "_", defaultAction->actionId());
    std::replace(defaultActionName.begin(), defaultActionName.end(), '.', '_');
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, defaultAction->actionName().size()));
    QualType ActionStrTy = QualType(nameArray);
    StringLiteral *ActionName = StringLiteral::Create(ctx, defaultActionName, StringLiteral::Ascii, ActionStrTy);
    GetActionPipeIdArgs.push_back(ActionName);
    IdentifierInfo *GetPipeIdId = const_cast<IdentifierInfo*>(FlowGetPipeIdByName.Decl->getIdentifier());
    DeclRefExpr *GetPipeIdRef = DeclRefExpr::Create(ctx, FlowGetPipeIdByName.Decl, GetActionPipeId, FlowGetPipeIdByName.Type, ExprValueKind::VK_PRValue);
    CallExpr *GetPipeIdCall = CallExpr::Create(ctx, GetPipeIdRef, GetActionPipeIdArgs, FlowGetPipeIdByName.Decl->getReturnType(), ExprValueKind::VK_PRValue);

    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, GetPipeIdCall, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    /* fwd.type=FLOW_FWD_PIPE */
    // IdentifierInfo *FwdTypeId = const_cast<IdentifierInfo*>(FlowFwd.FwdType->getIdentifier());
    MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdTypePipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
    DeclRefExpr *FwdPipeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypePipeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPipeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdTypeAssign);

    /* fwd.next_pipe=default_pipe */
    MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, FwdVarRef, false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IdentifierInfo *FwdNextPipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
    BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, DefaultPipeVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdNextPipeAssign);

    std::vector<Expr *> PipeAddEntryArgs;
    PipeAddEntryArgs.push_back(REF(ArgPipe));
    PipeAddEntryArgs.push_back(GetMatchAddr);
    PipeAddEntryArgs.push_back(GetActionsAddr);
    PipeAddEntryArgs.push_back(GetFwdAddr);
    IdentifierInfo *PipeAddEntryId = const_cast<IdentifierInfo*>(FlowPipeAddEntry.Decl->getIdentifier());
    DeclRefExpr *PipeAddEntryRef = DeclRefExpr::Create(ctx, FlowPipeAddEntry.Decl, PipeAddEntryId, FlowPipeAddEntry.Type, ExprValueKind::VK_PRValue);
    CallExpr *PipeAddEntryCall = CallExpr::Create(ctx, PipeAddEntryRef, PipeAddEntryArgs, FlowPipeAddEntry.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    Stmts.push_back(PipeAddEntryCall);

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);

    for (auto& pipe : j["pipes"]) {
        if (pipe["name"] == pipeName) {
            pipe["hw_ops"]["init_pipe"] = funcName;
            break;
        }
    }

    FuncDecl->setBody(CS);
#endif
    return true;
}

#define MAX_VAR_ARGS    8

bool DocaProgram::buildAddActionPipeEntry(const ActionNode* actionNode) {
    std::vector<Stmt *> Stmts;
    Action *action = actionNode->data;
    std::string pipeName = absl::StrCat(action->getName(), "_", action->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    std::vector<Primitive> primitives = action->getPrimitives();

    bool needAddFunc = true;
    for (auto primitive : primitives) {
        auto op = primitive.getOp();
        if (op == "mark_to_pass" || op == "mark_to_drop" || op == "mark_to_rss" || op == "mark_to_hairpin" || op == "mark_to_fwd_port") {
            needAddFunc = false;
            break;
        }
    }

    if (!needAddFunc) return false;

    /* Build add_pipe_entry */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("add_" + pipeName + "_hw_pipe_entry");

    std::vector<QualType> ActionFieldTypes;
    std::vector<std::string> ActionFieldNames;

    for (auto primitive : primitives) {
        ActionStateMachine stateMachine;
        auto parameters = primitive.getParameter();
        auto leftParam = parameters[0];
        std::shared_ptr<FieldParameter> fieldParam = std::dynamic_pointer_cast<FieldParameter>(leftParam);
        std::vector<std::string> fields = fieldParam->value();
        QualType ActionFieldType = stateMachine.getLastFieldTypeAndName(this, fields);
        ActionFieldTypes.push_back(ActionFieldType);
        ActionFieldNames.push_back(fields[fields.size() - 1]);
    }

    QualType PipePtrTy = QualType(PipePtr);
    PointerType *CharPtr = new PointerType(QualType(ctx.CharTy));
    QualType CharPtrTy = QualType(CharPtr, QualType::Qualifiers::Const);

    QualType FuncRetType = QualType(ctx.IntTy);
    std::vector<QualType> FuncArgsType;
    FuncArgsType.push_back(PipePtrTy);
    FuncArgsType.push_back(CharPtrTy);
    for (auto type : ActionFieldTypes) {
        FuncArgsType.push_back(type);
    }
    QualType FuncType = ctx.getFunctionType(FuncRetType, FuncArgsType);
    IdentifierInfo *FuncId = &ctx.getIdentifierTable()->get(funcName);
    FunctionDecl *FuncDecl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), FuncId, FuncType, SC_None);

    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", PipePtrTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgNextAction, "next_action", CharPtrTy);

    std::vector<ParmVarDecl*> VarArgDecls;
    std::vector<DeclRefExpr*> VarArgRefs;

    for (int i = 0; i < ActionFieldNames.size(); i++) {
        IdentifierInfo * ArgId = &ctx.getIdentifierTable()->get(ActionFieldNames[i]);
        ParmVarDecl * ArgDecl = ParmVarDecl::Create(ctx, FuncDecl, ArgId, ActionFieldTypes[i], SC_None);
        DeclRefExpr * ArgRef = DeclRefExpr::Create(ctx, ArgDecl, ArgId, ActionFieldTypes[i], ExprValueKind::VK_LValue);
        VarArgDecls.push_back(ArgDecl);
        VarArgRefs.push_back(ArgRef);
    }

    std::vector<ParmVarDecl *> FuncDecls;
    FuncDecls.push_back(DECL(ArgPipe));
    FuncDecls.push_back(DECL(ArgNextAction));
    for (auto decl : VarArgDecls) {
        FuncDecls.push_back(decl);
    }
    FuncDecl->setParams(ctx, FuncDecls);
    ctx.getTranslationUnitDecl()->addDecl(FuncDecl);

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Create SET_MAC_ADDR */
    DECLARE_EXTERN_FUNC(ctx, SetMacAddr, "SET_MAC_ADDR", ctx.IntTy,  QualType(ctx.VoidPtrTy), 
                    QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), 
                    QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy), QualType(ctx.UnsignedCharTy));

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;

    // for (auto primitive : primitives) {
    for (int i = 0; i < primitives.size(); i++) {
        auto primitive = primitives[i];
        auto op = primitive.getOp();
        if (op == "assign") {
            QualType ActionFieldType;
            auto parameters = primitive.getParameter();
            auto leftParam = parameters[0];
            std::shared_ptr<FieldParameter> fieldParam = std::dynamic_pointer_cast<FieldParameter>(leftParam);
            std::vector<std::string> fields = fieldParam->value();
            ActionStateMachine stateMachine;
            MemberExpr *ActionFieldExpr = stateMachine.parseTarget(this, Stmts, fields, ActionsVarRef, &ActionFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
            std::cout << ActionFieldExpr->emitStmt() << std::endl;
            auto type = ActionFieldType.getTypePtr();
            if (auto array = dynamic_cast<ConstantArrayType *>(type)) {
                auto arraySize = *array->getSize().getRawData();
                std::vector<Expr *> SetMacAddrArgs;
                SetMacAddrArgs.push_back(ActionFieldExpr);
                for (int j = 0; j < arraySize; j++) {
                    IntegerLiteral *MacAddrOffset = IntegerLiteral::Create(ctx, APInt(1, j), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
                    ArraySubscriptExpr *ActionsArrayElement = ArraySubscriptExpr::Create(ctx, VarArgRefs[i], MacAddrOffset, QualType(ctx.UnsignedIntTy), VK_PRValue, OK_Ordinary);
                    std::cout << ActionFieldExpr->emitStmt() << std::endl;
                    std::cout << MacAddrOffset->emitStmt() << std::endl;
                    std::cout << ActionsArrayElement->emitStmt() << std::endl;
                    SetMacAddrArgs.push_back(ActionsArrayElement);
                }
                CallExpr *MemcpyArrayCall = CallExpr::Create(ctx, SetMacAddrRef, SetMacAddrArgs, FuncDecl->getReturnType(), ExprValueKind::VK_PRValue);
                Stmts.push_back(MemcpyArrayCall);
                // CALL_FUNC_AND_STMT(ctx, FuncDecl, SetMacAddr, Stmts, ActionFieldExpr, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask, SetMacAddrMask);
            } else {
                BinaryOperator *FlowActionAssign = BinaryOperator::Create(ctx, ActionFieldExpr, VarArgRefs[i], BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
                Stmts.push_back(FlowActionAssign);
            }
        }
    }

    /* Set actions.meta.pkt_meta */
    CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, REF(ArgNextAction));
    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    /* set fwd to pipe */
    MemberExpr *FwdFwdType = MemberExpr::Create(ctx, VARREF(Fwd), false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdTypePipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
    DeclRefExpr *FwdPipeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypePipeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPipeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdTypeAssign);

    CALL_FUNC_WITH_REF(ctx, FlowGetPipeByName, GetTablePipe, REF(ArgNextAction));

    MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, VARREF(Fwd), false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, CALL(GetTablePipe), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdNextPipeAssign);

    /* flow_create_pipe(&pipe_cfg,pipe) */
    CALL_FUNC_WITH_REF(ctx, FlowPipeAddEntry, AddPipeEntry, REF(ArgPipe), GetMatchAddr, GetActionsAddr, GetFwdAddr);

    ReturnStmt *Result = ReturnStmt::Create(ctx, CALL(AddPipeEntry));

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);
    FuncDecl->setBody(CS);

    for (auto& pipe : j["pipes"]) {
        if (pipe["name"] == pipeName) {
            pipe["hw_ops"]["add_pipe_entry"] = funcName;
            break;
        }
    }

    return true;
}

bool DocaProgram::buildAddTablePipeEntry(const TableNode* tableNode) {
    std::vector<Stmt *> Stmts;
    Table *table = tableNode->data;
    std::string pipeName = absl::StrCat(table->getName(), "_", table->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    /* Build add_pipe_entry */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("add_" + pipeName + "_hw_pipe_entry");

    std::vector<QualType> MatchFieldTypes;
    std::vector<std::string> MatchFieldNames;

    auto keys = table->getKeys();
    auto nb_keys = keys.size();

    if (nb_keys > 0) {
        for (int i = 0; i < nb_keys; i++) {
            MatchStateMachine stateMachine;
            auto target = keys[i].target;
            QualType MatchFieldType = stateMachine.getLastFieldTypeAndName(this, target);
            MatchFieldTypes.push_back(MatchFieldType);
            MatchFieldNames.push_back(target[target.size() - 1]);
        }
    }

    QualType PipePtrTy = QualType(PipePtr);
    PointerType *CharPtr = new PointerType(QualType(ctx.CharTy));
    QualType CharPtrTy = QualType(CharPtr, QualType::Qualifiers::Const);

    QualType FuncRetType = QualType(ctx.IntTy);
    std::vector<QualType> FuncArgsType;
    FuncArgsType.push_back(PipePtrTy);
    // FuncArgsType.push_back(PipePtrTy);
    FuncArgsType.push_back(CharPtrTy);
    for (auto type : MatchFieldTypes) {
        FuncArgsType.push_back(type);
    }
    QualType FuncType = ctx.getFunctionType(FuncRetType, FuncArgsType);
    IdentifierInfo *FuncId = &ctx.getIdentifierTable()->get(funcName);
    FunctionDecl *FuncDecl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), FuncId, FuncType, SC_None);

    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", PipePtrTy);
    // CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgNextAction, "next_action", PipePtrTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgNextAction, "next_action", CharPtrTy);

    std::vector<ParmVarDecl*> VarArgDecls;
    std::vector<DeclRefExpr*> VarArgRefs;

    for (int i = 0; i < MatchFieldNames.size(); i++) {
        IdentifierInfo * ArgId = &ctx.getIdentifierTable()->get(MatchFieldNames[i]);
        ParmVarDecl * ArgDecl = ParmVarDecl::Create(ctx, FuncDecl, ArgId, MatchFieldTypes[i], SC_None);
        DeclRefExpr * ArgRef = DeclRefExpr::Create(ctx, ArgDecl, ArgId, MatchFieldTypes[i], ExprValueKind::VK_LValue);
        VarArgDecls.push_back(ArgDecl);
        VarArgRefs.push_back(ArgRef);
    }

    std::vector<ParmVarDecl *> FuncDecls;
    FuncDecls.push_back(DECL(ArgPipe));
    FuncDecls.push_back(DECL(ArgNextAction));
    for (auto decl : VarArgDecls) {
        FuncDecls.push_back(decl);
    }
    FuncDecl->setParams(ctx, FuncDecls);
    ctx.getTranslationUnitDecl()->addDecl(FuncDecl);

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* memset(&match,0,sizeof(match)) */
    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetMatchAddr, Zero, GetMatchSize);

    /* memset(&actions,0,sizeof(actions)) */
    UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetActionsAddr, Zero, GetActionsSize);

    /* memset(&fwd,0,sizeof(fwd)) */
    UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdAddr, Zero, GetFwdSize);

    bool ipProtocolSet = false;
    bool transportProtocolSet = false;
    bool tunnelSet = false;

    /* Set match fields */
    if (nb_keys > 0) {
        for (int i = 0; i < nb_keys; i++) {
            MatchStateMachine stateMachine;
            QualType MatchFieldType;
            MemberExpr *MatchFieldExpr = stateMachine.parseTarget(this, Stmts, keys[i].target, VARREF(Match), &MatchFieldType, &ipProtocolSet, &transportProtocolSet, &tunnelSet);
            BinaryOperator *FlowMatchAssign = BinaryOperator::Create(ctx, MatchFieldExpr, VarArgRefs[i], BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FlowMatchAssign);
        }
    }

    /* Set actions.meta.pkt_meta */
    CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, REF(ArgNextAction));
    MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(ActionsMetaAssign);

    /* set fwd to pipe */
    MemberExpr *FwdFwdType = MemberExpr::Create(ctx, VARREF(Fwd), false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdTypePipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
    DeclRefExpr *FwdPipeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypePipeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdPipeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdTypeAssign);

    CALL_FUNC_WITH_REF(ctx, FlowGetPipeByName, GetTablePipe, REF(ArgNextAction));

    MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, VARREF(Fwd), false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, CALL(GetTablePipe), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdNextPipeAssign);

    /* flow_create_pipe(&pipe_cfg,pipe) */
    CALL_FUNC_WITH_REF(ctx, FlowPipeAddEntry, AddPipeEntry, REF(ArgPipe), GetMatchAddr, GetActionsAddr, GetFwdAddr);

    ReturnStmt *Result = ReturnStmt::Create(ctx, CALL(AddPipeEntry));

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);
    FuncDecl->setBody(CS);

    for (auto& pipe : j["pipes"]) {
        if (pipe["name"] == pipeName) {
            pipe["hw_ops"]["add_pipe_entry"] = funcName;
            break;
        }
    }

    return true;
}

void DocaProgram::processExpression(const std::shared_ptr<Expression>& expr, std::vector<Stmt*>& Stmts, DeclRefExpr* MatchVarRef, bool* ipProtocolSet, bool* transportProtocolSet, bool* tunnelSet) {
    if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        if (binaryExpr->getOp() == Expression::AND) {
            auto left = binaryExpr->getLeft();
            auto right = binaryExpr->getRight();
            processExpression(left, Stmts, MatchVarRef, ipProtocolSet, transportProtocolSet, tunnelSet);
            // binaryExpr->print();
            processExpression(right, Stmts, MatchVarRef, ipProtocolSet, transportProtocolSet, tunnelSet);
        } else if (binaryExpr->getOp() == Expression::EQUAL) {
            auto left = binaryExpr->getLeft();
            auto right = binaryExpr->getRight();

            QualType MatchFieldType;
            MemberExpr *MatchFieldExpr = nullptr;
            IntegerLiteral *MatchFieldValue = nullptr;

            if (auto fieldExpr = std::dynamic_pointer_cast<FieldExpression>(left)) {
                MatchStateMachine stateMachine;
                MatchFieldExpr = stateMachine.parseTarget(this, Stmts, fieldExpr->getFields(), MatchVarRef, &MatchFieldType, ipProtocolSet, transportProtocolSet, tunnelSet);
            }

            if (auto hexStrExpr = std::dynamic_pointer_cast<HexStrExpression>(right)) {
                uint64_t value = std::stoull(hexStrExpr->getHexValue(), nullptr, 16);
                MatchFieldValue = IntegerLiteral::Create(ctx, APInt(MatchFieldType.getTypeWidth(), value), IntegerLiteral::Hex, QualType(ctx.UnsignedIntTy));
            }

            if (!MatchFieldExpr || !MatchFieldValue) {
                std::cout << "Failed to process Expression..." << std::endl;
            }

            BinaryOperator *FlowMatchAssign = BinaryOperator::Create(ctx, MatchFieldExpr, MatchFieldValue, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
            Stmts.push_back(FlowMatchAssign);
        }
    }  
}

bool DocaProgram::buildIngressConditionalPipe(const ConditionalNode* conditionalNode, bool isRoot) {
    std::vector<Stmt *> Stmts;
    Conditional *conditional = conditionalNode->data;
    std::string pipeName = absl::StrCat(conditional->getName(), "_", conditional->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    // IdentifierInfo *FuncId = &ctx.getIdentifierTable()->get(funcName);

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    Expr *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    auto *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

   /* Fwd Miss */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, FwdMiss, "fwd_miss", FlowFwd.Type, Stmts);

   /* Fwd Miss Pipe */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, FwdMissPipe, "fwd_miss_pipe", QualType(PipePtr), Stmts);

    // IdentifierInfo *FwdMissPipeId = &ctx.getIdentifierTable()->get("fwd_miss_pipe");
    // VarDecl *FwdMissPipeVar = VarDecl::Create(ctx, FuncDecl, FwdMissPipeId, QualType(PipePtr), SC_None);
    // Stmt *FwdMissPipeStmt = DeclStmt::Create(ctx, DeclGroupRef(FwdMissPipeVar));
    // DeclRefExpr *FwdMissPipeVarRef = DeclRefExpr::Create(ctx, FwdMissPipeVar, FwdMissPipeId, QualType(PipePtr), ExprValueKind::VK_LValue);
    // Stmts.push_back(FwdMissPipeStmt);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, VARREF(PipeCfg), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(PipeCfg)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(PipeCfg), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* memset(&fwd_miss,0,sizeof(fwd_miss)) */
    UnaryOperator *GetFwdMissAddr = UnaryOperator::Create(ctx, VARREF(FwdMiss), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(FwdMiss)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetFwdMissSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(FwdMiss), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetFwdMissAddr, Zero, GetFwdMissSize);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    // IdentifierInfo *CfgAttrId = const_cast<IdentifierInfo*>(FlowPipeCfg.Attr->getIdentifier());
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IdentifierInfo *AttrNameId = const_cast<IdentifierInfo*>(FlowPipeAttr.Name->getIdentifier());
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    // IdentifierInfo *AttrTypeId = const_cast<IdentifierInfo*>(FlowPipeAttr.PipeType->getIdentifier());
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeControlId = const_cast<IdentifierInfo*>(FlowPipeType.Control->getIdentifier());
    DeclRefExpr *PipeTypeControlRef = DeclRefExpr::Create(ctx, FlowPipeType.Control, PipeTypeControlId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeControlRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    // IdentifierInfo *AttrIsRootId = const_cast<IdentifierInfo*>(FlowPipeAttr.IsRoot->getIdentifier());
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *IsRootId;
    if (isRoot) {
        IsRootId = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    } else {
        IsRootId = IntegerLiteral::Create(ctx, APInt(IntSize, 0),  IntegerLiteral::Dec,QualType(ctx.BoolTy));
    }
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, IsRootId, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    Expr *NullPtr = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.VoidPtrTy));

    // std::vector<Expr *> GetPipeArgs;
    // std::string SwFsaName("sw_fsa");
    // ConstantArrayType * SwFsaNameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, SwFsaName.size()));
    // QualType SwFsaNameStrTy = QualType(SwFsaNameArray);
    // StringLiteral *FwdMissPipeName = StringLiteral::Create(ctx, SwFsaName, StringLiteral::Ascii, SwFsaNameStrTy);
    // GetPipeArgs.push_back(FwdMissPipeName);
    // IdentifierInfo *GetPipeId = const_cast<IdentifierInfo*>(FlowGetPipeByName.Decl->getIdentifier());
    // DeclRefExpr *GetPipeRef = DeclRefExpr::Create(ctx, FlowGetPipeByName.Decl, GetPipeId, FlowGetPipeByName.Type, ExprValueKind::VK_PRValue);
    // CallExpr *GetPipeCall = CallExpr::Create(ctx, GetPipeRef, GetPipeArgs, FlowGetPipeByName.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    // BinaryOperator *NextPipeAssign = BinaryOperator::Create(ctx, FwdMissPipeVarRef, GetPipeCall, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    // Stmts.push_back(NextPipeAssign);

    MemberExpr *FwdMissFwdType = MemberExpr::Create(ctx, FwdMissVarRef, false, FlowFwd.FwdType, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *FwdMissTypeRssId = const_cast<IdentifierInfo*>(FlowFwdType.FwdRss->getIdentifier());
    DeclRefExpr *FwdMissRssRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdRss, FwdMissTypeRssId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FwdMissTypeAssign = BinaryOperator::Create(ctx, FwdMissFwdType, FwdMissRssRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FwdMissTypeAssign);

    /* flow_create_pipe(&pipe_cfg,pipe) */
    std::vector<Expr *> CreatePipeArgs;
    CreatePipeArgs.push_back(GetPipeCfgAddr);
    CreatePipeArgs.push_back(NullPtr);
    CreatePipeArgs.push_back(GetFwdMissAddr);
    CreatePipeArgs.push_back(REF(ArgPipe));
    IdentifierInfo *CreatePipeId = const_cast<IdentifierInfo*>(FlowCreatePipe.Decl->getIdentifier());
    DeclRefExpr *CreatePipeRef = DeclRefExpr::Create(ctx, FlowCreatePipe.Decl, CreatePipeId, FlowCreatePipe.Type, ExprValueKind::VK_PRValue);
    CallExpr *CreatePipeCall = CallExpr::Create(ctx, CreatePipeRef, CreatePipeArgs, FlowCreatePipe.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    Stmts.push_back(CreatePipeCall);

    Stmts.push_back(Result);
    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json ops = {
        {"create_pipe", funcName}
    };
    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops}
    });

    return true;
}

bool DocaProgram::buildEgressConditionalPipe(const ConditionalNode* conditionalNode, bool isRoot) {
    std::vector<Stmt *> Stmts;
    Conditional *conditional = conditionalNode->data;
    std::string pipeName = absl::StrCat(conditional->getName(), "_", conditional->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("create_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    Expr *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    auto *Result = ReturnStmt::Create(ctx, Zero);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Cfg */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, PipeCfg, "pipe_cfg", FlowPipeCfg.Type, Stmts);

    /* memset(&pipe_cfg,0,sizeof(pipe_cfg)) */
    UnaryOperator *GetPipeCfgAddr = UnaryOperator::Create(ctx, VARREF(PipeCfg), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(PipeCfg)->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetPipeCfgSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(PipeCfg), QualType(ctx.UnsignedIntTy));
    CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, Stmts, GetPipeCfgAddr, Zero, GetPipeCfgSize);

    /* Set cfg.attr.name */
    ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, pipeName.size()));
    QualType StrTy = QualType(nameArray);
    // IdentifierInfo *CfgAttrId = const_cast<IdentifierInfo*>(FlowPipeCfg.Attr->getIdentifier());
    MemberExpr *CfgAttr = MemberExpr::Create(ctx, PipeCfgVarRef, false, FlowPipeAttr.Name, FlowPipeCfg.Attr->getDeclName(), FlowPipeCfg.Attr->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    // IdentifierInfo *AttrNameId = const_cast<IdentifierInfo*>(FlowPipeAttr.Name->getIdentifier());
    MemberExpr *AttrName = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Name->getDeclName(), FlowPipeAttr.Name->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    StringLiteral *PipeName = StringLiteral::Create(ctx, pipeName, StringLiteral::Ascii, StrTy);
    BinaryOperator *FlowPipeAttrNameAssign = BinaryOperator::Create(ctx, AttrName, PipeName, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrNameAssign);

    /* Set cfg.attr.type */
    // IdentifierInfo *AttrTypeId = const_cast<IdentifierInfo*>(FlowPipeAttr.PipeType->getIdentifier());
    MemberExpr *AttrType = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.PipeType->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IdentifierInfo *PipeTypeControlId = const_cast<IdentifierInfo*>(FlowPipeType.Control->getIdentifier());
    DeclRefExpr *PipeTypeControlRef = DeclRefExpr::Create(ctx, FlowPipeType.Control, PipeTypeControlId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrTypeAssign = BinaryOperator::Create(ctx, AttrType, PipeTypeControlRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrTypeAssign);

    /* Set cfg.attr.is_root */
    // IdentifierInfo *AttrIsRootId = const_cast<IdentifierInfo*>(FlowPipeAttr.IsRoot->getIdentifier());
    MemberExpr *AttrIsRoot = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.IsRoot->getDeclName(), FlowPipeAttr.PipeType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    IntegerLiteral *IsRootId;
    if (isRoot) {
        IsRootId = IntegerLiteral::Create(ctx, APInt(IntSize, 1), IntegerLiteral::Dec, QualType(ctx.BoolTy));
    } else {
        IsRootId = IntegerLiteral::Create(ctx, APInt(IntSize, 0),  IntegerLiteral::Dec,QualType(ctx.BoolTy));
    }
    BinaryOperator *FlowPipeAttrIsRootAssign = BinaryOperator::Create(ctx, AttrIsRoot, IsRootId, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrIsRootAssign);

    /* Set cfg.attr.domain */
    MemberExpr *AttrDomain = MemberExpr::Create(ctx, CfgAttr, false, nullptr, FlowPipeAttr.Domain->getDeclName(), FlowPipeAttr.Domain->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
    DeclRefExpr *PipeDomainRef = nullptr;
    IdentifierInfo *PipeDomainEgressId = const_cast<IdentifierInfo*>(FlowPipeDomain.Egress->getIdentifier());
    PipeDomainRef = DeclRefExpr::Create(ctx, FlowPipeDomain.Egress, PipeDomainEgressId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
    BinaryOperator *FlowPipeAttrDomainAssign = BinaryOperator::Create(ctx, AttrDomain, PipeDomainRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
    Stmts.push_back(FlowPipeAttrDomainAssign);

    Expr *NullPtr = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.VoidPtrTy));

    /* flow_create_pipe(&pipe_cfg,pipe) */
    std::vector<Expr *> CreatePipeArgs;
    CreatePipeArgs.push_back(GetPipeCfgAddr);
    CreatePipeArgs.push_back(NullPtr);
    CreatePipeArgs.push_back(NullPtr);
    CreatePipeArgs.push_back(REF(ArgPipe));
    IdentifierInfo *CreatePipeId = const_cast<IdentifierInfo*>(FlowCreatePipe.Decl->getIdentifier());
    DeclRefExpr *CreatePipeRef = DeclRefExpr::Create(ctx, FlowCreatePipe.Decl, CreatePipeId, FlowCreatePipe.Type, ExprValueKind::VK_PRValue);
    CallExpr *CreatePipeCall = CallExpr::Create(ctx, CreatePipeRef, CreatePipeArgs, FlowCreatePipe.Decl->getReturnType(), ExprValueKind::VK_PRValue);
    Stmts.push_back(CreatePipeCall);

    Stmts.push_back(Result);
    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);

    json ops = {
        {"create_pipe", funcName}
    };
    j["pipes"].push_back({
        {"name", pipeName},
        {"hw_enable", true},
        {"hw_ops", ops}
    });

    return true;
}

bool DocaProgram::buildConditionalPipe(Pipeline::Kind K, const ConditionalNode* conditionalNode, bool isRoot) {
    bool ret = false;
    switch (K)
    {
    case Pipeline::Kind::INGRESS:
        ret = buildIngressConditionalPipe(conditionalNode, isRoot);
        break;

    case Pipeline::Kind::EGRESS:
        ret = buildEgressConditionalPipe(conditionalNode, false);
        break;
    
    default:
        break;
    }

    return ret;
}

bool DocaProgram::initConditionalPipe(const ConditionalNode* conditionalNode) {
    std::vector<Stmt *> Stmts;
    Conditional *conditional = conditionalNode->data;
    std::string pipeName = absl::StrCat(conditional->getName(), "_", conditional->getId());
    std::replace(pipeName.begin(), pipeName.end(), '.', '_');
    std::shared_ptr<Expression> expr = conditional->expr();

    /* Build create_control_pipe */
    PointerType * PipePtr = new PointerType(QualType(FlowPipe.Type));

    IdentifierInfo *PipeId = &ctx.getIdentifierTable()->get(pipeName + "_pipe");
    VarDecl *PipeVar = VarDecl::Create(ctx, ctx.getTranslationUnitDecl(), PipeId, QualType(PipePtr), SC_None);
    ctx.getTranslationUnitDecl()->addDecl(PipeVar);

    std::string funcName("init_" + pipeName + "_hw_pipe");

    QualType ArgPipeTy = QualType(PipePtr);
    DECLARE_FUNC(ctx, Func, funcName, ctx.IntTy, ArgPipeTy);
    CREATE_PARAM_VAR_AND_REF(ctx, FuncDecl, ArgPipe, "pipe", ArgPipeTy);
    SET_FUNC_PARAMS_AND_ADD_DECL(ctx, FuncDecl,
        ParmVarDecl::Create(ctx, FuncDecl, &ctx.getIdentifierTable()->get("pipe"), QualType(PipePtr), SC_None)
    );

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    uint64_t UnsignedCharSize = ctx.getTypeSize(QualType(ctx.UnsignedCharTy));
    IntegerLiteral *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));
    ReturnStmt *Result = ReturnStmt::Create(ctx, Zero);

    /* Priority */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Priority, "priority", QualType(ctx.UnsignedCharTy), Stmts);

    /* Match */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Match, "match", FlowMatch.Type, Stmts);

    /* Actions */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Actions, "actions", FlowActions.Type, Stmts);

    /* Fwd */
    DECLARE_VAR_AND_STMT(ctx, FuncDecl, Fwd, "fwd", FlowFwd.Type, Stmts);

    /* Create memset */
    DECLARE_EXTERN_FUNC(ctx, Memset, "memset", ctx.IntTy, QualType(ctx.VoidPtrTy),  QualType(ctx.IntTy), QualType(ctx.UnsignedIntTy));

    /* Block for TRUE NEXT */ 
    {
        std::vector<Stmt *> ruleStmt;

        IdentifierInfo *NextPipeId = &ctx.getIdentifierTable()->get("next_pipe");
        VarDecl *NextPipeVar = VarDecl::Create(ctx, FuncDecl, NextPipeId, QualType(PipePtr), SC_None);
        Stmt *NextPipeStmt = DeclStmt::Create(ctx, DeclGroupRef(NextPipeVar));
        DeclRefExpr *NextPipeVarRef = DeclRefExpr::Create(ctx, NextPipeVar, NextPipeId, QualType(PipePtr), ExprValueKind::VK_LValue);
        ruleStmt.push_back(NextPipeStmt);

        IntegerLiteral *PriorityVal = IntegerLiteral::Create(ctx, APInt(UnsignedCharSize, 0), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
        BinaryOperator *PriorityAssign = BinaryOperator::Create(ctx, PriorityVarRef, PriorityVal, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(PriorityAssign);

        /* memset(&match,0,sizeof(match)) */
        UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
        UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
        CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, ruleStmt, GetMatchAddr, Zero, GetMatchSize);

        /* memset(&actions,0,sizeof(actions)) */
        UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
        UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
        CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, ruleStmt, GetActionsAddr, Zero, GetActionsSize);

        /* memset(&fwd,0,sizeof(fwd)) */
        UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
        UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
        CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, ruleStmt, GetFwdAddr, Zero, GetFwdSize);

        bool ipProtocolSet = false;
        bool transportProtocolSet = false;
        bool tunnelSet = false;

        processExpression(expr, ruleStmt, MatchVarRef, &ipProtocolSet, &transportProtocolSet, &tunnelSet);

        std::string trueNextName;
        CallGraphNodeBase *trueNextNode = conditionalNode->trueNextNode();
        if (auto trueTableNode = dynamic_cast<TableNode*>(trueNextNode)) {
            Table *table = trueTableNode->data;
            trueNextName = absl::StrCat(table->getName(), "_", table->getId());
        } else if (auto trueConditionalNode = dynamic_cast<ConditionalNode*>(trueNextNode)) {
            Conditional *conditional = trueConditionalNode->data;
            trueNextName = absl::StrCat(conditional->getName(), "_", conditional->getId());
        } else {
            std::cerr << "Unknown type for True next!" << std::endl;
        }
        std::replace(trueNextName.begin(), trueNextName.end(), '.', '_');

        ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, trueNextName.size()));
        QualType StrTy = QualType(nameArray);
        StringLiteral *NextPipeName = StringLiteral::Create(ctx, trueNextName, StringLiteral::Ascii, StrTy);

        /* Set actions.meta.pkt_meta */
        CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, NextPipeName);
        MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(ActionsMetaAssign);

        /* Set fwd.next_pipe */
        CALL_FUNC_WITH_REF(ctx, FlowGetPipeByName, GetTablePipe, NextPipeName);
        BinaryOperator *NextPipeAssign = BinaryOperator::Create(ctx, NextPipeVarRef, CALL(GetTablePipe), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(NextPipeAssign);

        MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, nullptr, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IdentifierInfo *FwdTypeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
        DeclRefExpr *FwdTypeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
        BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdTypeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(FwdTypeAssign);

        MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, FwdVarRef, false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        // IdentifierInfo *FwdNextPipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
        BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, NextPipeVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(FwdNextPipeAssign);

        std::vector<Expr *> ControlPipeAddEntryArgs;
        ControlPipeAddEntryArgs.push_back(REF(ArgPipe));
        ControlPipeAddEntryArgs.push_back(VARREF(Priority));
        ControlPipeAddEntryArgs.push_back(GetMatchAddr);
        ControlPipeAddEntryArgs.push_back(GetActionsAddr);
        ControlPipeAddEntryArgs.push_back(GetFwdAddr);
        IdentifierInfo *ControlPipeAddEntryId = const_cast<IdentifierInfo*>(FlowControlPipeAddEntry.Decl->getIdentifier());
        DeclRefExpr *ControlPipeAddEntryRef = DeclRefExpr::Create(ctx, FlowControlPipeAddEntry.Decl, ControlPipeAddEntryId, FlowControlPipeAddEntry.Type, ExprValueKind::VK_PRValue);
        CallExpr *ControlPipeAddEntryCall = CallExpr::Create(ctx, ControlPipeAddEntryRef, ControlPipeAddEntryArgs, FlowControlPipeAddEntry.Decl->getReturnType(), ExprValueKind::VK_PRValue);
        ruleStmt.push_back(ControlPipeAddEntryCall);

        auto *CS = CompoundStmt::Create(ctx, ruleStmt);
        Stmts.push_back(CS);
    }


    /* Block for False NEXT */ 
    {
        std::vector<Stmt *> ruleStmt;

        IdentifierInfo *NextPipeId = &ctx.getIdentifierTable()->get("next_pipe");
        VarDecl *NextPipeVar = VarDecl::Create(ctx, FuncDecl, NextPipeId, QualType(PipePtr), SC_None);
        Stmt *NextPipeStmt = DeclStmt::Create(ctx, DeclGroupRef(NextPipeVar));
        DeclRefExpr *NextPipeVarRef = DeclRefExpr::Create(ctx, NextPipeVar, NextPipeId, QualType(PipePtr), ExprValueKind::VK_LValue);
        ruleStmt.push_back(NextPipeStmt);

        IntegerLiteral *PriorityVal = IntegerLiteral::Create(ctx, APInt(UnsignedCharSize, 1), IntegerLiteral::Dec, QualType(ctx.UnsignedCharTy));
        BinaryOperator *PriorityAssign = BinaryOperator::Create(ctx, PriorityVarRef, PriorityVal, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(PriorityAssign);

        /* memset(&match,0,sizeof(match)) */
        UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, VARREF(Match), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Match)->getType()), VK_PRValue, OK_Ordinary);
        UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Match), QualType(ctx.UnsignedIntTy));
        CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, ruleStmt, GetMatchAddr, Zero, GetMatchSize);

        /* memset(&actions,0,sizeof(actions)) */
        UnaryOperator *GetActionsAddr = UnaryOperator::Create(ctx, VARREF(Actions), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Actions)->getType()), VK_PRValue, OK_Ordinary);
        UnaryExprOrTypeTraitExpr *GetActionsSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Actions), QualType(ctx.UnsignedIntTy));
        CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, ruleStmt, GetActionsAddr, Zero, GetActionsSize);

        /* memset(&fwd,0,sizeof(fwd)) */
        UnaryOperator *GetFwdAddr = UnaryOperator::Create(ctx, VARREF(Fwd), UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(VAR(Fwd)->getType()), VK_PRValue, OK_Ordinary);
        UnaryExprOrTypeTraitExpr *GetFwdSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, VARREF(Fwd), QualType(ctx.UnsignedIntTy));
        CALL_FUNC_AND_STMT(ctx, FuncDecl, Memset, ruleStmt, GetFwdAddr, Zero, GetFwdSize);

        CallGraphNodeBase *falseNextNode = conditionalNode->falseNextNode();
        std::string falseNextName;

        if (auto falseTableNode = dynamic_cast<TableNode*>(falseNextNode)) {
            Table *table = falseTableNode->data;
            falseNextName = absl::StrCat(table->getName(), "_", table->getId());
        } else if (auto falseConditionalNode = dynamic_cast<ConditionalNode*>(falseNextNode)) {
            Conditional *conditional = falseConditionalNode->data;
            falseNextName = absl::StrCat(conditional->getName(), "_", conditional->getId());
        } else {
            std::cerr << "Unknown type for True next!" << std::endl;
        }
        std::replace(falseNextName.begin(), falseNextName.end(), '.', '_');

        ConstantArrayType * nameArray = new ConstantArrayType(QualType(ctx.CharTy), APInt(APInt::APINT_BITS_PER_WORD, falseNextName.size()));
        QualType StrTy = QualType(nameArray);
        StringLiteral *NextPipeName = StringLiteral::Create(ctx, falseNextName, StringLiteral::Ascii, StrTy);

        /* Set actions.meta.pkt_meta */
        CALL_FUNC_WITH_REF(ctx, FlowGetPipeIdByName, GetTablePipeId, NextPipeName);
        MemberExpr *ActionsMetaAttr = MemberExpr::Create(ctx, ActionsVarRef, false, FlowMeta.PktMeta, FlowMatch.Meta->getDeclName(), FlowMatch.Meta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        MemberExpr *ActionsMetaPktmetaName = MemberExpr::Create(ctx, ActionsMetaAttr, false, nullptr, FlowMeta.PktMeta->getDeclName(), FlowMeta.PktMeta->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        BinaryOperator *ActionsMetaAssign = BinaryOperator::Create(ctx, ActionsMetaPktmetaName, CALL(GetTablePipeId), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(ActionsMetaAssign);

        /* Set fwd.next_pipe */
        CALL_FUNC_WITH_REF(ctx, FlowGetPipeByName, GetTablePipe, NextPipeName);
        BinaryOperator *NextPipeAssign = BinaryOperator::Create(ctx, NextPipeVarRef, CALL(GetTablePipe), BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(NextPipeAssign);

        MemberExpr *FwdFwdType = MemberExpr::Create(ctx, FwdVarRef, false, nullptr, FlowFwd.FwdType->getDeclName(), FlowFwd.FwdType->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        IdentifierInfo *FwdTypeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
        DeclRefExpr *FwdTypeRef = DeclRefExpr::Create(ctx, FlowFwdType.FwdPipe, FwdTypeId, QualType(ctx.UnsignedShortTy), ExprValueKind::VK_PRValue);
        BinaryOperator *FwdTypeAssign = BinaryOperator::Create(ctx, FwdFwdType, FwdTypeRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(FwdTypeAssign);

        MemberExpr *FwdFwdNextPipe = MemberExpr::Create(ctx, FwdVarRef, false, nullptr, FlowFwd.FwdNextPipe->getDeclName(), FlowFwd.FwdNextPipe->getType(), ExprValueKind::VK_LValue, OK_Ordinary);
        // IdentifierInfo *FwdNextPipeId = const_cast<IdentifierInfo*>(FlowFwdType.FwdPipe->getIdentifier());
        BinaryOperator *FwdNextPipeAssign = BinaryOperator::Create(ctx, FwdFwdNextPipe, NextPipeVarRef, BO_Assign, QualType(ctx.VoidTy), VK_LValue, OK_Ordinary);
        ruleStmt.push_back(FwdNextPipeAssign);

        std::vector<Expr *> ControlPipeAddEntryArgs;
        ControlPipeAddEntryArgs.push_back(REF(ArgPipe));
        ControlPipeAddEntryArgs.push_back(VARREF(Priority));
        ControlPipeAddEntryArgs.push_back(GetMatchAddr);
        ControlPipeAddEntryArgs.push_back(GetActionsAddr);
        ControlPipeAddEntryArgs.push_back(GetFwdAddr);
        IdentifierInfo *ControlPipeAddEntryId = const_cast<IdentifierInfo*>(FlowControlPipeAddEntry.Decl->getIdentifier());
        DeclRefExpr *ControlPipeAddEntryRef = DeclRefExpr::Create(ctx, FlowControlPipeAddEntry.Decl, ControlPipeAddEntryId, FlowControlPipeAddEntry.Type, ExprValueKind::VK_PRValue);
        CallExpr *ControlPipeAddEntryCall = CallExpr::Create(ctx, ControlPipeAddEntryRef, ControlPipeAddEntryArgs, FlowControlPipeAddEntry.Decl->getReturnType(), ExprValueKind::VK_PRValue);
        ruleStmt.push_back(ControlPipeAddEntryCall);

        auto *CS = CompoundStmt::Create(ctx, ruleStmt);
        Stmts.push_back(CS);
    }

    Stmts.push_back(Result);

    auto *CS = CompoundStmt::Create(ctx, Stmts);
    FuncDecl->setBody(CS);

    for (auto& pipe : j["pipes"]) {
        if (pipe["name"] == pipeName) {
            pipe["hw_ops"]["init_pipe"] = funcName;
            break;
        }
    }

    return true;
}

bool DocaProgram::build() {
#if 0
    /* Create a main function */
    IdentifierInfo *FuncId = &ctx.getIdentifierTable()->get("main");

    QualType FuncRetType = QualType(ctx.IntTy);

    std::vector<QualType> FuncArgs;
    QualType ArgcTy = QualType(ctx.IntTy);
    FuncArgs.push_back(ArgcTy);
    PointerType *charptr = new PointerType(QualType(ctx.CharTy));
    PointerType *charptrptr = new PointerType(QualType(charptr));
    QualType ArgvTy = QualType(charptrptr);
    FuncArgs.push_back(ArgvTy);

    QualType FuncType = ctx.getFunctionType(FuncRetType, FuncArgs);

    FunctionDecl *FuncDecl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), FuncId, FuncType, SC_None);

    std::vector<ParmVarDecl *> FuncDecls;
    IdentifierInfo *ArgcId = &ctx.getIdentifierTable()->get("argc");
    ParmVarDecl *Argc = ParmVarDecl::Create(ctx, FuncDecl, ArgcId, ArgcTy, SC_None);
    FuncDecls.push_back(Argc);

    IdentifierInfo *ArgvId = &ctx.getIdentifierTable()->get("argv");
    ParmVarDecl *Argv = ParmVarDecl::Create(ctx, FuncDecl, ArgvId, ArgvTy, SC_None);
    FuncDecls.push_back(Argv);

    FuncDecl->setParams(ctx, FuncDecls);

    ctx.getTranslationUnitDecl()->addDecl(FuncDecl);

    uint64_t IntSize = ctx.getTypeSize(QualType(ctx.IntTy));
    Expr *Zero = IntegerLiteral::Create(ctx, APInt(IntSize, 0), IntegerLiteral::Dec, QualType(ctx.IntTy));

    /* Create struct flow_match match */
    IdentifierInfo *MatchId = &ctx.getIdentifierTable()->get("match");
    VarDecl *MatchVar = VarDecl::Create(ctx, FuncDecl, MatchId, FlowMatch.Type, SC_None);
    Stmt *MatchStmt = DeclStmt::Create(ctx, DeclGroupRef(MatchVar));
    DeclRefExpr *MatchVarRef = DeclRefExpr::Create(ctx, MatchVar, MatchId, FlowMatch.Type, ExprValueKind::VK_LValue);

    /* Create memset */
    QualType MemsetRetType = QualType(ctx.IntTy);

    std::vector<QualType> MemsetArgsType;
    MemsetArgsType.push_back(QualType(ctx.VoidPtrTy));
    MemsetArgsType.push_back(QualType(ctx.IntTy));
    MemsetArgsType.push_back(QualType(ctx.UnsignedIntTy));
    QualType MemsetType = ctx.getFunctionType(MemsetRetType, MemsetArgsType);

    IdentifierInfo *MemsetId = &ctx.getIdentifierTable()->get("memset");
    FunctionDecl *MemsetDecl = FunctionDecl::Create(ctx, ctx.getTranslationUnitDecl(), MemsetId, MemsetType, SC_None);
    DeclRefExpr *MemsetRef = DeclRefExpr::Create(ctx, MemsetDecl, MemsetId, MemsetType, ExprValueKind::VK_PRValue);

    UnaryOperator *GetMatchAddr = UnaryOperator::Create(ctx, MatchVarRef, UnaryOperatorKind::UO_AddrOf, ctx.getPointerType(MatchVar->getType()), VK_PRValue, OK_Ordinary);
    UnaryExprOrTypeTraitExpr *GetMatchSize = UnaryExprOrTypeTraitExpr::Create(ctx, UETT_SizeOf, MatchVarRef, QualType(ctx.UnsignedIntTy));

    std::vector<Expr *> MemsetMatchArgs;
    MemsetMatchArgs.push_back(GetMatchAddr);
    MemsetMatchArgs.push_back(Zero);
    MemsetMatchArgs.push_back(GetMatchSize);
    CallExpr *Call = CallExpr::Create(ctx, MemsetRef, MemsetMatchArgs, FuncDecl->getReturnType(), ExprValueKind::VK_PRValue);

    auto *Result = ReturnStmt::Create(ctx, Zero);

    std::vector<Stmt *> Stmts;
    Stmts.push_back(MatchStmt);
    Stmts.push_back(Call);
    // Stmts.push_back(MatchOuterL4TypeAssign);
    for (auto call : CreatePipeCall) {
        Stmts.push_back(call);
    }
    Stmts.push_back(Result);
    auto *CS = CompoundStmt::Create(ctx, Stmts);

    FuncDecl->setBody(CS);
#endif
    return true;
}

void DocaProgram::emitGeneratedComment(CodeBuilder *builder) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    builder->append("/* Automatically generated on ");
    builder->append(std::ctime(&time));
    builder->append(" */");
    builder->newline();
}

void DocaProgram::emitH(CodeBuilder *builder, const std::filesystem::path &headerFile) {
    emitGeneratedComment(builder);
    builder->appendLine("#ifndef _NUTCRACKER_GEN_HEADER_");
    builder->appendLine("#define _NUTCRACKER_GEN_HEADER_");
    builder->newline();
    builder->target->emitIncludes(builder);
    builder->newline();
    builder->appendLine(
        "#define SET_MAC_ADDR(addr, a, b, c, d, e, f)\\\n"
        "do {\\\n"
        "\taddr[0] = a & 0xff;\\\n"
        "\taddr[1] = b & 0xff;\\\n"
        "\taddr[2] = c & 0xff;\\\n"
        "\taddr[3] = d & 0xff;\\\n"
        "\taddr[4] = e & 0xff;\\\n"
        "\taddr[5] = f & 0xff;\\\n"
        "} while (0)"
    );
    builder->appendLine("#endif");
}

void DocaProgram::emitC(CodeBuilder *builder, const std::filesystem::path &headerFile) {
    std::string header = "#include ";
    header = absl::StrCat(header, "\"", headerFile.filename().string(), "\"");
    builder->appendLine(header.c_str());
    builder->newline();
    ctx.emitC(builder);
}