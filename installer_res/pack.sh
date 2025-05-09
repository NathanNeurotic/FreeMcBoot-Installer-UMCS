#!/bin/sh

# Script to package FreeMcBoot Installer releases

# Generate date-based versioning components
PKG_DATE=$(date '+[%Y-%m-%d]')
DATE_FLAT=$(date '+%Y-%m-%d') # For the OSDMenu-Installer main release

# Fetch latest BDMAssault (exFAT USB drivers) - This part seems specific to a particular setup/fork.
# If these are general purpose and always needed, this is fine.
# If they are only for the exFAT version, this could be conditional or handled differently.
echo "Fetching BDMAssault (USB exFAT drivers)..."
wget https://github.com/israpps/BDMAssault/releases/download/latest/BDMAssault.7z -O BDMAssault.7z
7z x BDMAssault.7z READY_TO_USE/FreeMcBoot/ # Extracts to READY_TO_USE/FreeMcBoot/

# Loop through different FMCB versions to package
# The script assumes directories like '1966', '1965', etc., exist in the current path (which is installer_res/)
# and contain version-specific INSTALL folders.
for subdir in 1966 1965 1964 1963 1953
do
    echo "Packing v$subdir..."

    # Determine output directory and archive name based on the FMCB version
    if [ "$subdir" = "1966" ]; then
        # This seems to be the main/latest release package
        NEWDIR="OSDMenu-Installer-$DATE_FLAT"
        OUTFILE="../FMCB-1966.7z" # Output to parent directory
    else
        NEWDIR="FMCBinst-$subdir-$PKG_DATE"
        OUTFILE="../FMCB-$subdir.7z" # Output to parent directory
    fi

    echo "Creating package directory: $NEWDIR"
    rm -rf "$NEWDIR" # Clean up previous attempt
    mkdir -p "$NEWDIR"

    # --- Build base structure ---
    echo "Copying base files..."
    cp -r __base/* "$NEWDIR/" # Copies contents of __base (like lang, changelog)

    # Ensure the target INSTALL directory exists in NEWDIR
    mkdir -p "$NEWDIR/INSTALL/"

    # Copy version-specific INSTALL contents (e.g., from 1966/INSTALL/* into $NEWDIR/INSTALL/)
    if [ -d "$subdir/INSTALL" ]; then
        echo "Copying version-specific INSTALL files from $subdir/INSTALL/ ..."
        cp -r "$subdir/INSTALL/"* "$NEWDIR/INSTALL/"
    else
        echo "Warning: Version-specific INSTALL directory $subdir/INSTALL/ not found."
    fi

    # --- NEW: Copy our generic OpenTuna assets ---
    # These are expected by the installer to be in INSTALL/OPENTUNA/ relative to the ELF.
    if [ -d "INSTALL/OPENTUNA" ]; then # Checks for installer_res/INSTALL/OPENTUNA/
        echo "Copying OpenTuna assets..."
        mkdir -p "$NEWDIR/INSTALL/OPENTUNA" # Ensure target OPENTUNA dir exists in package
        cp -r INSTALL/OPENTUNA/* "$NEWDIR/INSTALL/OPENTUNA/"
    else
        echo "Warning: INSTALL/OPENTUNA directory not found in installer_res. OpenTuna payloads will be missing."
    fi
    # --- End NEW Section ---

    # Replace installer ELF files with renamed versions
    # Assumes FMCBInstaller.elf and FMCBInstaller_EXFAT.elf are present in the current directory (installer_res/)
    # These are built by the Makefile with EE_BIN_DIR=../installer_res/
    echo "Copying installer ELFs..."
    if [ -f "FMCBInstaller.elf" ]; then
        cp FMCBInstaller.elf "$NEWDIR/OSDMENU-INSTALLER-FAT32.ELF"
    else
        echo "Warning: FMCBInstaller.elf not found in installer_res."
    fi
    if [ -f "FMCBInstaller_EXFAT.elf" ]; then
        cp FMCBInstaller_EXFAT.elf "$NEWDIR/OSDMENU-INSTALLER-EXFAT.ELF"
    else
        echo "Warning: FMCBInstaller_EXFAT.elf not found in installer_res."
    fi

    # Write metadata files
    echo "Writing metadata (commit, title.cfg)..."
    # SHA8 should be passed as an environment variable or generated here if possible
    if [ -n "$SHA8" ]; then
        echo "$SHA8" > "$NEWDIR/lang/commit.txt"
    else
        echo "UnknownCommit" > "$NEWDIR/lang/commit.txt" # Fallback if SHA8 env var is not set
    fi
    
    echo "title=FreeMcBoot v$subdir $PKG_DATE" > "$NEWDIR/title.cfg"
    echo "boot=OSDMENU-INSTALLER-FAT32.ELF" >> "$NEWDIR/title.cfg" # Default boot ELF

    # Cleanup unnecessary files from the package
    echo "Cleaning up package..."
    rm -rf "$NEWDIR/FMCB_EXFAT/" # This directory seems to be from BDMAssault extraction, maybe not needed in final package
    rm -f "$NEWDIR/README.txt"
    rm -f "$NEWDIR/Changelog.md" # Assuming these are from __base/ and not desired in package root
    rm -rf "$NEWDIR/Changelog"   # Assuming this is a directory from __base/

    # Create the 7z archive
    echo "Creating archive $OUTFILE..."
    7z a -t7z -r "$OUTFILE" "$NEWDIR"/* # Archive contents of NEWDIR
    
    echo "Finished packing v$subdir into $OUTFILE."
    echo "------------------------------------"
    rm -rf "$NEWDIR" # Clean up the temporary packaging directory
done

# Cleanup downloaded BDMAssault
echo "Cleaning up downloaded files..."
rm -f BDMAssault.7z
rm -rf READY_TO_USE/

echo "All packaging complete."
