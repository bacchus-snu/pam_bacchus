#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <syslog.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>
#include <curl/curl.h>

#include "tweetnacl.h"
#include "utils.h"

#define PAM_ITEM_SECRET_KEY "PAM_BACCHUS_SECRET_KEY"
#define PUBLIC_KEY_BASE64_SIZE (((crypto_sign_PUBLICKEYBYTES + 2) / 3) * 4)
#define SIGNATURE_BASE64_SIZE (((crypto_sign_BYTES + 2) / 3) * 4)

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
    params->secret_key_path = "/etc/bacchus/keypair/tweetnacl";

    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "url=", 4) == 0) {
            params->login_endpoint = argv[i] + 4;
        } else if (strncmp(argv[i], "key=", 4) == 0) {
            params->secret_key_path = argv[i] + 4;
        } else if (strcmp(argv[i], "publickey_only") == 0) {
            params->publickey_only = 1;
        }
    }

    if (params->login_endpoint == NULL) {
        syslog(LOG_ERR, "Login endpoint not set");
        return PAM_AUTHINFO_UNAVAIL;
    }

    syslog(LOG_INFO, "Using %s as secret key", params->secret_key_path);
    return 0;
}

static void cleanup_secret_key(pam_handle_t *pamh, void *data, int error_status) {
    if (data == NULL) return;

    memset(data, 0, crypto_sign_SECRETKEYBYTES);
    free(data);
}

static int load_keypair(pam_handle_t *pamh, const char *key_path) {
    unsigned char *secret_key = malloc(crypto_sign_SECRETKEYBYTES);

    int fd_key = open(key_path, O_RDONLY);
    if (fd_key == -1) {
        syslog(LOG_ERR, "Failed to open secret key: %s", strerror(errno));
        cleanup_secret_key(pamh, (void *) secret_key, PAM_SILENT);
        return PAM_AUTHINFO_UNAVAIL;
    }

    ssize_t key_size = read_exact(fd_key, secret_key, crypto_sign_SECRETKEYBYTES);
    if (key_size != crypto_sign_SECRETKEYBYTES) {
        syslog(
            LOG_ERR,
            "Failed to read secret key: %s",
            key_size == -1 ? strerror(errno) : "key size mismatch"
        );
        cleanup_secret_key(pamh, (void *) secret_key, PAM_SILENT);
        close(fd_key);
        return PAM_AUTHINFO_UNAVAIL;
    }
    close(fd_key);

    syslog(LOG_INFO, "Keypair loaded");
    pam_set_data(pamh, PAM_ITEM_SECRET_KEY, secret_key, cleanup_secret_key);
    return 0;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int pam_ret;
    const char *username = NULL;
    const char *password = NULL;

    openlog("pam_bacchus", 0, LOG_AUTH);

    params_t params;
    pam_ret = parse_param(argc, argv, &params);
    if (pam_ret != 0) {
        return pam_ret;
    }

    int key_available = 1;
    pam_ret = load_keypair(pamh, params.secret_key_path);
    if (pam_ret != 0) {
        if (params.publickey_only) {
            syslog(LOG_ERR, "Public key auth enforced, aborting");
            return pam_ret;
        }
        syslog(LOG_WARNING, "Falling back to IP address auth");
        key_available = 0;
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

    const char *format = "{\"username\": \"%s\", \"password\": \"%s\"}";
    char *escaped_username = escape_json_string(username);
    char *escaped_password = escape_json_string(password);

    size_t post_body_len = strlen(format) + strlen(escaped_username) + strlen(escaped_password) - 4;
    char *post_body = (char *) malloc(sizeof(char) * (post_body_len + 1));

    sprintf(post_body, format, escaped_username, escaped_password);
    free(escaped_username);
    free(escaped_password);

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

    char pubkey_base64[21 + PUBLIC_KEY_BASE64_SIZE + 1];
    char timestamp_str[50];
    char signature_base64[24 + SIGNATURE_BASE64_SIZE + 1];
    if (key_available) {
        const unsigned char *secret_key;
        pam_get_data(pamh, PAM_ITEM_SECRET_KEY, (const void **)&secret_key);
        const unsigned char *public_key = &secret_key[crypto_sign_SECRETKEYBYTES - crypto_sign_PUBLICKEYBYTES];

        // pubkey
        strcpy(pubkey_base64, "X-Bacchus-Id-Pubkey: ");
        base64_enc(pubkey_base64 + 21, PUBLIC_KEY_BASE64_SIZE + 1, public_key, crypto_sign_PUBLICKEYBYTES);
        headers = curl_slist_append(headers, pubkey_base64);

        // timestamp
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        sprintf(timestamp_str, "X-Bacchus-Id-Timestamp: %ld", tp.tv_sec);
        headers = curl_slist_append(headers, timestamp_str);

        // signature
        unsigned long long msg_len = (strlen(timestamp_str) - 24) + post_body_len;
        unsigned char *signed_msg = malloc(sizeof(unsigned char) * (crypto_sign_BYTES + msg_len + 1));
        memset(signed_msg, 0, sizeof(unsigned char) * (crypto_sign_BYTES + msg_len + 1));
        memcpy(&signed_msg[crypto_sign_BYTES], &timestamp_str[24], strlen(timestamp_str) - 24);
        memcpy(&signed_msg[crypto_sign_BYTES + strlen(timestamp_str) - 24], post_body, post_body_len);
        crypto_sign(signed_msg, &msg_len, &signed_msg[crypto_sign_BYTES], msg_len, secret_key);

        strcpy(signature_base64, "X-Bacchus-Id-Signature: ");
        base64_enc(signature_base64 + 24, SIGNATURE_BASE64_SIZE + 1, signed_msg, crypto_sign_BYTES);
        headers = curl_slist_append(headers, signature_base64);
        free(signed_msg);
    }

    // headers done
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // synchronous http call
    curl_code = curl_easy_perform(curl);

    int ret = PAM_AUTH_ERR;

    if (curl_code == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
            syslog(
                LOG_WARNING,
                "Authentication failed for user %s: status code %ld",
                username, response_code
            );
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
