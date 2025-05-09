#!/bin/sh

# Script to package FreeMcBoot Installer releases

# Generate date-based versioning components
PKG_DATE=$(date '+[%Y-%m-%d]')
DATE_FLAT=$(date '+%Y-%m-%d') # For the OSDMenu-Installer main release name

# Fetch latest BDMAssault (exFAT USB drivers)
echo "Fetching BDMAssault (USB exFAT drivers)..."
wget https://github.com/israpps/BDMAssault/releases/download/latest/BDMAssault.7z -O BDMAssault.7z
# Extract to a known temporary location. The original path READY_TO_USE/FreeMcBoot/ might be too specific
# or create unwanted directory structures if not handled carefully.
# For now, assuming this extraction path is correct and its contents are used later if needed.
7z x BDMAssault.7z -oBDMAssault_temp READY_TO_USE/FreeMcBoot/

# Loop through different FMCB versions to package
# These versioned subdirectories (1966, 1965, etc.) are expected to be in installer_res/
for subdir in 1966 1965 1964 1963 1953
do
    echo ""
    echo "Packing v$subdir..."

    # Determine output directory and archive name based on the FMCB version
    if [ "$subdir" = "1966" ]; then
        # This is treated as the primary package by compile-core.yml
        NEWDIR_BASENAME="OSDMenu-Installer" # Base name for the directory inside the archive
        NEWDIR="${NEWDIR_BASENAME}-${DATE_FLAT}" # e.g., OSDMenu-Installer-2025-05-09
        OUTFILE_BASENAME="FMCB-${subdir}" # e.g. FMCB-1966
        OUTFILE="../${OUTFILE_BASENAME}.7z" # Output to parent directory (project root)
    else
        NEWDIR_BASENAME="FMCBinst-${subdir}"
        NEWDIR="${NEWDIR_BASENAME}-${PKG_DATE}" # e.g., FMCBinst-1965-[2025-05-09]
        OUTFILE_BASENAME="FMCB-${subdir}"
        OUTFILE="../${OUTFILE_BASENAME}.7z"
    fi

    echo "Creating package directory: $NEWDIR"
    rm -rf "$NEWDIR" # Clean up previous attempt
    mkdir -p "$NEWDIR"

    # --- Build base structure ---
    echo "Copying base files from __base/ ..."
    if [ -d "__base" ]; then
        cp -r __base/* "$NEWDIR/" # Copies contents of __base (like lang, changelog templates)
    else
        echo "Warning: __base/ directory not found."
    fi

    # Ensure the target INSTALL directory exists in NEWDIR
    mkdir -p "$NEWDIR/INSTALL/"

    # Copy version-specific INSTALL contents (e.g., from 1966/INSTALL/* into $NEWDIR/INSTALL/)
    if [ -d "$subdir/INSTALL" ]; then
        echo "Copying version-specific INSTALL files from $subdir/INSTALL/ ..."
        cp -r "$subdir/INSTALL/"* "$NEWDIR/INSTALL/"
    else
        echo "Warning: Version-specific INSTALL directory $subdir/INSTALL/ not found."
    fi

    # --- Copy our generic OpenTuna assets ---
    # These are expected by the installer to be in INSTALL/OPENTUNA/ relative to the ELF.
    # Source is installer_res/INSTALL/OPENTUNA/
    if [ -d "INSTALL/OPENTUNA" ]; then 
        echo "Copying OpenTuna assets to $NEWDIR/INSTALL/OPENTUNA/ ..."
        mkdir -p "$NEWDIR/INSTALL/OPENTUNA" 
        cp -r INSTALL/OPENTUNA/* "$NEWDIR/INSTALL/OPENTUNA/"
    else
        echo "Warning: INSTALL/OPENTUNA directory not found in installer_res. OpenTuna payloads will be missing."
    fi

    # Copy installer ELFs (built by Makefile into installer_res/)
    # The Makefile's EE_BIN_DIR is overridden by compile-core.yml to ../installer_res/
    # So, FMCBInstaller.elf and FMCBInstaller_EXFAT.elf should be in the current directory (installer_res/)
    # when this script is run by compile-core.yml.
    echo "Copying installer ELFs..."
    ELF_FAT32="FMCBInstaller.elf"
    ELF_EXFAT="FMCBInstaller_EXFAT.elf"

    if [ -f "$ELF_FAT32" ]; then
        cp "$ELF_FAT32" "$NEWDIR/OSDMENU-INSTALLER-FAT32.ELF"
    else
        echo "Warning: $ELF_FAT32 not found in $(pwd)."
    fi
    if [ -f "$ELF_EXFAT" ]; then
        cp "$ELF_EXFAT" "$NEWDIR/OSDMENU-INSTALLER-EXFAT.ELF"
    else
        echo "Warning: $ELF_EXFAT not found in $(pwd)."
    fi

    # Write metadata files
    echo "Writing metadata (commit.txt, title.cfg)..."
    # SHA8 environment variable is set by compile-core.yml
    if [ -n "$SHA8" ]; then
        echo "$SHA8" > "$NEWDIR/lang/commit.txt"
    else
        echo "UnknownCommit" > "$NEWDIR/lang/commit.txt" 
    fi
    
    echo "title=FreeMcBoot v$subdir $PKG_DATE" > "$NEWDIR/title.cfg"
    echo "boot=OSDMENU-INSTALLER-FAT32.ELF" >> "$NEWDIR/title.cfg" 

    # Cleanup unnecessary files from the package
    echo "Cleaning up package contents..."
    # These paths are relative to $NEWDIR
    rm -f "$NEWDIR/README.txt" # Assuming this comes from __base and is not needed
    rm -f "$NEWDIR/Changelog.md" # Assuming this comes from __base
    rm -rf "$NEWDIR/Changelog"   # Assuming this is a directory from __base

    # Create the 7z archive
    echo "Creating archive $OUTFILE (archiving contents of $NEWDIR)..."
    # Archive the NEWDIR itself, so it becomes the root folder in the archive
    7z a -t7z -r "$OUTFILE" "$NEWDIR" 
    
    echo "Finished packing v$subdir into $OUTFILE."
    echo "------------------------------------"
    rm -rf "$NEWDIR" # Clean up the temporary packaging directory for this version
done

# Cleanup downloaded BDMAssault
echo "Cleaning up downloaded BDMAssault files..."
rm -f BDMAssault.7z
rm -rf BDMAssault_temp/

echo "All packaging complete."
