#define _GNU_SOURCE
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define LOG_FILE "/etc/logcheck/pam.log"
#define BASELINE "/etc/security/user_baseline"

void write_log(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

int is_new_user(const char *user) {
    struct passwd *pw = getpwnam(user);
    if (!pw) return 0;

    struct stat st;
    if (stat(pw->pw_dir, &st) == 0) {
        time_t now = time(NULL);
        return (now - st.st_mtime) < 86400;
    }
    return 0;
}

int not_in_baseline(const char *user) {
    FILE *f = fopen(BASELINE, "r");
    if (!f) return 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *user, *ruser, *rhost, *service;

    pam_get_user(pamh, &user, NULL);
    pam_get_item(pamh, PAM_RUSER, (const void **)&ruser);
    pam_get_item(pamh, PAM_RHOST, (const void **)&rhost);
    pam_get_item(pamh, PAM_SERVICE, (const void **)&service);

    time_t now = time(NULL);
    char logbuf[512];

    snprintf(logbuf, sizeof(logbuf),
        "%ld | user=%s ruser=%s rhost=%s service=%s",
        now,
        user ? user : "",
        ruser ? ruser : "",
        rhost ? rhost : "",
        service ? service : ""
    );

    write_log(logbuf);

    // Detection logic
    if (user && is_new_user(user)) {
        write_log("[ALERT] New account login");
    }

    if (user && not_in_baseline(user)) {
        write_log("[ALERT] Unknown user");
    }

    if (service && strcmp(service, "sudo") == 0) {
        write_log("[PRIV ESC DETECTED]");
    }

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}