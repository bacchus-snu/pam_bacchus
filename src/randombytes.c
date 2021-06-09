#include <sys/random.h>
#include <errno.h>

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
