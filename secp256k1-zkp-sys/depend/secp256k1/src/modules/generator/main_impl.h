/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra & Pieter Wuille                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_GENERATOR_MAIN
#define SECP256K1_MODULE_GENERATOR_MAIN

#include <stdio.h>

#include "field.h"
#include "group.h"
#include "hash.h"
#include "scalar.h"

static void rustsecp256k1zkp_v0_5_0_generator_load(rustsecp256k1zkp_v0_5_0_ge* ge, const rustsecp256k1zkp_v0_5_0_generator* gen) {
    int succeed;
    succeed = rustsecp256k1zkp_v0_5_0_fe_set_b32(&ge->x, &gen->data[0]);
    VERIFY_CHECK(succeed != 0);
    succeed = rustsecp256k1zkp_v0_5_0_fe_set_b32(&ge->y, &gen->data[32]);
    VERIFY_CHECK(succeed != 0);
    ge->infinity = 0;
    (void) succeed;
}

static void rustsecp256k1zkp_v0_5_0_generator_save(rustsecp256k1zkp_v0_5_0_generator *gen, rustsecp256k1zkp_v0_5_0_ge* ge) {
    VERIFY_CHECK(!rustsecp256k1zkp_v0_5_0_ge_is_infinity(ge));
    rustsecp256k1zkp_v0_5_0_fe_normalize_var(&ge->x);
    rustsecp256k1zkp_v0_5_0_fe_normalize_var(&ge->y);
    rustsecp256k1zkp_v0_5_0_fe_get_b32(&gen->data[0], &ge->x);
    rustsecp256k1zkp_v0_5_0_fe_get_b32(&gen->data[32], &ge->y);
}

int rustsecp256k1zkp_v0_5_0_generator_parse(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_generator* gen, const unsigned char *input) {
    rustsecp256k1zkp_v0_5_0_fe x;
    rustsecp256k1zkp_v0_5_0_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(input != NULL);

    if ((input[0] & 0xFE) != 10 ||
        !rustsecp256k1zkp_v0_5_0_fe_set_b32(&x, &input[1]) ||
        !rustsecp256k1zkp_v0_5_0_ge_set_xquad(&ge, &x)) {
        return 0;
    }
    if (input[0] & 1) {
        rustsecp256k1zkp_v0_5_0_ge_neg(&ge, &ge);
    }
    rustsecp256k1zkp_v0_5_0_generator_save(gen, &ge);
    return 1;
}

int rustsecp256k1zkp_v0_5_0_generator_serialize(const rustsecp256k1zkp_v0_5_0_context* ctx, unsigned char *output, const rustsecp256k1zkp_v0_5_0_generator* gen) {
    rustsecp256k1zkp_v0_5_0_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(gen != NULL);

    rustsecp256k1zkp_v0_5_0_generator_load(&ge, gen);

    output[0] = 11 ^ rustsecp256k1zkp_v0_5_0_fe_is_quad_var(&ge.y);
    rustsecp256k1zkp_v0_5_0_fe_normalize_var(&ge.x);
    rustsecp256k1zkp_v0_5_0_fe_get_b32(&output[1], &ge.x);
    return 1;
}

