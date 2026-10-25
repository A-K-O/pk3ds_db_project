#!/usr/bin/env bash
# Install devkitPro's 3DS toolchain (the 3ds-dev group) on Fedora.
#
# devkitPro only ships an installer for Debian-based distros, so this uses
# Fedora's own pacman with a private config, package database and keyring
# under /opt/devkitpro/pacman -- the same layout devkitPro's dkp-pacman uses.
#
# Fedora's default /etc/pacman.conf enables Arch Linux's [core] and [extra]
# repos and is never used here, so pacman cannot touch system packages.
#
# Safe to re-run: it skips steps that are already done and updates packages.
set -euo pipefail

DKP_KEY=BC26F752D25B92CE272E0F44F7FD5492264BB9D0 # Dave Murphy (WinterMute) <davem@devkitpro.org>
PACROOT=/opt/devkitpro/pacman
CONF=$PACROOT/etc/pacman.conf
WRAPPER=/usr/local/bin/dkp-pacman
KEY_WRAPPER=/usr/local/bin/dkp-pacman-key

if [[ $EUID -ne 0 ]]; then
	exec sudo "$0" "$@"
fi

echo "==> Installing pacman from the Fedora repos"
dnf install -y pacman

echo "==> Writing $CONF"
mkdir -p "$PACROOT/etc/pacman.d/hooks" "$PACROOT/var/lib/pacman" \
	"$PACROOT/var/cache/pacman/pkg" "$PACROOT/var/log"
cat > "$CONF" <<EOF
# devkitPro-only pacman config. Use through $WRAPPER.
[options]
DBPath            = $PACROOT/var/lib/pacman/
CacheDir          = $PACROOT/var/cache/pacman/pkg/
LogFile           = $PACROOT/var/log/pacman.log
GPGDir            = $PACROOT/etc/pacman.d/gnupg/
HookDir           = $PACROOT/etc/pacman.d/hooks/
Architecture      = auto
CheckSpace
ParallelDownloads = 5
SigLevel          = Required DatabaseOptional
LocalFileSigLevel = Optional

[dkp-libs]
Server = https://pkg.devkitpro.org/packages

[dkp-linux]
Server = https://pkg.devkitpro.org/packages/linux/\$arch/
EOF

echo "==> Installing $WRAPPER and $KEY_WRAPPER"
cat > "$WRAPPER" <<EOF
#!/bin/sh
exec /usr/bin/pacman --config $CONF "\$@"
EOF
cat > "$KEY_WRAPPER" <<EOF
#!/bin/sh
exec /usr/bin/pacman-key --config $CONF "\$@"
EOF
chmod 755 "$WRAPPER" "$KEY_WRAPPER"

# devkitpro-keyring's install script runs dkp-pacman-key if it is on PATH and
# falls back to the system pacman-key otherwise. sudo drops /usr/local/bin
# from PATH on Fedora, so put it back.
export PATH=/usr/local/bin:$PATH

echo "==> Setting up the devkitPro signing key"
if [[ ! -d $PACROOT/etc/pacman.d/gnupg ]]; then
	dkp-pacman-key --init
fi
dkp-pacman-key --keyserver hkps://keyserver.ubuntu.com --recv-keys "$DKP_KEY"
dkp-pacman-key --lsign-key "$DKP_KEY"

# The package databases are also signed by other devkitPro maintainers, whose
# keys ship in devkitpro-keyring. Install it straight from its URL first (it
# is signed with the key above), then the databases can be verified. Any
# databases synced before the keyring was in place are dropped, because pacman
# checks them even for -U.
echo "==> Installing devkitpro-keyring"
rm -f "$PACROOT"/var/lib/pacman/sync/*
dkp-pacman -U --needed --noconfirm https://pkg.devkitpro.org/devkitpro-keyring.pkg.tar.zst
dkp-pacman-key --populate devkitpro

echo "==> Installing 3ds-dev"
dkp-pacman -Syu --needed --noconfirm 3ds-dev

cat <<'EOF'

==> Done.
The 3ds-dev group installed /etc/profile.d/devkit-env.sh, which sets
DEVKITPRO, DEVKITARM and adds the tools (3dslink, tex3ds, ...) to PATH.
It is loaded at login; to use it in this terminal right away run:

    source /etc/profile.d/devkit-env.sh

Update later with:  sudo dkp-pacman -Syu
EOF
