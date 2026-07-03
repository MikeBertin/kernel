// kernel/string.h — the handful of freestanding memory helpers we need.
// (gcc may also emit implicit calls to memset/memcpy, so these must exist.)
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int   memcmp(const void *a, const void *b, size_t n);

#endif
