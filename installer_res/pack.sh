#!/bin/sh

PKG_DATE=$(date '+[%Y-%m-%d]')
DATE_FLAT=$(date '+%Y-%m-%d')
wget https://github.com/israpps/BDMAssault/releases/download/latest/BDMAssault.7z -O BDMAssault.7z
7z x BDMAssault.7z READY_TO_USE/FreeMcBoot/
cp ../Changelog.md __base/Changelog.md

for subdir in 1966 1965 1964 1963 1953
do
    echo "Packing v$subdir..."
    
    # For version 1966, rename directory to OSDMenu-Installer-[DATE]
    if [ "$subdir" = "1966" ]; then
        NEWDIR="OSDMenu-Installer-$DATE_FLAT"
        OUTFILE="../FMCB-1966.7z"
    else
        NEWDIR="FMCBinst-$subdir-$PKG_DATE"
        OUTFILE="../FMCB-$subdir.7z"
    fi

    cp -r __base/ "$NEWDIR/"
    cp -r "$subdir/INSTALL/" "$NEWDIR/INSTALL/"
    echo "$SHA8" > "$NEWDIR/lang/commit.txt"
    echo "title=FreeMcBoot v$subdir $PKG_DATE" > "$NEWDIR/title.cfg"
    echo "boot=FMCBInstaller.elf" >> "$NEWDIR/title.cfg"
    cp FMCBInstaller.elf "$NEWDIR/"
    cp FMCBInstaller_EXFAT.elf "$NEWDIR/"
    mkdir -p "$NEWDIR/FMCB_EXFAT/"
    cp -r READY_TO_USE/FreeMcBoot/SYS-CONF/ "$NEWDIR/FMCB_EXFAT/"
    cp EXFAT_INSTALL_INSTRUCTIONS.TXT "$NEWDIR/FMCB_EXFAT/INSTRUCTIONS.TXT"

    echo "Creating archive $OUTFILE"
    7z a -t7z -r "$OUTFILE" "$NEWDIR"/*
done
