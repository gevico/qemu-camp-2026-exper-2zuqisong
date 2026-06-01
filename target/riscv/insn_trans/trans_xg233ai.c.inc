/*
 * RISC-V translation routines for the Xg233ai extension.
 *
 */

/* 生成 helper 调用的辅助宏 */
#define GEN_XG233AI_HELPER3(name) \
static bool trans_xg233ai_##name(DisasContext *ctx, arg_r *a) \
{ \
    TCGv dest = get_gpr(ctx, a->rd, EXT_NONE); \
    TCGv src1 = get_gpr(ctx, a->rs1, EXT_NONE); \
    TCGv src2 = get_gpr(ctx, a->rs2, EXT_NONE); \
    gen_helper_xg233ai_##name(tcg_env, dest, src1, src2); \
    return true; \
}

/* 有返回值的 helper */
#define GEN_XG233AI_HELPER2_RET(name) \
static bool trans_xg233ai_##name(DisasContext *ctx, arg_r *a) \
{ \
    TCGv dest = get_gpr(ctx, a->rd, EXT_NONE); \
    TCGv src1 = get_gpr(ctx, a->rs1, EXT_NONE); \
    TCGv src2 = get_gpr(ctx, a->rs2, EXT_NONE); \
    gen_helper_xg233ai_##name(dest, tcg_env, src1, src2); \
    gen_set_gpr(ctx, a->rd, dest); \
    return true; \
}

GEN_XG233AI_HELPER3(vrelu)

#undef GEN_XG233AI_HELPER3
#undef GEN_XG233AI_HELPER2_RET