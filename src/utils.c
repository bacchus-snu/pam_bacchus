#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

char *escape_json_string(const char *original) {
    int current_index;
    int len = strlen(original);
    int new_len = len;

    for (current_index = 0; current_index < len; current_index++) {
        char c = original[current_index];
        switch (c) {
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '"':
            case '\\':
                new_len++;
        }
    }

    int new_index = 0;
    char *result = (char *) malloc(sizeof(char) * (new_len + 1));
    for (current_index = 0; current_index < len; current_index++) {
        char c = original[current_index];
        switch (c) {
            case '\b':
                result[new_index++] = '\\';
                result[new_index++] = 'b';
                break;
            case '\f':
                result[new_index++] = '\\';
                result[new_index++] = 'f';
                break;
            case '\n':
                result[new_index++] = '\\';
                result[new_index++] = 'n';
                break;
            case '\r':
                result[new_index++] = '\\';
                result[new_index++] = 'r';
                break;
            case '\t':
                result[new_index++] = '\\';
                result[new_index++] = 't';
                break;
            case '"':
                result[new_index++] = '\\';
                result[new_index++] = '"';
                break;
            case '\\':
                result[new_index++] = '\\';
                result[new_index++] = '\\';
                break;
            default:
                result[new_index++] = c;
        }
    }
    result[new_len] = '\0';

    return result;
}

ssize_t read_exact(int fd, void *buf, size_t len) {
    size_t count = 0;
    while (count < len) {
        ssize_t read_count = read(fd, buf + count, len - count);
        if (read_count < 0) {
          if (read_count == EINTR) continue;
          return read_count;
        }
        if (read_count == 0) break;
        count += read_count;
    }
    return count;
}

int base64_enc(char *out, size_t outsize, const unsigned char *in, const size_t insize) {
    static const char table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    if (outsize <= ((insize + 2) / 3) * 4) {
        return -1;
    }

    size_t outcount = 0;
    for (size_t count = 0; count < insize; count += 3) {
        size_t len = insize - count;
        if (len > 3) len = 3;

        unsigned int value = 0;
        int loopcnt = len + 1;
        if (len == 1) {
            value = in[count] << 16;
        } else if (len == 2) {
            value = in[count] << 16 | in[count+1] << 8;
        } else if (len == 3) {
            value = in[count] << 16 | in[count+1] << 8 | in[count+2];
        }

        unsigned int shift = 18;
        for (int i = 0; i < loopcnt; i++) {
            out[outcount] = table[(value & (0x3f << shift)) >> shift];
            shift -= 6;
            outcount++;
        }
        for (int i = loopcnt; i < 4; i++) {
            out[outcount] = '=';
            outcount++;
        }
    }
    out[outcount] = 0;
    return 0;
}
