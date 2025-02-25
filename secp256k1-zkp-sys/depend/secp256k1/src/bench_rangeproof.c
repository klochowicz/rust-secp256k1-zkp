/**********************************************************************
 * Copyright (c) 2014, 2015 Pieter Wuille, Gregory Maxwell            *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdint.h>

#include "include/secp256k1_rangeproof.h"
#include "util.h"
#include "bench.h"

typedef struct {
    rustsecp256k1zkp_v0_5_0_context* ctx;
    rustsecp256k1zkp_v0_5_0_pedersen_commitment commit;
    unsigned char proof[5134];
    unsigned char blind[32];
    size_t len;
    int min_bits;
    uint64_t v;
} bench_rangeproof_t;

static void bench_rangeproof_setup(void* arg) {
    int i;
    uint64_t minv;
    uint64_t maxv;
    bench_rangeproof_t *data = (bench_rangeproof_t*)arg;

    data->v = 0;
    for (i = 0; i < 32; i++) data->blind[i] = i + 1;
    CHECK(rustsecp256k1zkp_v0_5_0_pedersen_commit(data->ctx, &data->commit, data->blind, data->v, rustsecp256k1zkp_v0_5_0_generator_h));
    data->len = 5134;
    CHECK(rustsecp256k1zkp_v0_5_0_rangeproof_sign(data->ctx, data->proof, &data->len, 0, &data->commit, data->blind, (const unsigned char*)&data->commit, 0, data->min_bits, data->v, NULL, 0, NULL, 0, rustsecp256k1zkp_v0_5_0_generator_h));
    CHECK(rustsecp256k1zkp_v0_5_0_rangeproof_verify(data->ctx, &minv, &maxv, &data->commit, data->proof, data->len, NULL, 0, rustsecp256k1zkp_v0_5_0_generator_h));
}

static void bench_rangeproof(void* arg, int iters) {
    int i;
    bench_rangeproof_t *data = (bench_rangeproof_t*)arg;

    for (i = 0; i < iters/data->min_bits; i++) {
        int j;
        uint64_t minv;
        uint64_t maxv;
        j = rustsecp256k1zkp_v0_5_0_rangeproof_verify(data->ctx, &minv, &maxv, &data->commit, data->proof, data->len, NULL, 0, rustsecp256k1zkp_v0_5_0_generator_h);
        for (j = 0; j < 4; j++) {
            data->proof[j + 2 + 32 *((data->min_bits + 1) >> 1) - 4] = (i >> 8)&255;
        }
    }
}

int main(void) {
    bench_rangeproof_t data;
    int iters;

    data.ctx = rustsecp256k1zkp_v0_5_0_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    data.min_bits = 32;
    iters = data.min_bits*get_iters(32);

    run_benchmark("rangeproof_verify_bit", bench_rangeproof, bench_rangeproof_setup, NULL, &data, 10, iters);

    rustsecp256k1zkp_v0_5_0_context_destroy(data.ctx);
    return 0;
}