static void shallue_van_de_woestijne(rustsecp256k1zkp_v0_5_0_ge* ge, const rustsecp256k1zkp_v0_5_0_fe* t) {
    /* Implements the algorithm from:
     *    Indifferentiable Hashing to Barreto-Naehrig Curves
     *    Pierre-Alain Fouque and Mehdi Tibouchi
     *    Latincrypt 2012
     */

    /* Basic algorithm:

       c = sqrt(-3)
       d = (c - 1)/2

       w = c * t / (1 + b + t^2)  [with b = 7]
       x1 = d - t*w
       x2 = -(x1 + 1)
       x3 = 1 + 1/w^2

       To avoid the 2 divisions, compute the above in numerator/denominator form:
       wn = c * t
       wd = 1 + 7 + t^2
       x1n = d*wd - t*wn
       x1d = wd
       x2n = -(x1n + wd)
       x2d = wd
       x3n = wd^2 + c^2 + t^2
       x3d = (c * t)^2

       The joint denominator j = wd * c^2 * t^2, and
       1 / x1d = 1/j * c^2 * t^2
       1 / x2d = x3d = 1/j * wd
    */

    static const rustsecp256k1zkp_v0_5_0_fe c = SECP256K1_FE_CONST(0x0a2d2ba9, 0x3507f1df, 0x233770c2, 0xa797962c, 0xc61f6d15, 0xda14ecd4, 0x7d8d27ae, 0x1cd5f852);
    static const rustsecp256k1zkp_v0_5_0_fe d = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa40);
    static const rustsecp256k1zkp_v0_5_0_fe b = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 7);
    static const rustsecp256k1zkp_v0_5_0_fe b_plus_one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 8);

    rustsecp256k1zkp_v0_5_0_fe wn, wd, x1n, x2n, x3n, x3d, jinv, tmp, x1, x2, x3, alphain, betain, gammain, y1, y2, y3;
    int alphaquad, betaquad;

    rustsecp256k1zkp_v0_5_0_fe_mul(&wn, &c, t); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_sqr(&wd, t); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_add(&wd, &b_plus_one); /* mag 2 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&tmp, t, &wn); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_negate(&tmp, &tmp, 1); /* mag 2 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x1n, &d, &wd); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_add(&x1n, &tmp); /* mag 3 */
    x2n = x1n; /* mag 3 */
    rustsecp256k1zkp_v0_5_0_fe_add(&x2n, &wd); /* mag 5 */
    rustsecp256k1zkp_v0_5_0_fe_negate(&x2n, &x2n, 5); /* mag 6 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x3d, &c, t); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_sqr(&x3d, &x3d); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_sqr(&x3n, &wd); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_add(&x3n, &x3d); /* mag 2 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&jinv, &x3d, &wd); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_inv(&jinv, &jinv); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x1, &x1n, &x3d); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x1, &x1, &jinv); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x2, &x2n, &x3d); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x2, &x2, &jinv); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x3, &x3n, &wd); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&x3, &x3, &jinv); /* mag 1 */

    rustsecp256k1zkp_v0_5_0_fe_sqr(&alphain, &x1); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&alphain, &alphain, &x1); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_add(&alphain, &b); /* mag 2 */
    rustsecp256k1zkp_v0_5_0_fe_sqr(&betain, &x2); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&betain, &betain, &x2); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_add(&betain, &b); /* mag 2 */
    rustsecp256k1zkp_v0_5_0_fe_sqr(&gammain, &x3); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_mul(&gammain, &gammain, &x3); /* mag 1 */
    rustsecp256k1zkp_v0_5_0_fe_add(&gammain, &b); /* mag 2 */

    alphaquad = rustsecp256k1zkp_v0_5_0_fe_sqrt(&y1, &alphain);
    betaquad = rustsecp256k1zkp_v0_5_0_fe_sqrt(&y2, &betain);
    rustsecp256k1zkp_v0_5_0_fe_sqrt(&y3, &gammain);

    rustsecp256k1zkp_v0_5_0_fe_cmov(&x1, &x2, (!alphaquad) & betaquad);
    rustsecp256k1zkp_v0_5_0_fe_cmov(&y1, &y2, (!alphaquad) & betaquad);
    rustsecp256k1zkp_v0_5_0_fe_cmov(&x1, &x3, (!alphaquad) & !betaquad);
    rustsecp256k1zkp_v0_5_0_fe_cmov(&y1, &y3, (!alphaquad) & !betaquad);

    rustsecp256k1zkp_v0_5_0_ge_set_xy(ge, &x1, &y1);

    /* The linked algorithm from the paper uses the Jacobi symbol of t to
     * determine the Jacobi symbol of the produced y coordinate. Since the
     * rest of the algorithm only uses t^2, we can safely use another criterion
     * as long as negation of t results in negation of the y coordinate. Here
     * we choose to use t's oddness, as it is faster to determine. */
    rustsecp256k1zkp_v0_5_0_fe_negate(&tmp, &ge->y, 1);
    rustsecp256k1zkp_v0_5_0_fe_cmov(&ge->y, &tmp, rustsecp256k1zkp_v0_5_0_fe_is_odd(t));
}

