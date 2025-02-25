/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_TESTRAND_H
#define SECP256K1_TESTRAND_H

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

/* A non-cryptographic RNG used only for test infrastructure. */

/** Seed the pseudorandom number generator for testing. */
SECP256K1_INLINE static void rustsecp256k1zkp_v0_5_0_testrand_seed(const unsigned char *seed16);

/** Generate a pseudorandom number in the range [0..2**32-1]. */
static uint32_t rustsecp256k1zkp_v0_5_0_testrand32(void);

/** Generate a pseudorandom number in the range [0..2**bits-1]. Bits must be 1 or
 *  more. */
static uint32_t rustsecp256k1zkp_v0_5_0_testrand_bits(int bits);

/** Generate a pseudorandom number in the range [0..range-1]. */
static uint32_t rustsecp256k1zkp_v0_5_0_testrand_int(uint32_t range);

/** Generate a pseudorandom 32-byte array. */
static void rustsecp256k1zkp_v0_5_0_testrand256(unsigned char *b32);

/** Generate a pseudorandom 32-byte array with long sequences of zero and one bits. */
static void rustsecp256k1zkp_v0_5_0_testrand256_test(unsigned char *b32);

/** Generate pseudorandom bytes with long sequences of zero and one bits. */
static void rustsecp256k1zkp_v0_5_0_testrand_bytes_test(unsigned char *bytes, size_t len);

/** Generate a pseudorandom 64-bit integer in the range min..max, inclusive. */
static int64_t rustsecp256k1zkp_v0_5_0_testrandi64(uint64_t min, uint64_t max);

/** Flip a single random bit in a byte array */
static void rustsecp256k1zkp_v0_5_0_testrand_flip(unsigned char *b, size_t len);

/** Initialize the test RNG using (hex encoded) array up to 16 bytes, or randomly if hexseed is NULL. */
static void rustsecp256k1zkp_v0_5_0_testrand_init(const char* hexseed);

/** Print final test information. */
static void rustsecp256k1zkp_v0_5_0_testrand_finish(void);

#endif /* SECP256K1_TESTRAND_H */
