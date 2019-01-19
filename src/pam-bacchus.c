#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>
#include <curl/curl.h>

struct response_data {
    char *data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    struct response_data *current_data = (struct response_data *) userdata;

    char *reallocated_ptr = (char *) realloc(current_data->data, current_data->size + realsize + 1);
    if (reallocated_ptr == NULL) {
        fprintf(stderr, "Not enough memory\n");
        return 0;
    }

    current_data->data = reallocated_ptr;
    memcpy(&(current_data->data[current_data->size]), contents, realsize);
    current_data->size += realsize;
    current_data->data[current_data->size] = '\0';

    return realsize;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *URL = "https://id.snucse.org/api/login";
    CURL *curl;
    CURLcode curl_code;
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        curl_global_cleanup();
        return PAM_AUTH_ERR;
    }

    struct response_data userdata;
    userdata.data = (char *) malloc(1);
    userdata.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, URL);

    headers = curl_slist_append(headers, "Accept: applicaton/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"name\": \"test\", \"password\": \"doge\"}");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &userdata);

    curl_code = curl_easy_perform(curl);

    int ret = PAM_AUTH_ERR;

    if (curl_code == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
            ret = PAM_AUTH_ERR;
        } else {
            ret = PAM_SUCCESS;
        }
    }

    free(userdata.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return ret;
}
