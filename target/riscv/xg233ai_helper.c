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

void HELPER(xg233ai_vrelu)(CPURISCVState *env, target_ulong dst,
                            target_ulong src, target_ulong n)
{
    long count = (long)n;

    for (long i = 0; i < count; i++) {
        /* 从 guest 内存读取 INT32 */
        uint32_t val = cpu_ldl_data(env, src + i * 4);
        /* ReLU: max(0, val) */
        if ((int32_t)val < 0) {
            val = 0;
        }
        /* 写回 guest 内存 */
        cpu_stl_data(env, dst + i * 4, val);
    }
}
