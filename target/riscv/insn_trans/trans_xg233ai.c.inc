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

GEN_XG233AI_HELPER3(crush)
GEN_XG233AI_HELPER3(dma)
GEN_XG233AI_HELPER3(expand)
GEN_XG233AI_HELPER3(sort)
/* GEN_XG233AI_HELPER3(vadd) -- [已替换为内联 TCG 版本，helper 保留用于对比] */
GEN_XG233AI_HELPER2_RET(vdot)   /* 返回 long */
GEN_XG233AI_HELPER2_RET(vmax)   /* 返回 long */
GEN_XG233AI_HELPER3(vrelu)
GEN_XG233AI_HELPER3(vscale)
GEN_XG233AI_HELPER3(gemm)

/* ================================================================
 * 内联 TCG IR 版本：vadd
 *
 * 语义: C[i] = A[i] + B[i], i in 0..15, 每个元素 int32
 * 寄存器映射: rd=C指针, rs1=A指针, rs2=B指针
 *
 * 展开 16 次 ld+add+st, 全部在翻译块内完成,
 * 消除 helper 调用的保存/恢复/重入开销。
 *
 * 注意: 不能用 tcg_gen_gvec_add_var() ——
 * gvec 系列函数的 TCGv_ptr 是 Host 内存地址 (env+offset),
 * 而这里 rd/rs1/rs2 存的是 Guest 地址, 必须通过 qemu_ld/st
 * 走 softmmu 翻译。
 * ================================================================ */
static bool trans_xg233ai_vadd(DisasContext *ctx, arg_r *a)
{
    /* 复制 Guest 地址到本地 TCG 临时变量, 避免修改 GPR 值 */
    TCGv ptr_c = tcg_temp_new();
    TCGv ptr_a = tcg_temp_new();
    TCGv ptr_b = tcg_temp_new();
    tcg_gen_mov_tl(ptr_c, get_gpr(ctx, a->rd,  EXT_NONE));
    tcg_gen_mov_tl(ptr_a, get_gpr(ctx, a->rs1, EXT_NONE));
    tcg_gen_mov_tl(ptr_b, get_gpr(ctx, a->rs2, EXT_NONE));

    /* 展开 16 次: ld A[i], ld B[i], add, st C[i] */
    for (int i = 0; i < 16; i++) {
        TCGv_i32 av = tcg_temp_new_i32();
        TCGv_i32 bv = tcg_temp_new_i32();

        tcg_gen_qemu_ld_i32(av, ptr_a, ctx->mem_idx, MO_TESL);
        tcg_gen_qemu_ld_i32(bv, ptr_b, ctx->mem_idx, MO_TESL);
        tcg_gen_add_i32(av, av, bv);
        tcg_gen_qemu_st_i32(av, ptr_c, ctx->mem_idx, MO_TESL);

        /* 最后一轮不推进指针 */
        if (i < 15) {
            tcg_gen_addi_tl(ptr_c, ptr_c, 4);
            tcg_gen_addi_tl(ptr_a, ptr_a, 4);
            tcg_gen_addi_tl(ptr_b, ptr_b, 4);
        }
    }

    return true;
}

#undef GEN_XG233AI_HELPER3
#undef GEN_XG233AI_HELPER2_RET