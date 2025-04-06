# FreeMcBoot-Installer-UMCS-PS2BBL-MOD

is a fork of El_isra's (https://israpps.github.io/) Custom FMCB Installer. (https://israpps.github.io/FreeMcBoot-Installer/)

# Unified Memory Card System (UMCS) FreeMcBoot Installer
### Alternative to [KELFBinder: UMCS](https://github.com/NathanNeurotic/KELFbinder-UMCS/releases/tag/latest)
Works with or without [OpenTuna Installer: UMCS](https://github.com/NathanNeurotic/FreeMcTuna/releases/tag/UMCS-OPENTUNA-4eddc244)

---

## Overview
This installer integrates the PS2 Browser Boot Loader (PS2BBL) exploit and provides decrypted FreeMcBoot (FMCB) boot options for PlayStation 2. Designed specifically for PS2 homebrew enthusiasts, this UMCS standard installation ensures streamlined and flexible system management.

**Visionary:** [Tna-Plastic](https://github.com/TnA-Plastic)

---

## Features

- **PS2BBL Integration:**
  - Installs PS2BBL exploit directly from the installer menu.
  - Leverages the full UMCS standard environment for comprehensive functionality.

- **Multiple Boot Options:**
  - FMCB-1.966 *(requires SYS_FMCB-CFG)*
  - FMCB-1.953 *(requires SYS_FMCB-CFG and POWEROFF)*
  - OSDMENU *(does not require SYS_FMCB-CFG or POWEROFF)*

  *You can manually delete any boot options you do not need.*

- **Emergency Access:**
  - Hold `START` on system boot to directly access `wLaunchELF` in emergency mode.

- **RESCUE ELF Access:**
  - Press `R1 + START` on system boot to launch `RESCUE.ELF` for additional troubleshooting options.

---

## Optional Addition

### OpenTuna Integration (Optional)
You can optionally add OpenTuna for further system flexibility:

- **OpenTuna UMCS:** [Download Here](https://github.com/NathanNeurotic/FreeMcTuna/releases/tag/UMCS-OPENTUNA-4eddc244)

---

## Installation Instructions

1. Copy the entire UMCS installer folder (not just the ELF file) to a USB drive.
2. Insert the USB drive into your PlayStation 2.
3. Launch `wLaunchELF` on your PS2.
4. Navigate to `FileBrowser -> mass:/` and locate the UMCS installer folder.
5. Run the ELF file from within the folder to start the installer.
6. From the installer menu, select the option to install the **PS2BBL exploit**.
7. Follow the on-screen prompts to complete installation.
8. Upon reboot, UMCS Standard will be fully operational.

---

## Notes
- Ensure your memory card has sufficient free space.
- Do **not** move or run the ELF independently; the installer relies on folder structure and included assets.
- Carefully manage boot option deletions to maintain system stability.
- Keep the emergency access and rescue keys handy for troubleshooting.

---

### Credits  
Special thanks to the PS2 homebrew community and all contributors to the UMCS environment.

- [KELFBinder](https://github.com/israpps/KELFbinder) by [israpps](https://github.com/israpps)  
- [PS2BBL](https://israpps.github.io/PlayStation2-Basic-BootLoader/) by [israpps](https://github.com/israpps)  
- [Free McBoot Installer](https://github.com/israpps/FreeMcBoot-Installer) by [israpps](https://github.com/israpps)  
- [OSDMENU](https://github.com/pcm720/osdmenu-launcher) by [pcm720](https://github.com/pcm720)  
- [ESR Launcher](https://www.psx-place.com/resources/esr-launcher.1526/) by [HowlingWolfChelseaFantasy](https://github.com/HowlingWolfHWC)  
- [DKWDRV (Driver)](https://github.com/DKWDRV/DKWDRV)  
- [POPStarter Wiki](https://bitbucket.org/ShaolinAssassin/popstarter-documentation-stuff/wiki/Home) by [ShaolinAssassin](https://github.com/shaolinassassin)


Original Documentation:
----------------------
# FreeMcBoot-Installer

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3a7e81446817406a94eeb77bcc3762dd)](https://app.codacy.com/gh/israpps/FreeMcBoot-Installer?utm_source=github.com&utm_medium=referral&utm_content=israpps/FreeMcBoot-Installer&utm_campaign=Badge_Grade_Settings)
[![Build [All]](https://github.com/israpps/FreeMcBoot-Installer/actions/workflows/compile-core.yml/badge.svg)](https://github.com/israpps/FreeMcBoot-Installer/actions/workflows/compile-core.yml)

[![GitHub release (latest by SemVer and asset including pre-releases)](https://img.shields.io/github/downloads-pre/israpps/FreeMcBoot-Installer/latest/FMCB-1966.7z?color=black&label=&logo=GitHub)](https://github.com/israpps/FreeMcBoot-Installer/releases/tag/latest)
[![GitHub release (latest by SemVer and asset including pre-releases)](https://img.shields.io/github/downloads-pre/israpps/FreeMcBoot-Installer/latest/FMCB-1965.7z?color=black&label=&logo=GitHub)](https://github.com/israpps/FreeMcBoot-Installer/releases/tag/latest)
[![GitHub release (latest by SemVer and asset including pre-releases)](https://img.shields.io/github/downloads-pre/israpps/FreeMcBoot-Installer/latest/FMCB-1964.7z?color=black&label=&logo=GitHub)](https://github.com/israpps/FreeMcBoot-Installer/releases/tag/latest)
[![GitHub release (latest by SemVer and asset including pre-releases)](https://img.shields.io/github/downloads-pre/israpps/FreeMcBoot-Installer/latest/FMCB-1963.7z?color=black&label=&logo=GitHub)](https://github.com/israpps/FreeMcBoot-Installer/releases/tag/latest)
[![GitHub release (latest by SemVer and asset including pre-releases)](https://img.shields.io/github/downloads-pre/israpps/FreeMcBoot-Installer/latest/FMCB-1953.7z?color=black&label=&logo=GitHub)](https://github.com/israpps/FreeMcBoot-Installer/releases/tag/latest)

[![GitHub release (by tag)](https://img.shields.io/github/downloads/israpps/FreeMcBoot-Installer/APPS/total?color=000000&label=Apps%20Pack)](https://github.com/israpps/FreeMcBoot-Installer/releases/tag/APPS)

 Custom installers for FreeMcBoot 1.966, 1.965, 1.953, 1.964 and 1.963

They're packed with updated software.

In addition, several enhancements were made:
+ Installer:
  - Forbid multi install (corrupts memory card filesystem and doesn't achieve anything different than normal install)
  - Renamed normal install options to be user friendly
  - added manual HDD format option
  - added variant of installer that can be launched from exfat USB
+ Installation package:
  - updated Kernel patch updates for SCPH-10000 & SCPH-15000 to the one used on FreeMcBoot 1.966
  - Updated FreeHdBoot FSCK and MBR bootstraps to the one used on FreeHdBoot 1.966
  - added console shutdown ELF to all versions prior to 1.966
  - Optional custom IRX files to make FreeMcBoot/FreeHdBoot support EXFAT USB storage devices
  - internal HDD APPS partition header data changed to allow KELF execution from HDD-OSD.

[Original source code and binaries](https://sites.google.com/view/ysai187/home/projects/fmcbfhdb)

Special Thanks to SP193 for leaving the installer source code! it will help me out to add features to mi wLE mod ^^

-----

<details>
  <summary> <b> APPS Package contents: </b> </summary>

```ini
ESR ESR r10f_direct
[Open PS2 Loader]
1.0.0
latest
0.9.3
0.9.2
0.9.1
0.9.0
0.8
0.7
0.6
0.5
[Cheats]
Cheat device (PAL)
Cheat device (NTSC)
[uLaunchELF]
4.43x_isr
4.43x_isr_hdd
4.43a 41e4ebe
4.43a_khn
4.43a latest
[MultiMedia]
SMS
Argon
[PS2ESDL]
v0.810 OB
v0.825 OB
[GSM]
v0.23x
v0.38
[Emulators]
FCEU
InfoNES
SNES Station (0.2.4S)
SNES Station (0.2.6C)
SNES9x
InfoGB
GPS2
GPSP-KAI
ReGBA
TempGBA
VBAM
PVCS
RetroArch (1.9.1)
[Utilities]
MechaPwn 2.0
LensChanger 1.2b
Padtest
RDRAM TEST
PS2 Ident
HDD Checker v0.964
Memory Card Anihilator 2.0
HWC Language Selector
Launch disc
Shutdown System app
```

</details>
