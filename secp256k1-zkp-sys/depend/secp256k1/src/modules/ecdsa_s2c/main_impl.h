/**********************************************************************
 * Copyright (c) 2019-2020 Marko Bencun, Jonas Nick                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_ECDSA_S2C_MAIN_H
#define SECP256K1_MODULE_ECDSA_S2C_MAIN_H

#include "include/secp256k1.h"
#include "include/secp256k1_ecdsa_s2c.h"

static void rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening_save(rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* opening, rustsecp256k1zkp_v0_5_0_ge* ge) {
    rustsecp256k1zkp_v0_5_0_pubkey_save((rustsecp256k1zkp_v0_5_0_pubkey*) opening, ge);
}

static int rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening_load(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_ge* ge, const rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* opening) {
    return rustsecp256k1zkp_v0_5_0_pubkey_load(ctx, ge, (const rustsecp256k1zkp_v0_5_0_pubkey*) opening);
}

int rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening_parse(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* opening, const unsigned char* input33) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(opening != NULL);
    ARG_CHECK(input33 != NULL);
    return rustsecp256k1zkp_v0_5_0_ec_pubkey_parse(ctx, (rustsecp256k1zkp_v0_5_0_pubkey*) opening, input33, 33);
}

int rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening_serialize(const rustsecp256k1zkp_v0_5_0_context* ctx, unsigned char* output33, const rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* opening) {
    size_t out_len = 33;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output33 != NULL);
    ARG_CHECK(opening != NULL);
    return rustsecp256k1zkp_v0_5_0_ec_pubkey_serialize(ctx, output33, &out_len, (const rustsecp256k1zkp_v0_5_0_pubkey*) opening, SECP256K1_EC_COMPRESSED);
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("s2c/ecdsa/point")||SHA256("s2c/ecdsa/point"). */
static void rustsecp256k1zkp_v0_5_0_s2c_ecdsa_point_sha256_tagged(rustsecp256k1zkp_v0_5_0_sha256 *sha) {
    rustsecp256k1zkp_v0_5_0_sha256_initialize(sha);
    sha->s[0] = 0xa9b21c7bul;
    sha->s[1] = 0x358c3e3eul;
    sha->s[2] = 0x0b6863d1ul;
    sha->s[3] = 0xc62b2035ul;
    sha->s[4] = 0xb44b40ceul;
    sha->s[5] = 0x254a8912ul;
    sha->s[6] = 0x0f85d0d4ul;
    sha->s[7] = 0x8a5bf91cul;

    sha->bytes = 64;
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("s2c/ecdsa/data")||SHA256("s2c/ecdsa/data"). */
static void rustsecp256k1zkp_v0_5_0_s2c_ecdsa_data_sha256_tagged(rustsecp256k1zkp_v0_5_0_sha256 *sha) {
    rustsecp256k1zkp_v0_5_0_sha256_initialize(sha);
    sha->s[0] = 0xfeefd675ul;
    sha->s[1] = 0x73166c99ul;
    sha->s[2] = 0xe2309cb8ul;
    sha->s[3] = 0x6d458113ul;
    sha->s[4] = 0x01d3a512ul;
    sha->s[5] = 0x00e18112ul;
    sha->s[6] = 0x37ee0874ul;
    sha->s[7] = 0x421fc55ful;

    sha->bytes = 64;
}

int rustsecp256k1zkp_v0_5_0_ecdsa_s2c_sign(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_ecdsa_signature* signature, rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* s2c_opening, const unsigned char
 *msg32, const unsigned char *seckey, const unsigned char* s2c_data32) {
    rustsecp256k1zkp_v0_5_0_scalar r, s;
    int ret;
    unsigned char ndata[32];
    rustsecp256k1zkp_v0_5_0_sha256 s2c_sha;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(rustsecp256k1zkp_v0_5_0_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(signature != NULL);
    ARG_CHECK(seckey != NULL);
    ARG_CHECK(s2c_data32 != NULL);

    /* Provide `s2c_data32` to the nonce function as additional data to
     * derive the nonce. It is first hashed because it should be possible
     * to derive nonces even if only a SHA256 commitment to the data is
     * known.  This is important in the ECDSA anti-exfil protocol. */
    rustsecp256k1zkp_v0_5_0_s2c_ecdsa_data_sha256_tagged(&s2c_sha);
    rustsecp256k1zkp_v0_5_0_sha256_write(&s2c_sha, s2c_data32, 32);
    rustsecp256k1zkp_v0_5_0_sha256_finalize(&s2c_sha, ndata);

    rustsecp256k1zkp_v0_5_0_s2c_ecdsa_point_sha256_tagged(&s2c_sha);
    ret = rustsecp256k1zkp_v0_5_0_ecdsa_sign_inner(ctx, &r, &s, NULL, &s2c_sha, s2c_opening, s2c_data32, msg32, seckey, NULL, ndata);
    rustsecp256k1zkp_v0_5_0_scalar_cmov(&r, &rustsecp256k1zkp_v0_5_0_scalar_zero, !ret);
    rustsecp256k1zkp_v0_5_0_scalar_cmov(&s, &rustsecp256k1zkp_v0_5_0_scalar_zero, !ret);
    rustsecp256k1zkp_v0_5_0_ecdsa_signature_save(signature, &r, &s);
    return ret;
}

int rustsecp256k1zkp_v0_5_0_ecdsa_s2c_verify_commit(const rustsecp256k1zkp_v0_5_0_context* ctx, const rustsecp256k1zkp_v0_5_0_ecdsa_signature* sig, const unsigned char* data32, const rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* opening) {
    rustsecp256k1zkp_v0_5_0_ge commitment_ge;
    rustsecp256k1zkp_v0_5_0_ge original_pubnonce_ge;
    unsigned char x_bytes[32];
    rustsecp256k1zkp_v0_5_0_scalar sigr, sigs, x_scalar;
    rustsecp256k1zkp_v0_5_0_sha256 s2c_sha;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(rustsecp256k1zkp_v0_5_0_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(sig != NULL);
    ARG_CHECK(data32 != NULL);
    ARG_CHECK(opening != NULL);

    if (!rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening_load(ctx, &original_pubnonce_ge, opening)) {
        return 0;
    }
    rustsecp256k1zkp_v0_5_0_s2c_ecdsa_point_sha256_tagged(&s2c_sha);
    if (!rustsecp256k1zkp_v0_5_0_ec_commit(&ctx->ecmult_ctx, &commitment_ge, &original_pubnonce_ge, &s2c_sha, data32, 32)) {
        return 0;
    }

    /* Check that sig_r == commitment_x (mod n)
     * sig_r is the x coordinate of R represented by a scalar.
     * commitment_x is the x coordinate of the commitment (field element).
     *
     * Note that we are only checking the x-coordinate -- this is because the y-coordinate
     * is not part of the ECDSA signature (and therefore not part of the commitment!)
     */
    rustsecp256k1zkp_v0_5_0_ecdsa_signature_load(ctx, &sigr, &sigs, sig);

    rustsecp256k1zkp_v0_5_0_fe_normalize(&commitment_ge.x);
    rustsecp256k1zkp_v0_5_0_fe_get_b32(x_bytes, &commitment_ge.x);
    /* Do not check overflow; overflowing a scalar does not affect whether
     * or not the R value is a cryptographic commitment, only whether it
     * is a valid R value for an ECDSA signature. If users care about that
     * they should use `ecdsa_verify` or `anti_exfil_host_verify`. In other
     * words, this check would be (at best) unnecessary, and (at worst)
     * insufficient. */
    rustsecp256k1zkp_v0_5_0_scalar_set_b32(&x_scalar, x_bytes, NULL);
    return rustsecp256k1zkp_v0_5_0_scalar_eq(&sigr, &x_scalar);
}

/*** anti-exfil ***/
int rustsecp256k1zkp_v0_5_0_ecdsa_anti_exfil_host_commit(const rustsecp256k1zkp_v0_5_0_context* ctx, unsigned char* rand_commitment32, const unsigned char* rand32) {
    rustsecp256k1zkp_v0_5_0_sha256 sha;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(rand_commitment32 != NULL);
    ARG_CHECK(rand32 != NULL);

    rustsecp256k1zkp_v0_5_0_s2c_ecdsa_data_sha256_tagged(&sha);
    rustsecp256k1zkp_v0_5_0_sha256_write(&sha, rand32, 32);
    rustsecp256k1zkp_v0_5_0_sha256_finalize(&sha, rand_commitment32);
    return 1;
}

int rustsecp256k1zkp_v0_5_0_ecdsa_anti_exfil_signer_commit(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening* opening, const unsigned char* msg32, const unsigned char* seckey32, const unsigned char* rand_commitment32) {
    unsigned char nonce32[32];
    rustsecp256k1zkp_v0_5_0_scalar k;
    rustsecp256k1zkp_v0_5_0_gej rj;
    rustsecp256k1zkp_v0_5_0_ge r;
    unsigned int count = 0;
    int is_nonce_valid = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(rustsecp256k1zkp_v0_5_0_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(opening != NULL);
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(seckey32 != NULL);
    ARG_CHECK(rand_commitment32 != NULL);

    memset(nonce32, 0, 32);
    while (!is_nonce_valid) {
        /* cast to void* removes const qualifier, but rustsecp256k1zkp_v0_5_0_nonce_function_default does not modify it */
        if (!rustsecp256k1zkp_v0_5_0_nonce_function_default(nonce32, msg32, seckey32, NULL, (void*)rand_commitment32, count)) {
            rustsecp256k1zkp_v0_5_0_callback_call(&ctx->error_callback, "(cryptographically unreachable) generated bad nonce");
        }
        is_nonce_valid = rustsecp256k1zkp_v0_5_0_scalar_set_b32_seckey(&k, nonce32);
        /* The nonce is still secret here, but it being invalid is is less likely than 1:2^255. */
        rustsecp256k1zkp_v0_5_0_declassify(ctx, &is_nonce_valid, sizeof(is_nonce_valid));
        count++;
    }

    rustsecp256k1zkp_v0_5_0_ecmult_gen(&ctx->ecmult_gen_ctx, &rj, &k);
    rustsecp256k1zkp_v0_5_0_ge_set_gej(&r, &rj);
    rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening_save(opening, &r);
    memset(nonce32, 0, 32);
    rustsecp256k1zkp_v0_5_0_scalar_clear(&k);
    return 1;
}

int rustsecp256k1zkp_v0_5_0_anti_exfil_sign(const rustsecp256k1zkp_v0_5_0_context* ctx, rustsecp256k1zkp_v0_5_0_ecdsa_signature* sig, const unsigned char* msg32, const unsigned char* seckey, const unsigned char* host_data32) {
    return rustsecp256k1zkp_v0_5_0_ecdsa_s2c_sign(ctx, sig, NULL, msg32, seckey, host_data32);
}

int rustsecp256k1zkp_v0_5_0_anti_exfil_host_verify(const rustsecp256k1zkp_v0_5_0_context* ctx, const rustsecp256k1zkp_v0_5_0_ecdsa_signature *sig, const unsigned char *msg32, const rustsecp256k1zkp_v0_5_0_pubkey *pubkey, const unsigned char *host_data32, const rustsecp256k1zkp_v0_5_0_ecdsa_s2c_opening *opening) {
    return rustsecp256k1zkp_v0_5_0_ecdsa_s2c_verify_commit(ctx, sig, host_data32, opening) &&
        rustsecp256k1zkp_v0_5_0_ecdsa_verify(ctx, sig, msg32, pubkey);
}

#endif /* SECP256K1_ECDSA_S2C_MAIN_H */
