#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>
#include <curl/curl.h>

#define ID_LOGIN_ENDPOINT "https://id.snucse.org/api/login"

struct response_data {
    char *data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    /*
    struct response_data *current_data = (struct response_data *) userdata;

    char *reallocated_ptr = (char *) realloc(current_data->data, current_data->size + realsize + 1);
    if (reallocated_ptr == NULL) {
        // error, not enough memory
        return 0;
    }

    current_data->data = reallocated_ptr;
    memcpy(&(current_data->data[current_data->size]), contents, realsize);
    current_data->size += realsize;
    current_data->data[current_data->size] = '\0';
    */
    return realsize;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int pam_ret;
    const char *username = NULL;
    const char *password = NULL;

    struct pam_conv *pam_conv_data = NULL;
    struct pam_message msg;
    const struct pam_message *pam_msg = &msg;
    struct pam_response *pam_resp = NULL;

    pam_ret = pam_get_item(pamh, PAM_USER, (const void **) &username);
    if (pam_ret != PAM_SUCCESS || !username) {
        // cannot get username
        return PAM_AUTH_ERR;
    }

    pam_ret = pam_get_item(pamh, PAM_AUTHTOK, (const void **) &password);
    if (pam_ret != PAM_SUCCESS || !password) {
        // get pam_conv object
        pam_ret = pam_get_item(pamh, PAM_CONV, (const void **) &pam_conv_data);
        if (pam_ret != PAM_SUCCESS || !pam_conv_data) {
            // cannot get pam_conv object
            return PAM_AUTH_ERR;
        }

        const char *prompt = "Password: ";
        msg.msg = prompt;
        msg.msg_style = PAM_PROMPT_ECHO_OFF;

        pam_conv_data->conv(1, &pam_msg, &pam_resp, pam_conv_data->appdata_ptr);
        password = (const char *) pam_resp[0].resp;
    }

    if (flags & PAM_DISALLOW_NULL_AUTHTOK) {
        if (strcmp(password, "") == 0) {
            // disallow null authentication token
            return PAM_AUTH_ERR;
        }
    }

    const char *URL = ID_LOGIN_ENDPOINT;
    CURL *curl;
    CURLcode curl_code;
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        curl_global_cleanup();
        return PAM_AUTH_ERR;
    }

    /*
    struct response_data userdata;
    // preallocate for a null character
    userdata.data = (char *) malloc(1);
    userdata.size = 0;
    */

    curl_easy_setopt(curl, CURLOPT_URL, URL);

    headers = curl_slist_append(headers, "Accept: applicaton/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    const char *format = "{\"username\": \"%s\", \"password\": \"%s\"}";
    // TODO: escape json string
    char *post_body = (char *) malloc(sizeof(char) * strlen(format)
            + strlen(username) + strlen(password) - 4 + 1);
    sprintf(post_body, format, username, password);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &userdata);

    // synchronous http call
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

    // free(userdata.data);
    if (pam_conv_data) {
        memset(pam_resp[0].resp, '\0', strlen(pam_resp[0].resp));
        free(pam_resp);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return ret;
}
