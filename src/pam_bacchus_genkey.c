#include <stdio.h>
#include <errno.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "tweetnacl.h"

static ssize_t write_exact(int fd, const void *buf, size_t len) {
    size_t count = 0;
    while (count < len) {
        ssize_t written = write(fd, buf + count, len - count);
        if (written < 0) {
            if (errno == EINTR) continue;
            return written;
        }
        if (written == 0) break;
        count += written;
    }
    return count;
}

void randombytes(unsigned char *buf, unsigned long long n) {
    size_t count = 0;
    while (count < n) {
        size_t read = getrandom(&buf[count], n - count, 0);
        if (read == -1) {
            if (errno == EINTR) continue;
            break;
        }
        count += read;
    }
}

int main(int argc, char **argv) {
    if (argc > 2) {
        return 1;
    }

    const char *keydir = "/etc/bacchus/keypair";
    if (argc == 2) {
        keydir = argv[1];
    }

    int fd_dir = open(keydir, O_DIRECTORY | O_RDONLY);
    if (fd_dir == -1) {
        perror("Cannot open key directory");
        return 1;
    }

    fprintf(stderr, "Generating keypair...");
    fflush(stderr);
    unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
    unsigned char secret_key[crypto_sign_SECRETKEYBYTES];
    crypto_sign_keypair(public_key, secret_key);
    fprintf(stderr, " done.\n");

    fprintf(stderr, "Writing keypair...");
    fflush(stderr);

    int fd_key = openat(fd_dir, "tweetnacl.pub", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (write_exact(fd_key, public_key, crypto_sign_PUBLICKEYBYTES) < 0) {
        perror(" Cannot write public key");
        return 1;
    }
    close(fd_key);

    fd_key = openat(fd_dir, "tweetnacl", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (write_exact(fd_key, secret_key, crypto_sign_SECRETKEYBYTES) < 0) {
        perror(" Cannot write secret key");
        return 1;
    }
    close(fd_key);

    fprintf(stderr, " done.\n");
    return 0;
}
