Installer Script Capabilities

Detects distro (Rocky or Ubuntu)
Installs logger script to /usr/local/bin/pam_auth_logger.sh
Modifies appropriate PAM config files
Creates backups before touching anything

Each auth attempt generates:
2026-04-03 14:22:11 | user=root | ruser= | rhost=192.168.1.50 | tty=ssh | service=sshd


Test each vector:

ssh user@host
sudo whoami
su -
login (TTY)

Then:

tail -f /var/log/pam_auth.log