#ifndef HRMS_LOWLEVEL_SHA256_H
#define HRMS_LOWLEVEL_SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_BLOCK_SIZE 32 // SHA256 outputs a 32 byte digest

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
} sha256_ctx_t;

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const void *data, size_t len);
void sha256_final(sha256_ctx_t *ctx, uint8_t hash[32]);

/**
 * Convenience function to hash a buffer and produce a 64-character null-terminated hex string.
 * hex_out must have space for at least 65 bytes.
 */
void sha256_hex(const void *data, size_t len, char hex_out[65]);

/**
 * Key-stretching hash function:
 * Computes 1000 rounds:
 * H_0 = SHA256(salt + password)
 * H_i = SHA256(H_{i-1} + salt)
 * Stores 64-char hex digest in hex_out (must be >= 65 bytes).
 */
void pbkdf_sha256(const char* password, size_t pwd_len,
                  const char* salt, size_t salt_len,
                  char hex_out[65]);

#ifdef __cplusplus
}
#endif

#endif // HRMS_LOWLEVEL_SHA256_H
