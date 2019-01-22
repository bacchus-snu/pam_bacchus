#include <stdlib.h>
#include <string.h>

const char *escape_json_string(const char *original) {
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
