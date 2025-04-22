#!/bin/sh

PKG_DATE=$(date '+[%Y-%m-%d]')
DATE_FLAT=$(date '+%Y-%m-%d')
wget https://github.com/israpps/BDMAssault/releases/download/latest/BDMAssault.7z -O BDMAssault.7z
7z x BDMAssault.7z READY_TO_USE/FreeMcBoot/

for subdir in 1966 1965 1964 1963 1953
do
    echo "Packing v$subdir..."

    # Determine output directory and archive name
    if [ "$subdir" = "1966" ]; then
        NEWDIR="OSDMenu-Installer-$DATE_FLAT"
        OUTFILE="../FMCB-1966.7z"
    else
        NEWDIR="FMCBinst-$subdir-$PKG_DATE"
        OUTFILE="../FMCB-$subdir.7z"
    fi

    # Build structure
    cp -r __base/ "$NEWDIR/"
    cp -r "$subdir/INSTALL/" "$NEWDIR/INSTALL/"

    # Replace installer ELF files with renamed versions
    cp FMCBInstaller.elf "$NEWDIR/OSDMENU-INSTALLER-FAT32.ELF"
    cp FMCBInstaller_EXFAT.elf "$NEWDIR/OSDMENU-INSTALLER-EXFAT.ELF"

    # Write metadata
    echo "$SHA8" > "$NEWDIR/lang/commit.txt"
    echo "title=FreeMcBoot v$subdir $PKG_DATE" > "$NEWDIR/title.cfg"
    echo "boot=OSDMENU-INSTALLER-FAT32.ELF" >> "$NEWDIR/title.cfg"

    # Cleanup
    rm -rf "$NEWDIR/FMCB_EXFAT/"
    rm -f "$NEWDIR/README.txt"
    rm -f "$NEWDIR/Changelog.md"
    rm -rf "$NEWDIR/Changelog"
    
    echo "Creating archive $OUTFILE"
    7z a -t7z -r "$OUTFILE" "$NEWDIR"/*
done
