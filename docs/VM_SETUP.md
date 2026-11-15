# Running the game on the VM (Ubuntu, auto-start, no login)

Goal from the spec: *when the VM starts, it immediately shows the game, without
asking for a username and password,* on a Linux system **without a GUI**.

Our setup: **Ubuntu in VirtualBox** (Windows host), switched to **text mode** (no
desktop). On boot, the game appears on the first screen (tty1) with no login.

## Why Ubuntu instead of Alpine

The spec gives Alpine Linux as an example of a small Linux system without a GUI.
We already had Ubuntu running in VirtualBox, so we used it in text-only mode
instead, which also meets the "no GUI" requirement.

Our auto-start script was first written for Alpine, which starts programs from
the file `/etc/inittab`. When we ran it on Ubuntu, it failed with
`cp: cannot stat '/etc/inittab': No such file or directory`, because Ubuntu uses
systemd instead. We changed the script to detect which system it is running on:
on Ubuntu it adds a systemd drop-in for `getty@tty1`, and on Alpine it still edits
`/etc/inittab`. It now works on both.

## 1. Install the tools

```sh
sudo apt update
sudo apt install -y build-essential gdb valgrind git
```

## 2. Get the code, build and test

```sh
git clone https://github.com/Malek177/DotsAndBoxes.git
cd DotsAndBoxes
make test          # unit tests + full scripted games
make valgrind      # must end with "Valgrind: no errors, no leaks."
```

## 3. Boot in text mode (no GUI)

```sh
sudo systemctl set-default multi-user.target
```

`multi-user.target` = boot to text screens only; the desktop never starts.
(To get the desktop back: `sudo systemctl set-default graphical.target`.)

## 4. Start the game on boot

```sh
sudo sh scripts/install_autostart.sh
sudo reboot
```

How it works: Ubuntu's startup system (**systemd**) runs a login program on
each text screen; the one on screen 1 is the service `getty@tty1`. The script
installs the game to `/usr/local/bin` and adds a small "drop-in" file that
replaces only that service's command:

```ini
# /etc/systemd/system/getty@tty1.service.d/dotsandboxes.conf
[Service]
ExecStart=
ExecStart=-/usr/local/bin/dotsandboxes
StandardInput=tty
StandardOutput=tty
Restart=always
```

The empty `ExecStart=` clears the original login command, and `Restart=always`
starts a new game whenever the game exits. Screens 2–6 keep their normal login.

## 5. Using the VM afterwards

| Need | How |
|---|---|
| A normal login (to edit code, use Git, run GDB/Valgrind) | **Right Ctrl + F2** in VirtualBox. **Right Ctrl + F1** goes back to the game. |
| Install a new version of the game | `git pull && make test && sudo make install && sudo systemctl restart getty@tty1` |
| Undo the auto-start | `sudo sh scripts/install_autostart.sh --undo && sudo reboot` |
| Shut down | `sudo poweroff` (do **not** use "Save the machine state", see below) |

## Problems we hit and how we fixed them

| Problem | Cause | Fix |
|---|---|---|
| VirtualBox itself crashed ("memory could not be written") when the VM switched to text mode | VirtualBox 3D graphics support | Settings → Display → untick **Enable 3D Acceleration**, Graphics Controller **VMSVGA** |
| Boot took several minutes and stayed on early startup messages | VirtualBox running on top of Windows Hyper-V (turtle icon in the status bar) | Wait it out, or turn off Hyper-V / Memory integrity on Windows |
| After the desktop's automatic login stopped working, we did not know the password | The VM was created with VirtualBox's automatic install (user `vboxuser`) | Boot menu (hold Shift) → Advanced options → recovery mode → root shell → `mount -o remount,rw /` → `passwd vboxuser` |
| Keyboard stopped working and Caps Lock blinked after restoring a saved state | Kernel panic in the VM after resuming | Machine → Reset; since then we always shut down with `sudo poweroff` |

## Alpine Linux (alternative)

The same script also works on Alpine Linux, which has no GUI to begin with.
There it replaces the `tty1` line in `/etc/inittab` (BusyBox init) instead of
using systemd. Install the tools with `apk add build-base gdb valgrind git`,
then follow steps 2 and 4 (run as root, without `sudo`).
