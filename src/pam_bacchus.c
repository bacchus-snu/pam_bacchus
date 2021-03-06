#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>
#include <curl/curl.h>

#include "utils.h"

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

static int parse_param(int argc, const char **argv, params_t *params) {
    memset(params, 0, sizeof(params_t));

    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "url=", 4) == 0) {
            params->login_endpoint = argv[i] + 4;
        }
    }

    if (params->login_endpoint == NULL) {
        return PAM_AUTHINFO_UNAVAIL;
    }

    return 0;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int pam_ret;
    const char *username = NULL;
    const char *password = NULL;

    params_t params;
    pam_ret = parse_param(argc, argv, &params);
    if (pam_ret != 0) {
        return pam_ret;
    }

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
        // if conversation function was interrupted, this will be NULL
        if (password == NULL) {
            // set password to null authtok
            password = "";
        }
        // set authtok to pass our authentication token to other modules
        // for pam_unix.so, you probably have to use try_first_pass or use_first_pass options
        pam_ret = pam_set_item(pamh, PAM_AUTHTOK, (const void *) password);
        if (pam_ret != PAM_SUCCESS) {
            // this can be ignored, but for now, we will make authentication failed
            return PAM_AUTH_ERR;
        }
    }

    if (flags & PAM_DISALLOW_NULL_AUTHTOK) {
        if (strcmp(password, "") == 0) {
            // disallow null authentication token
            return PAM_AUTH_ERR;
        }
    }

    const char *URL = params.login_endpoint;
    CURL *curl;
    CURLcode curl_code;
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        curl_global_cleanup();
        return PAM_AUTH_ERR;
    }

    curl_easy_setopt(curl, CURLOPT_URL, URL);

    headers = curl_slist_append(headers, "Accept: applicaton/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    const char *format = "{\"username\": \"%s\", \"password\": \"%s\"}";
    char *escaped_username = escape_json_string(username);
    char *escaped_password = escape_json_string(password);

    char *post_body = (char *) malloc(sizeof(char) * strlen(format)
            + strlen(escaped_username) + strlen(escaped_password) - 4 + 1);

    sprintf(post_body, format, escaped_username, escaped_password);
    free(escaped_username);
    free(escaped_password);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

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

    if (pam_conv_data && pam_resp && pam_resp[0].resp) {
        memset(pam_resp[0].resp, '\0', strlen(pam_resp[0].resp));
        free(pam_resp);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return ret;
}
