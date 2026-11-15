# Running the game on an Alpine Linux VM (auto-start, no login)

Goal from the spec: *when the Alpine VM starts, it immediately shows the game,
without asking for a username and password.*

Approach: BusyBox `init` (Alpine's init system) reads `/etc/inittab`.
We replace the login prompt on the first console (tty1) with the game.

## 1. Create the VM (VirtualBox on Windows; UTM/VMware are similar)

1. Download the **Alpine "Virtual" ISO, x86_64** from alpinelinux.org/downloads.
   (On an Apple-Silicon Mac with UTM use the **aarch64** ISO instead.)
2. VirtualBox → New → Type *Linux*, Version *Other Linux (64-bit)*,
   512 MB RAM, 1 CPU, 2 GB disk. Attach the ISO as the optical drive.
3. Boot. Log in as `root` (no password on the live ISO).

## 2. Install Alpine to the disk

```sh
setup-alpine
```
Answer the prompts (keyboard, hostname, `eth0` + `dhcp`, root password, timezone).
When asked for **mirror**, pick `f` (fastest) and **enable the community repository**
if asked. When asked for the **disk**, choose `sda` and mode **`sys`**.

Then: `poweroff`, remove the ISO from the VM's optical drive, and start the VM again.

## 3. Install the tools and get the code

```sh
apk update
apk add build-base gdb valgrind git
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>
make test          # builds and runs every test
make valgrind      # must end with "Valgrind: no errors, no leaks."
```

## 4. Make the game start on boot

```sh
sh scripts/install_autostart.sh
reboot
```

The script runs `make install` (copies the game to `/usr/local/bin/dotsandboxes`),
backs up `/etc/inittab`, and changes **only** the tty1 line:

```diff
- tty1::respawn:/sbin/getty 38400 tty1
+ tty1::respawn:/usr/local/bin/dotsandboxes
```

`respawn` means: when the game exits (player answers `n` to "Play again?"),
init starts a fresh game.

## 5. Using the VM afterwards

| Need | How |
|---|---|
| A normal shell (to debug, `git pull`, etc.) | Press **Alt+F2** (tty2 still has a login prompt). **Alt+F1** goes back to the game. |
| Install a new version of the game | On tty2: `git pull && make install && pkill dotsandboxes` (init restarts it with the new binary). |
| Undo the auto-start | `sh scripts/install_autostart.sh --undo && reboot` |
| Run GDB/Valgrind on the VM | On tty2: `make gdb` / `make valgrind`. Take screenshots for the report. |

## 6. Optional polish: hide most boot messages

The game clears the screen when it starts, so it is the first thing you see once
booting finishes. To also hide kernel messages (BIOS VM, extlinux bootloader —
the VirtualBox default): add `quiet` to `default_kernel_opts` in
`/etc/update-extlinux.conf`, then run `update-extlinux` and reboot.

## Using Ubuntu Server instead (text-only, no GUI)

The same script detects systemd and uses it instead of `/etc/inittab`:

```sh
sudo apt install build-essential gdb valgrind unzip
make test && make valgrind
sudo sh scripts/install_autostart.sh
sudo reboot
```

It creates `/etc/systemd/system/getty@tty1.service.d/dotsandboxes.conf`, which
replaces only the command of the tty1 login service with the game
(`ExecStart=` empty line first clears the original `agetty` command).
`Restart=always` restarts the game when it exits. Alt+F2 still gives a login.
Undo: `sudo sh scripts/install_autostart.sh --undo && sudo reboot`.
To update the game: `sudo make install && sudo systemctl restart getty@tty1`.

## Troubleshooting

| Symptom | Check |
|---|---|
| Still get a login prompt on boot | `grep tty1 /etc/inittab` must show the game path; `ls -l /usr/local/bin/dotsandboxes` must exist and be executable. |
| `make: not found` / `gcc: not found` | `apk add build-base` |
| `valgrind` or `gdb` not found | Enable the community repo in `/etc/apk/repositories`, then `apk update`. |
| Strange characters on screen | Change the inittab line to `.../dotsandboxes --plain` (no colours). |
| Screen flashes / game restarts in a loop | Game exits immediately: on tty2 run `/usr/local/bin/dotsandboxes` by hand to see the error. |
