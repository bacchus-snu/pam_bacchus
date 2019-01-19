#include <stdio.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>
#include <curl/curl.h>


PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
