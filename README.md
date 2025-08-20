# THIS BRANCH IS FOR MY WORK - NONE OF THE README IS ACCURATE. WAIT FOR OFFICIAL RELEASES.

[![Downloads](https://img.shields.io/github/downloads/NathanNeurotic/FreeMcBoot-Installer-UMCS/OSDMenu/total?color=purple&label=Downloads&style=flat)](https://github.com/NathanNeurotic/FreeMcBoot-Installer-UMCS/releases/tag/OSDMenu)

# OSDMenu Installer (Not UMCS or FMCB)

This package installs **PS2BBL** and **OSDMenu**, modified/created by @pcm720, the most compatible and latest FMCB release.
A collection of bundled tools and applications have also been included in the installer, so you can jump right in and play.
It is **not** a UMCS or FMCB installer. It is forked from FMCB installer and compatible with UMCS and SAS additions.


## 📦 How To Install

1. **Download and extract** the `.7z` archive to your **FAT32 or exFAT** USB storage device.
2. **Access the USB** from your PS2 using **uLaunchELF**, **wLaunchELF**, or **FDVDB**.
3. **Launch the correct OSDMenu Installer ELF** for your USB format:
   - Use the **FAT32** installer if your USB is formatted as FAT32.
   - Use the **exFAT** installer if your USB is formatted as exFAT.
4. Once inside the installer, **press `R1` twice** to open the **Format MC** menu.
   - 🛑 *This step is strongly recommended and often required.*
   - 💾 *Backup any memory card data you want to keep using wLaunchELF (copy from `mc?:/` to `mass:/`).*
5. When you're ready, **format the Memory Card**.
6. After formatting, **press `L1` twice** to return to the main installer menu.
7. **Select “Install OSDMenu”**.
8. Choose your preferred **PS2BBL coverage type**.
9. Wait for the installer to finish.  
   🕐 *It may take a while — the progress bar might appear stuck, but it’s working. Please be patient.*
10. Once the installation is complete, **reboot your PS2** and enjoy the new setup.


## Overview

- ✅ Installs **PS2BBL** + **OSDMenu** and a pre-configured environment  
- ✅ Bundled apps that can stay or go after install: Neutrino, NHDDL, OPL, ESR-Launcher, wLaunchELF  
- ✅ Safe to delete everything **except**:
  - `PS2BBL` (**B?EXEC-SYSTEM** folder(s))
  - `BOOT` Folder
  - `SYS-CONF` Folder 
  These are essential for the exploit to function.

---

## Repositories

- 📁 **Installer Source Repo:**  
  https://github.com/NathanNeurotic/FreeMcBoot-Installer-UMCS/tree/OSDMenu  
- 🌐 **OSDMenu Homepage:**  
  https://github.com/pcm720/OSDMenu  

Thanks to @[pcm720](https://github.com/pcm720) for OSDMenu, NHDDL, and ongoing support for the PS2 homebrew community.


# 🔄 Differences from FMCB 1.8 (and other FMCB versions)

- Replaces initialization with external bootloader (PS2BBL)
- Drops ESR and USB support from patcher (can still launch APPS from USB once in OSDMenu)
- Custom OSDSYS replaces FMCB’s menu with extra details and higher compatibility.
- No button-held ELF launching/LaunchKeys were removed from PS2BBL's Start Up.
- BOOT.ELF still supports launch keys.
- Expands CD/DVD handling:
  - Skip PS2 logo
  - Auto VMC mounts
  - Visual Game ID
  - PS1 disc booting via DKWDRV
- Supports more display modes: `480p`, `1080i`, and line-doubled `240p`
- Fully supports “protokernel” consoles like SCPH-10000/15000
- Supports launching directly from Memory Card Browser - Yes, you can launch an application by selecting its Save Icon!
- FMCB launcher split into **patcher** and **launcher** due to memory constraints

---

# 🧩 Components

## Patcher

Located at `mc?:/BOOT/patcher.elf`, it applies OSDSYS patches based on `OSDMENU.CNF`.

### Features:

- Custom OSDSYS menu (up to 255 entries)
- Infinite scroll, custom prompts and headers
- Forced video modes (`PAL`, `NTSC`, `480p`, `1080i`)
- Auto disc launch bypass
- HDD update check bypass
- Enhanced version submenu (ROM/EE/GS/MechaCon)
- Memory Card Browser app launcher via `title.cfg`

### Protokernel Limitations:

- No auto disc launch
- No prompt customization
- No PAL mode

---

## Launcher

Located at `mc?:/BOOT/launcher.elf`, it handles launching ELFs/discs across devices.

### Supported Devices:

- `mc?` — Memory Cards
- `mass:`, `usb:` — USB (BDM supported)
- `ata:` — exFAT HDD (BDM)
- `mx4sio:`, `ilink:`, `udpbd:` — Supported via BDM
- `hdd0:` — APA HDD
- `cdrom` — CD/DVD
- `fmcb:` — Config-based FMCB paths

### Special Handlers:

- **`udpbd:`** – Reads IP from `IPCONFIG.DAT`
- **`cdrom:`** – Supports:
  - `-nologo`, `-nogameid`, `-dkwdrv`
  - `-dkwdrv=mc?:/path` for custom DKWDRV path
- **`fmcb:`** – Reads indexed OSDMENU.CNF entries
- **Quickboot:** Loads `.CNF` with paths/args if no CLI args

```ini
boot=boot.elf
path=mc?:/BOOT/BOOT.ELF
arg=-testarg
```

---

## OSDMENU.CNF

Most of `OSDMENU.CNF` settings are directly compatible with those from FMCB 1.8 `FREEMCB.CNF`.

### Character limits

OSDMenu supports up to 250 custom menu entries, each up to 79 characters long.  
Note that left and right cursors are limited to 19 characters and top and bottom delimiters are limited to 79 characters.  
Launcher and DKWDRV paths are limited to 49 characters.

### Configuration options

1. `OSDSYS_video_mode` — force OSDSYS mode. Valid values are `AUTO`, `PAL`, `NTSC`, `480p` or `1080i`
2. `hacked_OSDSYS` — enables or disables OSDSYS patches
3. `OSDSYS_scroll_menu` — enables or disables infinite scrolling
4. `OSDSYS_menu_x` — menu X center coordinate
5. `OSDSYS_menu_y` — menu Y center coordinate
6. `OSDSYS_enter_x` — `Enter` button X coordinate (at main OSDSYS menu)
7. `OSDSYS_enter_y` — `Enter` button Y coordinate (at main OSDSYS menu)
8. `OSDSYS_version_x` — `Version` button X coordinate (at main OSDSYS menu)
9. `OSDSYS_version_y` — `Version` button Y coordinate (at main OSDSYS menu)
10. `OSDSYS_cursor_max_velocity` — max cursor speed
11. `OSDSYS_cursor_acceleration` — cursor speed
12. `OSDSYS_left_cursor` — left cursor text
13. `OSDSYS_right_cursor` — right cursor text
14. `OSDSYS_menu_top_delimiter` — top menu delimiter text
15. `OSDSYS_menu_bottom_delimiter` — bottom menu delimiter text
16. `OSDSYS_num_displayed_items` — the number of menu items displayed
17. `OSDSYS_Skip_Disc` — enables/disables automatic CD/DVD launch
18. `OSDSYS_Skip_Logo` — enables/disables SCE logo
19. `OSDSYS_Inner_Browser` — enables/disables going to the Browser after launching OSDSYS
20. `OSDSYS_selected_color` — color of selected menu entry
21. `OSDSYS_unselected_color` — color of unselected menu entry
22. `name_OSDSYS_ITEM_???` — menu entry name
23. `path?_OSDSYS_ITEM_???` — path to ELF. Also supports the following special paths: `cdrom`, `OSDSYS`, `POWEROFF`

New to this launcher:

24. `arg_OSDSYS_ITEM_???` — custom argument to be passed to the ELF. Each argument needs a separate entry.
25. `cdrom_skip_ps2logo` — enables or disables running discs via `rom0:PS2LOGO`. Useful for MechaPwn-patched consoles.
26. `cdrom_disable_gameid` — disables or enables visual Game ID
27. `cdrom_use_dkwdrv` — enables or disables launching DKWDRV for PS1 discs
28. `path_LAUNCHER_ELF` — custom path to launcher.elf. The path MUST be on the memory card
29. `path_DKWDRV_ELF` — custom path to DKWDRV.ELF. The path MUST be on the memory card
30. `OSDSYS_Browser_Launcher` — enables/disables patch for launching applications from the Browser 

---

# 🙌 Credits

- **@pcm720** – OSDMenu, NHDDL
- **Neme & jimmikaelkael** – Original FMCB/OSDSYS patchers
- **@alex-free** – [TonyHax Game ID logic](https://github.com/alex-free/tonyhax/blob/master/loader/gameid-psx-exe.c)
- **@Maximus32** – smap_udpbd
- **@israpps/Matías Israelson** – [PS2BBL](https://github.com/israpps/PlayStation2-Basic-BootLoader)
- **@CosmicScale** – [RetroGEM PS2 Disc Launcher](https://github.com/CosmicScale/Retro-GEM-PS2-Disc-Launcher)
- **OSDMenu Save Icon** - taken from "PS2 Dashboard" (https://skfb.ly/pnAyJ) by deaffrasiertheboy is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/). Merged with [@koraxial](https://github.com/koraxial)'s SAS icons.
