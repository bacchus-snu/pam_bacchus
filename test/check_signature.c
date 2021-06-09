#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include "../src/utils.h"
#include "../src/tweetnacl.h"

void randombytes(unsigned char *buf, unsigned long long n) {
    memset(buf, 0, n);
}

static ssize_t read_to_end(int fd, void **pbuf) {
    size_t cap = 64;
    size_t len = 0;
    void *buf = malloc(cap);
    while (1) {
        ssize_t count = read_exact(fd, buf + len, cap - len);
        if (count < 0) {
            int t = errno;
            free(buf);
            errno = t;
            return count;
        }
        len += count;
        if (count < cap) {
            break;
        }

        cap *= 2;
        buf = realloc(buf, cap);
    }
    *pbuf = buf;
    return len;
}

int main() {
    if (isatty(STDIN_FILENO)) {
        fprintf(stderr, "Input public key, signature, and message through stdin in binary format.\n");
        return 1;
    }

    unsigned char pubkey[crypto_sign_PUBLICKEYBYTES];
    if (read_exact(STDIN_FILENO, pubkey, crypto_sign_PUBLICKEYBYTES) < 0) {
        perror("Failed to read public key");
        return 1;
    }

    unsigned char *signed_msg;
    ssize_t msglen = read_to_end(STDIN_FILENO, (void **) &signed_msg);
    if (msglen < 0) {
        perror("Failed to read signature and message");
        free(signed_msg);
        return 1;
    }

    unsigned char *dummy = malloc(msglen);
    unsigned long long n;
    if (crypto_sign_open(dummy, &n, signed_msg, msglen, pubkey) != 0) {
        fprintf(stderr, "Failed to verify signature");
        free(dummy);
        free(signed_msg);
        return 1;
    }

    free(dummy);
    free(signed_msg);
    return 0;
}
