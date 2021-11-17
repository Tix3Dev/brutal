#include <bid/builtin.h>
#include <bid/gen/c.h>
#include <brutal/debug.h>
#include <cc/builder.h>

static CType gen_decl_type(BidType type, Alloc *alloc);

/* --- C Header ----------------------------------------- */

static CType gen_decl_enum(BidType type, Alloc *alloc)
{
    CType ctype = ctype_enum(alloc);

    int i = 0;
    vec_foreach(member, &type.enum_.members)
    {
        ctype_constant(&ctype, member.mangled, cval_signed(i++), alloc);
    }

    return ctype;
}

static CType gen_decl_struct(BidType type, Alloc *alloc)
{
    CType ctype = ctype_struct(alloc);

    vec_foreach(member, &type.struct_.members)
    {
        ctype_member(&ctype, member.name, gen_decl_type(member.type, alloc), alloc);
    }

    return ctype;
}

static CType gen_decl_vec(BidType type, Alloc *alloc)
{
    CType ctype = ctype_struct(alloc);

    ctype_member(&ctype, str$("buf"), ctype_ptr(gen_decl_type(*type.vec_.subtype, alloc), alloc), alloc);
    ctype_member(&ctype, str$("len"), ctype_name(str$("size_t"), alloc), alloc);

    return ctype;
}

static CType gen_decl_type(BidType type, Alloc *alloc)
{
    switch (type.type)
    {
    case BID_TYPE_NIL:
        return ctype_void();

    case BID_TYPE_PRIMITIVE:
        return ctype_name(type.primitive_.mangled, alloc);

    case BID_TYPE_ENUM:
        return gen_decl_enum(type, alloc);

    case BID_TYPE_STRUCT:
        return gen_decl_struct(type, alloc);

    case BID_TYPE_VEC:
        return gen_decl_vec(type, alloc);

    default:
        panic$("Unknow type type {}", type.type);
    }
}

static CType gen_type(BidType type, Alloc *alloc)
{
    assert_truth(type.type == BID_TYPE_PRIMITIVE);
    return ctype_name(type.primitive_.mangled, alloc);
}

static CType gen_func_method(BidMethod method, BidIface const iface, Alloc *alloc)
{
    CType ctype = ctype_func(gen_type(iface.errors, alloc), alloc);

    ctype_member(&ctype, str$("ev"), ctype_ptr(ctype_name(str$("IpcEv"), alloc), alloc), alloc);

    ctype_member(&ctype, str$("task"), ctype_name(str$("BrTask"), alloc), alloc);

    if (method.request.type != BID_TYPE_NIL)
    {
        ctype_member(&ctype, str$("req"), ctype_ptr(gen_type(method.request, alloc), alloc), alloc);
    }

    if (method.response.type != BID_TYPE_NIL)
    {
        ctype_member(&ctype, str$("resp"), ctype_ptr(gen_type(method.response, alloc), alloc), alloc);
    }

    ctype_member(&ctype, str$("alloc"), ctype_ptr(ctype_name(str$("Alloc"), alloc), alloc), alloc);

    return ctype;
}

static CType gen_func_impl(BidIface const iface, Alloc *alloc)
{
    CType ctype = ctype_func(ctype_void(), alloc);
    ctype_member(&ctype, str$("self"), ctype_ptr(ctype_name(str$("IpcEv"), alloc), alloc), alloc);
    ctype_member(&ctype, str$("vtable"), ctype_ptr(ctype_name(str_fmt(alloc, "{}VTable", iface.name), alloc), alloc), alloc);
    return ctype;
}

CUnit bidgen_c_header(MAYBE_UNUSED BidIface const iface, Alloc *alloc)
{
    CUnit unit = cunit(alloc);
    cunit_pragma_once(&unit, alloc);
    cunit_include(&unit, true, str_fmt(alloc, "bal/ipc.h", iface.name), alloc);

    vec_foreach(alias, &iface.aliases)
    {
        cunit_decl(&unit, cdecl_type(alias.mangled, gen_decl_type(alias.type, alloc), alloc));
    }

    int i = 0;
    CType vtable = ctype_struct(alloc);
    CType msgtype = ctype_enum(alloc);

    ctype_constant(&msgtype, str_fmt(alloc, "MSG_{case:constant}_ERR", iface.name), cval_unsigned(i++), alloc);
    vec_foreach(method, &iface.methods)
    {
        Str name = case_change_str(CASE_PASCAL, method.mangled, alloc);
        CType type = gen_func_method(method, iface, alloc);

        ctype_member(&vtable, method.name, ctype_ptr(ctype_name(name, alloc), alloc), alloc);
        cunit_decl(&unit, cdecl_type(name, type, alloc));
        cunit_decl(&unit, cdecl_func(method.mangled, type, cstmt_empty(), alloc));

        ctype_constant(&msgtype, str_fmt(alloc, "MSG_{case:constant}_REQ", method.mangled), cval_unsigned(i++), alloc);
        ctype_constant(&msgtype, str_fmt(alloc, "MSG_{case:constant}_RESP", method.mangled), cval_unsigned(i++), alloc);
    }

    cunit_decl(&unit, cdecl_type(str_fmt(alloc, "{}Msgs", iface.name), msgtype, alloc));
    cunit_decl(&unit, cdecl_type(str_fmt(alloc, "{}VTable", iface.name), vtable, alloc));
    cunit_decl(&unit, cdecl_func(str_fmt(alloc, "{case:lower}_impl", iface.name), gen_func_impl(iface, alloc), cstmt_empty(), alloc));

    return unit;
}

