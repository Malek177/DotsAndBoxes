#!/bin/sh
# install_autostart.sh - Make Dots and Boxes start automatically on boot,
# with NO username/password prompt. Works on:
#   - Alpine Linux (BusyBox init, /etc/inittab)
#   - Ubuntu / Debian server (systemd)
#
# Run as root from the project folder:
#   Alpine:  sh scripts/install_autostart.sh
#   Ubuntu:  sudo sh scripts/install_autostart.sh
# Undo:      add --undo
#
# Idea (same on both systems): the first console (tty1) normally runs a
# login prompt ("getty"). We run the game there instead, and the system
# restarts it whenever it exits (after "Play again? n").
# tty2 keeps its normal login: press Alt+F2 to log in and maintain the VM.

set -e

GAME=/usr/local/bin/dotsandboxes

if [ "$(id -u)" -ne 0 ]; then
  echo "Please run as root (Ubuntu: sudo sh $0)." >&2
  exit 1
fi

# ---- systemd (Ubuntu, Debian) ------------------------------------------
# A "drop-in" file overrides only the command of the tty1 login service
# (getty@tty1.service). The original service file is never touched.
DROPIN_DIR=/etc/systemd/system/getty@tty1.service.d
DROPIN=$DROPIN_DIR/dotsandboxes.conf

install_systemd() {
  mkdir -p "$DROPIN_DIR"
  cat > "$DROPIN" <<EOF
# Run Dots and Boxes on tty1 instead of the login prompt.
[Service]
# An empty ExecStart= clears the original agetty command first.
ExecStart=
ExecStart=-$GAME
StandardInput=tty
StandardOutput=tty
Restart=always
EOF
  systemctl daemon-reload
  echo "Created $DROPIN"
}

undo_systemd() {
  rm -f "$DROPIN"
  rmdir "$DROPIN_DIR" 2>/dev/null || true
  systemctl daemon-reload
  echo "Removed $DROPIN. Reboot to get the login prompt back."
}

# ---- BusyBox init (Alpine) ---------------------------------------------
INITTAB=/etc/inittab
BACKUP=/etc/inittab.before-dotsandboxes

install_inittab() {
  # Back up inittab once, so --undo always returns to the original.
  if [ ! -f "$BACKUP" ]; then
    cp "$INITTAB" "$BACKUP"
    echo "Backed up $INITTAB to $BACKUP"
  fi
  # Replace the tty1 entry (whatever it currently runs) with the game.
  if grep -q '^tty1::' "$INITTAB"; then
    sed -i "s|^tty1::.*|tty1::respawn:$GAME|" "$INITTAB"
  else
    echo "tty1::respawn:$GAME" >> "$INITTAB"
  fi
  echo "tty1 entry is now:"
  grep '^tty1::' "$INITTAB"
}

undo_inittab() {
  if [ -f "$BACKUP" ]; then
    cp "$BACKUP" "$INITTAB"
    echo "Restored original $INITTAB. Reboot to get the login prompt back."
  else
    echo "No backup found at $BACKUP; nothing to undo." >&2
  fi
}

# ---- main ----------------------------------------------------------------
# /run/systemd/system exists only when the machine was booted by systemd.
if [ -d /run/systemd/system ]; then
  MODE=systemd
elif [ -f "$INITTAB" ]; then
  MODE=inittab
else
  echo "Unsupported init system (neither systemd nor /etc/inittab)." >&2
  exit 1
fi
echo "Detected init system: $MODE"

if [ "$1" = "--undo" ]; then
  "undo_$MODE"
  exit 0
fi

# Build and install the game first.
make
make install
echo "Installed $GAME"

"install_$MODE"

echo
echo "Done. Reboot with:  reboot"
echo "The game will appear on boot. Alt+F2 = normal login on tty2."
