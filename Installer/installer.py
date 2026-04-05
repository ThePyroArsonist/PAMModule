#!/usr/bin/env python3

import os
import shutil
import subprocess
from datetime import datetime

LOGGER_PATH = "/usr/local/bin/pam_auth_logger.sh"
LOG_FILE = "/etc/logcheck/pam.log"

PAM_FILES = [
    "/etc/pam.d/sshd",
    "/etc/pam.d/login",
    "/etc/pam.d/sudo",
]

def require_root():
    if os.geteuid() != 0:
        print("[!] This script must be run as root.")
        exit(1)

def detect_os():
    if os.path.exists("/etc/os-release"):
        with open("/etc/os-release") as f:
            data = f.read()
            if "Ubuntu" in data:
                return "ubuntu"
            elif "Rocky" in data or "rhel" in data.lower():
                return "rocky"
    return "unknown"

def backup_file(path):
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    backup_path = f"{path}.bak.{timestamp}"
    shutil.copy2(path, backup_path)
    print(f"[+] Backup created: {backup_path}")

def write_logger_script():
    script = f"""#!/bin/bash

LOGFILE="{LOG_FILE}"

echo "$(date '+%Y-%m-%d %H:%M:%S') | user=$PAM_USER | ruser=$PAM_RUSER | rhost=$PAM_RHOST | tty=$PAM_TTY | service=$PAM_SERVICE" >> $LOGFILE
"""

    with open(LOGGER_PATH, "w") as f:
        f.write(script)

    os.chmod(LOGGER_PATH, 0o750)
    print(f"[+] Logger script written to {LOGGER_PATH}")

def secure_log_file():
    if not os.path.exists(LOG_FILE):
        open(LOG_FILE, "w").close()

    os.chmod(LOG_FILE, 0o600)
    print(f"[+] Log file secured at {LOG_FILE}")

def update_pam_file(pam_file):
    if not os.path.exists(pam_file):
        print(f"[-] Skipping missing PAM file: {pam_file}")
        return

    backup_file(pam_file)

    with open(pam_file, "r") as f:
        lines = f.readlines()

    hook_line = f"auth optional pam_exec.so seteuid {LOGGER_PATH}\n"

    # Avoid duplicate insertion
    if any(LOGGER_PATH in line for line in lines):
        print(f"[=] PAM hook already exists in {pam_file}")
        return

    # Insert near top for auth stack visibility
    for i, line in enumerate(lines):
        if line.strip().startswith("auth"):
            lines.insert(i + 1, hook_line)
            break
    else:
        lines.append(hook_line)

    with open(pam_file, "w") as f:
        f.writelines(lines)

    print(f"[+] Updated PAM file: {pam_file}")

def main():
    require_root()

    os_type = detect_os()
    print(f"[+] Detected OS: {os_type}")

    write_logger_script()
    secure_log_file()

    for pam_file in PAM_FILES:
        update_pam_file(pam_file)

    print("\n[✓] PAM logging installation complete.")
    print("[!] Test with: ssh login / sudo / console login")

if __name__ == "__main__":
    main()