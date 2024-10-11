#include "AST/Attr.h"
#include "AST/Stmt.h"
#include "backends/target.h"
#include "cProgram.h"

bool CProgram::buildHeaderStructure() {
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


bool CProgram::initPipe(Pipeline::Kind K, const CallGraphNodeBase* node) {
    if (const auto* actionNode = dynamic_cast<const ActionNode*>(node)) {
        std::cout << "  Generating initialization for " <<  "\033[32m" << " action " << "\033[0m" << " pipe: " << actionNode->getName();
        buildActionPipeRun(actionNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    } else if (const auto* tableNode = dynamic_cast<const TableNode*>(node)) {
        std::cout << "  Generating initialization for " <<  "\033[32m" << " table " << "\033[0m" << " pipe: " << tableNode->getName();
        // initTablePipe(tableNode);
        buildAddTablePipeEntry(tableNode);
        buildTablePipeRun(tableNode);
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

bool CProgram::buildPipe(Pipeline::Kind K, const CallGraphNodeBase* node, bool isRoot) {
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
