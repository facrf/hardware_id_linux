/*
 * sha256.h — Implementação pura e portável de SHA-256 em C11
 *
 * Não depende de bibliotecas externas (como OpenSSL).
 * Usado para cálculo de Hardware Fingerprints / Machine IDs únicos.
 */

#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t  data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[32]);

/* Função utilitária de alto nível: calcula o hash em formato hexadecimal (64 chars + null) */
void sha256_string(const char *input, char output_hex[65]);

#endif /* SHA256_H */
