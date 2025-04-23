[![Downloads](https://img.shields.io/github/downloads/NathanNeurotic/FreeMcBoot-Installer-UMCS/OSDMenu/total?color=purple&label=Downloads&style=flat)](https://github.com/NathanNeurotic/FreeMcBoot-Installer-UMCS/releases/tag/OSDMenu)

# OSDMenu Installer (Not UMCS or FMCB)

This package installs **PS2BBL** and **OSDMenu**, modified/created by @pcm720, the most compatible and latest FMCB release.
A collection of bundled tools and applications have also been included in the installer, so you can jump right in and play.
It is **not** a UMCS or FMCB installer. It is forked from FMCB installer and compatible with UMCS additions.

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

# 📝 OSDMENU.CNF Configuration

Fully compatible with FMCB 1.8 CNF. Extended with new options.

### Entry Limits

- Max entries: 250  
- Entry name: 79 characters  
- Path: 49 characters  
- Cursor text: 19 characters

### Settings

```ini
OSDSYS_video_mode=AUTO     ; AUTO | PAL | NTSC | 480p | 1080i
hacked_OSDSYS=1
OSDSYS_scroll_menu=1
OSDSYS_menu_x=...
OSDSYS_menu_y=...
OSDSYS_cursor_max_velocity=...
OSDSYS_selected_color=...
OSDSYS_unselected_color=...
name_OSDSYS_ITEM_001=...
path0_OSDSYS_ITEM_001=...
arg_OSDSYS_ITEM_001=...
cdrom_skip_ps2logo=1
cdrom_disable_gameid=1
cdrom_use_dkwdrv=1
path_LAUNCHER_ELF=mc?:/BOOT/launcher.elf
path_DKWDRV_ELF=mc?:/BOOT/DKWDRV.ELF
OSDSYS_Browser_Launcher=1
```

---

# 🙌 Credits

- **@pcm720** – OSDMenu, NHDDL
- **Neme & jimmikaelkael** – Original FMCB/OSDSYS patchers
- **@alex-free** – [TonyHax Game ID logic](https://github.com/alex-free/tonyhax/blob/master/loader/gameid-psx-exe.c)
- **@Maximus32** – smap_udpbd
- **@israpps/Matías Israelson** – [PS2BBL](https://github.com/israpps/PlayStation2-Basic-BootLoader)
- **@CosmicScale** – [RetroGEM PS2 Disc Launcher](https://github.com/CosmicScale/Retro-GEM-PS2-Disc-Launcher)