/* --- C Source ----------------------------------------- */

Str gen_pack_name(Str name, Alloc *alloc)
{
    BidBuiltinType *builtin = bid_lookup_builtin(name);

    if (builtin)
    {
        return builtin->pack_fn;
    }

    return str_fmt(alloc, "{case:snake}_pack", name);
}

Str gen_unpack_name(Str name, Alloc *alloc)
{
    BidBuiltinType *builtin = bid_lookup_builtin(name);

    if (builtin)
    {
        return builtin->unpack_fn;
    }

    return str_fmt(alloc, "{case:snake}_unpack", name);
}

CType gen_pack_type(BidAlias alias, Alloc *alloc)
{
    CType ctype = ctype_func(ctype_void(), alloc);
    ctype_member(&ctype, str$("self"), ctype_ptr(ctype_name(str$("BalPack"), alloc), alloc), alloc);
    ctype_member(&ctype, str$("data"), ctype_ptr(ctype_name(alias.mangled, alloc), alloc), alloc);
    return ctype;
}

CType gen_unpack_type(BidAlias alias, Alloc *alloc)
{
    CType ctype = ctype_func(ctype_void(), alloc);
    ctype_member(&ctype, str$("self"), ctype_ptr(ctype_name(str$("BalUnpack"), alloc), alloc), alloc);
    ctype_member(&ctype, str$("data"), ctype_ptr(ctype_name(alias.mangled, alloc), alloc), alloc);
    return ctype;
}

void gen_pack_body(CStmt *block, BidType type, CExpr path, Alloc *alloc)
{
    switch (type.type)
    {
    case BID_TYPE_NIL:
        break;

    case BID_TYPE_PRIMITIVE:
    {
        CExpr expr = cexpr_call(alloc, cexpr_ident(gen_pack_name(type.primitive_.mangled, alloc), alloc));
        cexpr_member(&expr, cexpr_ident(str$("self"), alloc));
        cexpr_member(&expr, cexpr_ref(path, alloc));

        cstmt_block_add(block, cstmt_expr(expr));
    }
    break;

    case BID_TYPE_ENUM:
    {
        CExpr expr = cexpr_call(alloc, cexpr_ident(gen_pack_name(str$("Enum"), alloc), alloc));
        cexpr_member(&expr, cexpr_ident(str$("self"), alloc));
        cexpr_member(&expr, cexpr_cast(cexpr_ref(path, alloc), ctype_ptr(ctype_name(str$("int"), alloc), alloc), alloc));

        cstmt_block_add(block, cstmt_expr(expr));
    }
    break;

    case BID_TYPE_STRUCT:
    {
        vec_foreach(member, &type.struct_.members)
        {
            gen_pack_body(block, member.type, cexpr_access(path, cexpr_ident(member.name, alloc), alloc), alloc);
        }
    }
    break;

    case BID_TYPE_VEC:
    {
        BidType subtype = *type.vec_.subtype;
        assert_truth(subtype.type == BID_TYPE_PRIMITIVE);
        CExpr expr = cexpr_call(alloc, cexpr_ident(str$("bal_pack_slice"), alloc));
        cexpr_member(&expr, cexpr_ident(str$("self"), alloc));
        cexpr_member(&expr, cexpr_ref(path, alloc));
        cexpr_member(&expr, cexpr_ident(gen_pack_name(subtype.primitive_.mangled, alloc), alloc));
        cstmt_block_add(block, cstmt_expr(expr));
    }

    break;
        panic$("Unknow type type {}", type.type);
    }
}

