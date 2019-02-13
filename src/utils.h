#ifndef __UTILS_H
#define __UTILS_H

char *escape_json_string(const char *original);

typedef struct {
    const char *login_endpoint;
} params_t;

#endif
