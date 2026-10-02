#ifndef BATTLEHUD_SHA1_H
#define BATTLEHUD_SHA1_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct sha1_context { uint32_t h[5]; uint64_t bytes; unsigned char block[64]; size_t used; };
void sha1_init(struct sha1_context *ctx);
void sha1_update(struct sha1_context *ctx, const void *data, size_t size);
void sha1_final(struct sha1_context *ctx, char hex[41]);
bool sha1_file(const char *path, char hex[41]);
#endif