void gen_unpack_body(CStmt *block, BidType type, CExpr path, Alloc *alloc)
{
    switch (type.type)
    {
    case BID_TYPE_NIL:
        break;

    case BID_TYPE_PRIMITIVE:
    {
        CExpr expr = cexpr_call(alloc, cexpr_ident(gen_unpack_name(type.primitive_.mangled, alloc), alloc));
        cexpr_member(&expr, cexpr_ident(str$("self"), alloc));
        cexpr_member(&expr, cexpr_ref(path, alloc));

        cstmt_block_add(block, cstmt_expr(expr));
    }
    break;

    case BID_TYPE_ENUM:
    {
        CExpr expr = cexpr_call(alloc, cexpr_ident(gen_unpack_name(str$("Enum"), alloc), alloc));
        cexpr_member(&expr, cexpr_ident(str$("self"), alloc));
        cexpr_member(&expr, cexpr_cast(cexpr_ref(path, alloc), ctype_ptr(ctype_name(str$("int"), alloc), alloc), alloc));

        cstmt_block_add(block, cstmt_expr(expr));
    }
    break;

    case BID_TYPE_STRUCT:
    {
        vec_foreach(member, &type.struct_.members)
        {
            gen_unpack_body(block, member.type, cexpr_access(path, cexpr_ident(member.name, alloc), alloc), alloc);
        }
    }
    break;

    case BID_TYPE_VEC:
    {
        BidType subtype = *type.vec_.subtype;
        assert_truth(subtype.type == BID_TYPE_PRIMITIVE);
        CExpr expr = cexpr_call(alloc, cexpr_ident(str$("bal_unpack_slice"), alloc));
        cexpr_member(&expr, cexpr_ident(str$("self"), alloc));
        cexpr_member(&expr, cexpr_ref(path, alloc));
        cexpr_member(&expr, cexpr_ident(gen_unpack_name(subtype.primitive_.mangled, alloc), alloc));
        cstmt_block_add(block, cstmt_expr(expr));
    }

    break;
        panic$("Unknow type type {}", type.type);
    }
}

CDecl gen_pack_func(BidAlias alias, Alloc *alloc)
{
    Str name = gen_pack_name(alias.mangled, alloc);
    CType type = gen_pack_type(alias, alloc);
    CStmt body = cstmt_block(alloc);
    gen_pack_body(&body, alias.type, cexpr_deref(cexpr_ident(str$("data"), alloc), alloc), alloc);

    return cdecl_func(name, type, body, alloc);
}

CDecl gen_unpack_func(BidAlias alias, Alloc *alloc)
{
    Str name = gen_unpack_name(alias.mangled, alloc);
    CType type = gen_unpack_type(alias, alloc);
    CStmt body = cstmt_block(alloc);
    gen_unpack_body(&body, alias.type, cexpr_deref(cexpr_ident(str$("data"), alloc), alloc), alloc);

    return cdecl_func(name, type, body, alloc);
}

CStmt gen_method_body(BidMethod method, BidIface const iface, Alloc *alloc)
{
    CStmt block = cstmt_block(alloc);

    CExpr pack_request = cexpr_call(alloc, cexpr_ident(str$("br_ev_req"), alloc));
    cexpr_member(&pack_request, cexpr_ident(str$("ev"), alloc));
    cexpr_member(&pack_request, cexpr_ident(str$("task"), alloc));
    cexpr_member(&pack_request, cexpr_constant(cval_signed(iface.id)));

    cexpr_member(&pack_request, cexpr_ident(str_fmt(alloc, "MSG_{case:constant}_REQ", method.mangled), alloc));

    if (method.request.type != BID_TYPE_NIL)
    {
        cexpr_member(&pack_request, cexpr_ident(str$("req"), alloc));
        cexpr_member(&pack_request, cexpr_ident(gen_pack_name(method.request.primitive_.mangled, alloc), alloc));
    }
    else
    {
        cexpr_member(&pack_request, cexpr_ident(str$("nullptr"), alloc));
        cexpr_member(&pack_request, cexpr_ident(str$("nullptr"), alloc));
    }

    cexpr_member(&pack_request, cexpr_ident(str_fmt(alloc, "MSG_{case:constant}_RESP", method.mangled), alloc));

    if (method.response.type != BID_TYPE_NIL)
    {
        cexpr_member(&pack_request, cexpr_ident(str$("resp"), alloc));
        cexpr_member(&pack_request, cexpr_ident(gen_unpack_name(method.response.primitive_.mangled, alloc), alloc));
    }
    else
    {
        cexpr_member(&pack_request, cexpr_ident(str$("nullptr"), alloc));
        cexpr_member(&pack_request, cexpr_ident(str$("nullptr"), alloc));
    }

    cexpr_member(&pack_request, cexpr_ident(str$("alloc"), alloc));

    cstmt_block_add(&block, cstmt_return(pack_request));

    return block;
}

CUnit bidgen_c_source(MAYBE_UNUSED BidIface const iface, Alloc *alloc)
{
    CUnit unit = cunit(alloc);

    cunit_include(&unit, false, str_fmt(alloc, "{case:snake}.h", iface.name), alloc);

    vec_foreach(alias, &iface.aliases)
    {
        cunit_decl(&unit, gen_pack_func(alias, alloc));
        cunit_decl(&unit, gen_unpack_func(alias, alloc));
    }

    vec_foreach(method, &iface.methods)
    {
        CType type = gen_func_method(method, iface, alloc);
        CStmt body = gen_method_body(method, iface, alloc);

        cunit_decl(&unit, cdecl_func(method.mangled, type, body, alloc));
    }

    return unit;
}
