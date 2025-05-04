#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <fileio.h>
#include <unistd.h>
#include <debug.h>
#include <tamtypes.h>
#include <libmc.h>
#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>

#include "opentuna_installer.h"
#include "system.h"
#include "UI.h"
#include "main.h"

int InstallOpenTunaIcon(int slot) {
    char mcPath[64];
    sprintf(mcPath, "mc%d:/OPENTUNA", slot);
    DeleteFolder(mcPath);
    // Step 1 - Delete all conflicting Tuna-related folders
    const char* tunaConflicts[] = {
        "FUNTUNA", "FUNTUNA-FORK", "BXEXEC-FUNTUNA", "BXEXEC-OPENTUNA",
        "FORTUNA", "OPENTUNA", "DST_OPENTUNA-INSTALLER", "DST_FMCT-INSTALLER"
    };
    for (int i = 0; i < sizeof(tunaConflicts)/sizeof(tunaConflicts[0]); i++) {
        sprintf(mcPath, "mc%d:/%s", slot, tunaConflicts[i]);
        DeleteFolder(mcPath);
    }
    // TODO: Step 1 - Delete existing /OPENTUNA/
    int icontype;
    char path[128];
    char iconBin[32];
    
    unsigned long romver = strtoul(g_RomVersion, NULL, 16);
    if (romver == 0x170)
        icontype = 1; // FAT170
    else if (romver >= 0x190)
        icontype = 3; // SLIMS
    else if (romver >= 0x110 && romver < 0x190)
        icontype = 2; // FATS
    else {
        DisplayErrorMessage("OpenTuna not supported on this ROM version.");
        return 0;
    }

    switch (icontype) {
        case 1: strcpy(iconBin, "opentuna_fat170.bin"); break;
        case 2: strcpy(iconBin, "opentuna_fats.bin"); break;
        case 3: strcpy(iconBin, "opentuna_slims.bin"); break;
        default:
            DisplayErrorMessage("Unknown icon type.");
            return 0;
    }

    sprintf(path, "%sINSTALL/OPENTUNA/%s", g_source_path, iconBin);
    if (!FileExists(path)) {
        DisplayErrorMessage("Required OpenTuna payload is missing.");
        return 0;
    }
    sprintf(path, "%sINSTALL/OPENTUNA/icon.sys", g_source_path);
    if (!FileExists(path)) {
        DisplayErrorMessage("icon.sys not found in OpenTuna folder.");
        return 0;
    }
    // TODO: Step 2 - Check required OpenTuna files
    // Step 3 - Copy selected .bin as icon.icn
    char destPath[64];
    sprintf(path, "%sINSTALL/OPENTUNA/%s", g_source_path, iconBin);
    sprintf(destPath, "mc%d:/OPENTUNA/icon.icn", slot);
    CopyFile(path, destPath);

    // Step 4 - Copy icon.sys
    sprintf(path, "%sINSTALL/OPENTUNA/icon.sys", g_source_path);
    sprintf(destPath, "mc%d:/OPENTUNA/icon.sys", slot);
    CopyFile(path, destPath);
    // TODO: Step 3 - Parse ROMVER and determine payload
    // TODO: Step 4 - Copy icon.icn and icon.sys
    // TODO: Step 5 - Set timestamps to 12/31/2099 11:59:59 PM
    // TODO: Step 6 - Display status and return
    return 1;
}
