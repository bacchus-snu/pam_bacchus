#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include <string.h>

static struct pam_conv conv = {
    misc_conv,
    NULL
};

int main(int argc, char **argv) {
    pam_handle_t *pamh = NULL;
    int pam_ret;

    if (argc != 3) {
        fprintf(stderr, "Usage: ./id_test <username> <expect>\n");
        return 1;
    }

    const char *username = argv[1];
    int expect;

    if (strcmp("true", argv[2]) == 0) {
        expect = 1;
    } else if (strcmp("false", argv[2]) == 0) {
        expect = 0;
    } else {
        fprintf(stderr, "Usage: ./id_test <username> <expect>\n");
        return 1;
    }

    // start pam session
    pam_ret = pam_start("id_test", username, &conv, &pamh);
    if (pam_ret != PAM_SUCCESS) {
        fprintf(stderr, "pam_start failed\n");
        return 1;
    }

    fprintf(stdout, "starting pam_authenticate\n");
    pam_ret = pam_authenticate(pamh, 0);
    int auth_success;
    if (pam_ret == PAM_SUCCESS) {
        fprintf(stdout, "auth success\n");
        auth_success = 1;
    } else {
        fprintf(stdout, "auth failed\n");
        auth_success = 0;
    }

    pam_ret = pam_end(pamh, pam_ret);
    if (pam_ret != PAM_SUCCESS) {
        fprintf(stderr, "pam_end failed\n");
        return 1;
    }

    if (auth_success == expect) {
        return 0;
    } else {
        return 1;
    }
}
