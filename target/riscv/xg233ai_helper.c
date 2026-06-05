/*
 * Xg233ai custom instruction helpers
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exec/cputlb.h"
#include "accel/tcg/cpu-ldst.h"

// void HELPER(xg233ai_vrelu)(
//     CPURISCVState *env, 
//     target_ulong dst,           
//     target_ulong src, 
//     target_ulong n)
// {
//     long count = (long)n;

//     for (long i = 0; i < count; i++) {
//         /* 从 guest 内存读取 INT32 */
//         uint32_t val = cpu_ldl_data(env, src + i * 4);
//         /* ReLU: max(0, val) */
//         if ((int32_t)val < 0) {
//             val = 0;
//         }
//         /* 写回 guest 内存 */
//         cpu_stl_data(env, dst + i * 4, val);
//     }
// }

void HELPER(xg233ai_dma)(
    CPURISCVState *env,
    target_ulong dst,
    target_ulong src,
    target_ulong size_sel)
{
    int N;

    switch (size_sel) {
    case 0:
        N = 8;
        break;

    case 1:
        N = 16;
        break;

    case 2:
        N = 32;
        break;

    default:
        return;   /* undefined */
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {

            uint32_t val =
                cpu_ldl_data(
                    env,
                    src + (i * N + j) * 4
                );

            cpu_stl_data(
                env,
                dst + (j * N + i) * 4,
                val
            );
        }
    }
}

void HELPER(xg233ai_sort)(
    CPURISCVState *env,
    target_ulong k,         // 排序元素个数 
    target_ulong arr,       //  数组指针 
    target_ulong n)         // 数组总大小 
{
    long sort_len = (long)k;

    if (sort_len <= 0 || sort_len > 1024) {
        return;
    }

    uint32_t *tmp = g_new(uint32_t, sort_len);

    for (long i = 0; i < sort_len; i++) {
        tmp[i] = cpu_ldl_data(env, arr + i * 4);
    }

    for (long i = 0; i < sort_len - 1; i++) {
        for (long j = 0; j < sort_len - i - 1; j++) {
            if ((int32_t)tmp[j] > (int32_t)tmp[j + 1]) {
                uint32_t t = tmp[j];
                tmp[j] = tmp[j + 1];
                tmp[j + 1] = t;
            }
        }
    }

    for (long i = 0; i < sort_len; i++) {
        cpu_stl_data(env, arr + i * 4, tmp[i]);
    }

    g_free(tmp);
}

void HELPER(xg233ai_crush)(
    CPURISCVState *env,
    target_ulong dst,
    target_ulong src,
    target_ulong n)
{
    long count = (long)n;

    for (long i = 0; i < count / 2; i++) {
        uint8_t low = cpu_ldub_data(env, src + i * 2);
        uint8_t high = cpu_ldub_data(env, src + i * 2 + 1);
        uint8_t crushed = (low & 0x0f) | ((high & 0x0f) << 4);
        cpu_stb_data(env, dst + i, crushed);
    }

    if (count & 1) {
        uint8_t last = cpu_ldub_data(env, src + count - 1);
        cpu_stb_data(env, dst + count / 2, last & 0x0f);
    }
}

void HELPER(xg233ai_expand)(
    CPURISCVState *env,
    target_ulong dst,
    target_ulong src,
    target_ulong n)
{
    long count = (long)n;

    for (long i = 0; i < count; i++) {
        uint8_t crushed = cpu_ldub_data(env, src + i);
        uint8_t low = crushed & 0x0f;
        uint8_t high = (crushed >> 4) & 0x0f;
        cpu_stb_data(env, dst + i * 2, low);
        cpu_stb_data(env, dst + i * 2 + 1, high);
    }
}

target_ulong HELPER(xg233ai_vdot)(
    CPURISCVState *env,
    target_ulong a_ptr,     // rs1
    target_ulong b_ptr)     // rs2
{
    int64_t acc = 0;
    for (int i = 0; i < 16; i++) {       
        int32_t a = (int32_t)cpu_ldl_data(env, a_ptr + i * 4);
        int32_t b = (int32_t)cpu_ldl_data(env, b_ptr + i * 4);
        acc += (int64_t)a * (int64_t)b;
    }
    return (target_ulong)acc;
}

void HELPER(xg233ai_vscale)(
    CPURISCVState *env,
    target_ulong dst,       // rd
    target_ulong src,       // rs1 
    target_ulong scale)     // rs2 
{
    int64_t s = (int64_t)(long)scale;

    for (int i = 0; i < 16; i++) {
        int32_t val = (int32_t)cpu_ldl_data(env, src + i * 4);
        cpu_stl_data(env, dst + i * 4, (uint32_t)((int64_t)val * s));
    }
}

target_ulong HELPER(xg233ai_vmax)(
    CPURISCVState *env,
    target_ulong src,       // rs1 
    target_ulong n)         // rs2 
{
    int count = (int)(long)n;
    int32_t max_val = (int32_t)cpu_ldl_data(env, src);

    for (int i = 1; i < count; i++) {
        int32_t val = (int32_t)cpu_ldl_data(env, src + i * 4);
        if (val > max_val) {
            max_val = val;
        }
    }
    
    return (target_ulong)(int64_t)max_val;
}

void HELPER(xg233ai_gemm)(
    CPURISCVState *env,
    target_ulong c,         // rd 
    target_ulong a,         // rs1 
    target_ulong b)         // rs2 
{
    int n = 4;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int64_t acc = 0;
            for (int k = 0; k < n; k++) {
                int32_t aik = (int32_t)cpu_ldl_data(env, a + (i * n + k) * 4);
                int32_t bkj = (int32_t)cpu_ldl_data(env, b + (k * n + j) * 4);
                acc += (int64_t)aik * (int64_t)bkj;
            }
            cpu_stl_data(env, c + (i * n + j) * 4, (uint32_t)acc);
        }
    }
}

/*
void HELPER(xg233ai_vadd)(
    CPURISCVState *env,
    target_ulong c,         // rd
    target_ulong a,         // rs1
    target_ulong b)         // rs2
{
    for (int i = 0; i < 16; i++) {
        uint32_t av = cpu_ldl_data(env, a + i * 4);
        uint32_t bv = cpu_ldl_data(env, b + i * 4);
        cpu_stl_data(env, c + i * 4, av + bv);
    }
}
 */