static int rustsecp256k1zkp_v0_5_0_generator_generate_internal(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_generator* gen, const unsigned char *key32, const unsigned char *blind32) {
    static const unsigned char prefix1[17] = "1st generation: ";
    static const unsigned char prefix2[17] = "2nd generation: ";
    rustsecp256k1zkp_v0_5_0_fe t = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 4);
    rustsecp256k1zkp_v0_5_0_ge add;
    rustsecp256k1zkp_v0_5_0_gej accum;
    int overflow;
    rustsecp256k1zkp_v0_5_0_sha256 sha256;
    unsigned char b32[32];
    int ret = 1;

    if (blind32) {
        rustsecp256k1zkp_v0_5_0_scalar blind;
        rustsecp256k1zkp_v0_5_0_scalar_set_b32(&blind, blind32, &overflow);
        ret = !overflow;
        rustsecp256k1zkp_v0_5_0_ecmult_gen(&ctx->ecmult_gen_ctx, &accum, &blind);
    }

    rustsecp256k1zkp_v0_5_0_sha256_initialize(&sha256);
    rustsecp256k1zkp_v0_5_0_sha256_write(&sha256, prefix1, 16);
    rustsecp256k1zkp_v0_5_0_sha256_write(&sha256, key32, 32);
    rustsecp256k1zkp_v0_5_0_sha256_finalize(&sha256, b32);
    ret &= rustsecp256k1zkp_v0_5_0_fe_set_b32(&t, b32);
    shallue_van_de_woestijne(&add, &t);
    if (blind32) {
        rustsecp256k1zkp_v0_5_0_gej_add_ge(&accum, &accum, &add);
    } else {
        rustsecp256k1zkp_v0_5_0_gej_set_ge(&accum, &add);
    }

    rustsecp256k1zkp_v0_5_0_sha256_initialize(&sha256);
    rustsecp256k1zkp_v0_5_0_sha256_write(&sha256, prefix2, 16);
    rustsecp256k1zkp_v0_5_0_sha256_write(&sha256, key32, 32);
    rustsecp256k1zkp_v0_5_0_sha256_finalize(&sha256, b32);
    ret &= rustsecp256k1zkp_v0_5_0_fe_set_b32(&t, b32);
    shallue_van_de_woestijne(&add, &t);
    rustsecp256k1zkp_v0_5_0_gej_add_ge(&accum, &accum, &add);

    rustsecp256k1zkp_v0_5_0_ge_set_gej(&add, &accum);
    rustsecp256k1zkp_v0_5_0_generator_save(gen, &add);
    return ret;
}

int rustsecp256k1zkp_v0_5_0_generator_generate(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_generator* gen, const unsigned char *key32) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(key32 != NULL);
    return rustsecp256k1zkp_v0_5_0_generator_generate_internal(ctx, gen, key32, NULL);
}

int rustsecp256k1zkp_v0_5_0_generator_generate_blinded(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_generator* gen, const unsigned char *key32, const unsigned char *blind32) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(gen != NULL);
    ARG_CHECK(key32 != NULL);
    ARG_CHECK(blind32 != NULL);
    ARG_CHECK(rustsecp256k1zkp_v0_5_0_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    return rustsecp256k1zkp_v0_5_0_generator_generate_internal(ctx, gen, key32, blind32);
}

#endif
