#ifndef __UTILS_H
#define __UTILS_H

#include <sys/types.h>

char *escape_json_string(const char *original);
ssize_t read_exact(int fd, void *buf, size_t len);
int base64_enc(char *out, size_t outsize, const unsigned char *in, const size_t insize);

typedef struct {
    const char *login_endpoint;
    const char *secret_key_path;
} params_t;

#endif
