// menu_opentuna.c
// Handles OpenTuna Exploit Installation

#include <stdio.h>      // For sprintf, NULL, fopen, fread, fclose, perror, printf
#include <string.h>     // For strcpy, strcmp, strrchr, strlen, memset
#include <stdlib.h>     // For strtoul
#include <kernel.h>     // For basic PS2 types, ee_sema_t, sceMcStDateTime, etc.
#include <dirent.h>     // For getcwd 
#include <sys/stat.h>   // For S_ISDIR (often abstracted by PS2SDK fileio or FMCB's system calls)
#include <fcntl.h>      // For O_CREAT, O_WRONLY, O_TRUNC for basic file ops
#include <errno.h>      // For errno

// FMCB Project Specific Includes
#include "main.h"       // For g_Port, g_Slot, menu events, MAX_PATH, MENU_EVENT_RETURN_TO_MAIN, FILE_TYPE_FILE, SelectButton, CancelButton etc.
#include "system.h"     // For createFolder, CopyFiles, DeleteFolder (and potentially SecrGetRomID wrapper)
#include "UI.h"         // For DisplayMessageTimed, GetString, UI constants (SYS_MSG_INFO, SYS_MSG_ERROR, SYS_MSG_SUCCESS)
                        // GetString is usually declared in lang.h, which UI.h should include.
#include "mctools_rpc.h"// For mcSetFileInfo, mcSync, sceMcResSucceed, sceMcFileInfoModify
#include "pad.h"        // For ReadCombinedPadStatus_raw, PadInitPads etc. 
#include "lang.h"       // For GetString, GetUILabel and all SYS_UI_LBL_OT_... and SYS_UI_MSG_OT_... constants
#include "graphics.h"   // For DrawBackground, FontPrintf (or FontPrint), SyncFlipFB etc. (assuming UIDrawGlobal is accessible or wrappers exist)

// --- Configuration for OpenTuna Installation ---
#define OPENTUNA_DIR_NAME "OPENTUNA"
#define OPENTUNA_INSTALL_SUBDIR "INSTALL/OPENTUNA/" // Relative to installer's root path

// Payloads as identified from user's textual description of image
// (to be sourced from [InstallerLaunchPath]/INSTALL/OPENTUNA/)
#define OPENTUNA_PAYLOAD_SLIMS              "OpenTuna_Slims.bin"
#define OPENTUNA_PAYLOAD_FAT170             "OpenTuna_FAT-170.bin"
#define OPENTUNA_PAYLOAD_FATS               "OpenTuna_FAT-110-120-150-160.bin"
#define OPENTUNA_ICON_SYS_SOURCE_FILENAME   "icon.sys" 

// Destination names on Memory Card
#define OPENTUNA_DEST_ICON_ICN_FILENAME "icon.icn"
#define OPENTUNA_DEST_ICON_SYS_FILENAME "icon.sys"

// ROM Version Categories (from reference OpenTuna Installer)
enum OpenTunaPayloadType {
    OT_TYPE_SLIMS = 0,   // ROM >= 0x190
    OT_TYPE_FATS,        // ROM < 0x190 && ROM >= 0x110 (excluding 0x170)
    OT_TYPE_FAT170,      // ROM == 0x170
    OT_TYPE_UNSUPPORTED
};

// Aliases for manifest file content (from reference OpenTuna Installer)
static const char* g_OpenTunaTypeAliases[] = {
    "190+", // SLIMS
    "110+", // FATS
    "170 ", // FAT170
    "UNSP"  // UNSUPPORTED
};

// Filenames for the .cnf manifest file on MC, matching reference OpenTuna installer's ICONFILE_NAMES
static const char* g_OpenTunaManifestFileNames[] = {
    "slims",  // SLIMS
    "fats",   // FATS
    "fat170", // FAT170
    "unkn"    // UNSUPPORTED / UNKNOWN (should ideally not create a .cnf for unsupported)
};

// Source payload filenames corresponding to the enum types
static const char* g_OpenTunaPayloadSourceFilenames[] = {
    OPENTUNA_PAYLOAD_SLIMS,
    OPENTUNA_PAYLOAD_FATS,
    OPENTUNA_PAYLOAD_FAT170,
    NULL // For OT_TYPE_UNSUPPORTED
};

// Menu items for the OpenTuna sub-menu
#define OT_MENU_ITEM_AUTO    0
#define OT_MENU_ITEM_MANUAL  1
#define OT_MENU_ITEM_CLEANUP 2
#define OT_MENU_ITEM_RETURN  3
#define OT_MENU_ITEM_COUNT   4

// --- Static Helper Function Declarations --- (Prototypes only)
static enum OpenTunaPayloadType get_opentuna_payload_type(void);
static int opentuna_delete_existing_folders(int mc_port, int mc_slot);
static int opentuna_perform_install_core(int mc_port, int mc_slot, enum OpenTunaPayloadType payload_type);
static int opentuna_apply_timestamp(int mc_port, int mc_slot);
static void draw_opentuna_menu_gfx(int selected_item); 
static enum OpenTunaPayloadType ShowManualPayloadSelectionMenu(void); 


// --- Public Functions (to be called from menu.c / main.c) ---

int menu_opentuna_install(int connecting_event, int current_selection) {
    static int selected_item = OT_MENU_ITEM_AUTO;
    u32 pad_data_old = 0, pad_data_cur = 0, pad_data_tap = 0;
    int next_event = MENU_EVENT_NONE; 

    (void)connecting_event; 
    if(current_selection >= OT_MENU_ITEM_AUTO && current_selection < OT_MENU_ITEM_COUNT) { 
        selected_item = current_selection;
    }

    printf("menu_opentuna_install: Entered. Port: %d, Slot: %d\n", g_Port, g_Slot);
    
    // Initial display of the menu title (if desired as a timed message, or handled by draw function)
    // DisplayMessageTimed(GetString(SYS_UI_LBL_OT_MENU_TITLE), 1000, SYS_MSG_INFO);

    while (next_event == MENU_EVENT_NONE) {
        pad_data_cur = ReadCombinedPadStatus_raw(); 
        pad_data_tap = pad_data_cur & ~pad_data_old; 
        pad_data_old = pad_data_cur;

        if (pad_data_tap & PAD_DOWN) {
            selected_item = (selected_item + 1) % OT_MENU_ITEM_COUNT;
        } else if (pad_data_tap & PAD_UP) {
            selected_item = (selected_item - 1 + OT_MENU_ITEM_COUNT) % OT_MENU_ITEM_COUNT;
        } else if (pad_data_tap & SelectButton) { 
            switch (selected_item) {
                case OT_MENU_ITEM_AUTO:
                    printf("Menu: Selected Auto Install...\n");
                    // Optional: Confirmation prompt
                    // if (DisplayPromptMessage(GetString(SYS_UI_MSG_OT_AUTO_CONFIRM), GetString(SYS_UI_LBL_YES), GetString(SYS_UI_LBL_NO)) == 1) {
                        opentuna_auto_install(g_Port, g_Slot);
                    // } else { DisplayMessageTimed(GetString(SYS_UI_MSG_OT_MANUAL_CANCELLED), 1500, SYS_MSG_INFO); } // Example cancel
                    break;
                case OT_MENU_ITEM_MANUAL:
                    printf("Menu: Selected Manual Install...\n");
                    opentuna_manual_install(g_Port, g_Slot);
                    break;
                case OT_MENU_ITEM_CLEANUP:
                    printf("Menu: Selected Cleanup...\n");
                    // Optional: Confirmation prompt
                    // if (DisplayPromptMessage(GetString(SYS_UI_MSG_OT_CLEANUP_CONFIRM), GetString(SYS_UI_LBL_YES), GetString(SYS_UI_LBL_NO)) == 1) {
                        opentuna_cleanup(g_Port, g_Slot);
                    // } else { DisplayMessageTimed(GetString(SYS_UI_MSG_OT_MANUAL_CANCELLED), 1500, SYS_MSG_INFO); } // Example cancel
                    break;
                case OT_MENU_ITEM_RETURN:
                    printf("Menu: Selected Return to Main Menu...\n");
                    next_event = MENU_EVENT_RETURN_TO_MAIN;
                    break;
            }
        } else if (pad_data_tap & CancelButton) { 
            printf("Menu: Cancel pressed, returning to main menu...\n");
            next_event = MENU_EVENT_RETURN_TO_MAIN;
        }

        draw_opentuna_menu_gfx(selected_item); 
        SyncFlipFB(&UIDrawGlobal); 
    }

    printf("menu_opentuna_install: Exiting with event %d.\n", next_event);
    return next_event;
}

int opentuna_auto_install(int mc_port, int mc_slot) {
    printf("opentuna_auto_install: Called for mc%d:/, slot %d\n", mc_port, mc_slot);
    DisplayMessageTimed(GetString(SYS_UI_MSG_OT_AUTO_START), 1000, SYS_MSG_INFO);

    enum OpenTunaPayloadType payload_type = get_opentuna_payload_type();

    if (payload_type == OT_TYPE_UNSUPPORTED) {
        printf("Unsupported ROM version for OpenTuna auto-install.\n");
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_ERR_UNSUPPORTED_MODEL), 3000, SYS_MSG_ERROR);
        return -1; 
    }

    printf("Detected payload type for auto-install: %s (Source: %s)\n", 
           g_OpenTunaTypeAliases[payload_type], 
           g_OpenTunaPayloadSourceFilenames[payload_type]);

    if (opentuna_delete_existing_folders(mc_port, mc_slot) < 0) { 
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_WARN_CLEANUP_ISSUES), 2000, SYS_MSG_INFO);
    }

    if (opentuna_perform_install_core(mc_port, mc_slot, payload_type) != 0) {
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_ERR_INSTALL_FAILED), 3000, SYS_MSG_ERROR);
        return -1;
    }

    if (opentuna_apply_timestamp(mc_port, mc_slot) != 0) {
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_WARN_TIMESTAMP_FAILED), 2000, SYS_MSG_INFO);
    }

    DisplayMessageTimed(GetString(SYS_UI_MSG_OT_SUCCESS_AUTO), 3000, SYS_MSG_SUCCESS);
    printf("OpenTuna auto-install successful for mc%d:/, slot %d.\n", mc_port, mc_slot);
    return 0; 
}

int opentuna_manual_install(int mc_port, int mc_slot) {
    printf("opentuna_manual_install: Called for mc%d:/, slot %d\n", mc_port, mc_slot);
    enum OpenTunaPayloadType selected_payload_type;

    selected_payload_type = ShowManualPayloadSelectionMenu(); 

    if (selected_payload_type == OT_TYPE_UNSUPPORTED) { 
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_MANUAL_CANCELLED), 1500, SYS_MSG_INFO); 
        return 1; 
    }
    
    printf("Manual install selected type: %s (Source: %s)\n", 
           g_OpenTunaTypeAliases[selected_payload_type],
           g_OpenTunaPayloadSourceFilenames[selected_payload_type]);
    
    DisplayMessageTimed(GetString(SYS_UI_MSG_OT_MANUAL_SELECT_PAYLOAD), 1000, SYS_MSG_INFO); // Or a "Starting manual install..." message

    if (opentuna_delete_existing_folders(mc_port, mc_slot) < 0) {
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_WARN_CLEANUP_ISSUES), 2000, SYS_MSG_INFO); 
    }

    if (opentuna_perform_install_core(mc_port, mc_slot, selected_payload_type) != 0) {
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_ERR_INSTALL_FAILED), 3000, SYS_MSG_ERROR); 
        return -1;
    }

    if (opentuna_apply_timestamp(mc_port, mc_slot) != 0) {
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_WARN_TIMESTAMP_FAILED), 2000, SYS_MSG_INFO); 
    }

    DisplayMessageTimed(GetString(SYS_UI_MSG_OT_SUCCESS_MANUAL), 3000, SYS_MSG_SUCCESS); 
    printf("OpenTuna manual install successful for mc%d:/, slot %d.\n", mc_port, mc_slot);
    return 0; 
}

int opentuna_cleanup(int mc_port, int mc_slot) {
    printf("opentuna_cleanup: Called for mc%d:/, slot %d\n", mc_port, mc_slot);
    DisplayMessageTimed(GetString(SYS_UI_MSG_OT_CLEANUP_START), 1000, SYS_MSG_INFO); 

    if (opentuna_delete_existing_folders(mc_port, mc_slot) < 0) { 
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_WARN_CLEANUP_ISSUES), 2000, SYS_MSG_ERROR); 
        return -1; 
    } else {
        DisplayMessageTimed(GetString(SYS_UI_MSG_OT_CLEANUP_SUCCESS), 2000, SYS_MSG_SUCCESS); 
    }
    printf("OpenTuna cleanup finished for mc%d:/, slot %d.\n", mc_port, mc_slot);
    return 0; 
}


// --- Static Helper Function Implementations --- (Function definitions/bodies)

static void draw_opentuna_menu_gfx(int selected_item) {
    char SPRINTF_BUFF[128]; 
    const char* item_label;

    // Conceptual: ClearScreen(); or DrawBackground(&UIDrawGlobal, &BackgroundTexture);
    // This function will eventually use proper FMCB UI drawing functions from UI.c/graphics.c

    sprintf(SPRINTF_BUFF, "\n\n===== %s =====\n", GetString(SYS_UI_LBL_OT_MENU_TITLE)); 
    printf("%s", SPRINTF_BUFF);

    for (int i = 0; i < OT_MENU_ITEM_COUNT; i++) {
        switch(i) {
            case OT_MENU_ITEM_AUTO:    item_label = GetString(SYS_UI_LBL_OT_ITEM_AUTO); break;
            case OT_MENU_ITEM_MANUAL:  item_label = GetString(SYS_UI_LBL_OT_ITEM_MANUAL); break;
            case OT_MENU_ITEM_CLEANUP: item_label = GetString(SYS_UI_LBL_OT_ITEM_CLEANUP); break;
            case OT_MENU_ITEM_RETURN:  item_label = GetString(SYS_UI_LBL_EXIT); break; // Using generic EXIT label
            default: item_label = "Error Label"; break; // Should not happen
        }

        if (i == selected_item) {
            sprintf(SPRINTF_BUFF, " > %s <\n", item_label);
        } else {
            sprintf(SPRINTF_BUFF, "   %s   \n", item_label);
        }
        printf("%s", SPRINTF_BUFF);
    }
    printf("===============================\n");
    sprintf(SPRINTF_BUFF, "UP/DOWN: Navigate | %s: Select | %s: Back\n", 
           (SelectButton == PAD_CROSS ? "CROSS" : "CIRCLE"), 
           (CancelButton == PAD_TRIANGLE ? "TRIANGLE" : (CancelButton == PAD_CIRCLE ? "CIRCLE" : "UnknownCancel")));
    printf("%s", SPRINTF_BUFF);
    // printf("Debug: Selected Item Index: %d\n", selected_item); 
}

static enum OpenTunaPayloadType ShowManualPayloadSelectionMenu(void) {
    int manual_selected_item = 0; 
    const int manual_label_ids[] = { 
        SYS_UI_LBL_OT_PAYLOAD_SLIMS,
        SYS_UI_LBL_OT_PAYLOAD_FATS,
        SYS_UI_LBL_OT_PAYLOAD_FAT170,
        SYS_UI_LBL_CANCEL // Using generic CANCEL label
    };
    int num_manual_options = sizeof(manual_label_ids) / sizeof(manual_label_ids[0]);
    u32 pad_data_old_manual = 0, pad_data_cur_manual = 0, pad_data_tap_manual = 0;
    char SPRINTF_BUFF[128];

    DisplayMessageTimed(GetString(SYS_UI_LBL_OT_MANUAL_SELECT_TITLE), 1000, SYS_MSG_INFO); 

    while(1) {
        pad_data_cur_manual = ReadCombinedPadStatus_raw();
        pad_data_tap_manual = pad_data_cur_manual & ~pad_data_old_manual;
        pad_data_old_manual = pad_data_cur_manual;

        sprintf(SPRINTF_BUFF, "\n\n--- %s ---\n", GetString(SYS_UI_LBL_OT_MANUAL_SELECT_TITLE)); 
        printf("%s", SPRINTF_BUFF);
        for(int i=0; i<num_manual_options; ++i) {
            if (i == manual_selected_item) sprintf(SPRINTF_BUFF, " > %s <\n", GetString(manual_label_ids[i])); 
            else sprintf(SPRINTF_BUFF, "   %s   \n", GetString(manual_label_ids[i]));
            printf("%s", SPRINTF_BUFF);
        }
        printf("------------------------------\n");
        // SyncFlipFB(&UIDrawGlobal); 
        // DelayThread(16666); 

        if (pad_data_tap_manual & PAD_DOWN) {
            manual_selected_item = (manual_selected_item + 1) % num_manual_options;
        } else if (pad_data_tap_manual & PAD_UP) {
            manual_selected_item = (manual_selected_item - 1 + num_manual_options) % num_manual_options;
        } else if (pad_data_tap_manual & SelectButton) {
            if (manual_selected_item == 0) return OT_TYPE_SLIMS;    // Corresponds to SYS_UI_LBL_OT_PAYLOAD_SLIMS
            if (manual_selected_item == 1) return OT_TYPE_FATS;     // Corresponds to SYS_UI_LBL_OT_PAYLOAD_FATS
            if (manual_selected_item == 2) return OT_TYPE_FAT170;   // Corresponds to SYS_UI_LBL_OT_PAYLOAD_FAT170
            if (manual_selected_item == 3) return OT_TYPE_UNSUPPORTED; // Corresponds to SYS_UI_LBL_CANCEL
        } else if (pad_data_tap_manual & CancelButton) {
            return OT_TYPE_UNSUPPORTED; // Cancel
        }
    }
}

static enum OpenTunaPayloadType get_opentuna_payload_type(void) {
    FILE *fd_romver;
    char romver_buffer[16]; 
    unsigned long int rom_version_ulong = 0;

    printf("get_opentuna_payload_type: Attempting to read rom0:ROMVER...\n");

    fd_romver = fopen("rom0:ROMVER", "r"); 
    if (fd_romver != NULL) {
        size_t bytes_read = fread(romver_buffer, 1, 4, fd_romver); 
        fclose(fd_romver);

        if (bytes_read == 4) {
            romver_buffer[4] = '\0'; 
            char *endptr;
            rom_version_ulong = strtoul(romver_buffer, &endptr, 16); 

            if (*endptr == '\0' && rom_version_ulong != 0) { 
                printf("Successfully read and parsed ROMVER: %s -> 0x%04lX\n", romver_buffer, rom_version_ulong);
            } else {
                printf("Error: Failed to convert ROMVER string '%s' to unsigned long, or result was zero. endptr: '%c'\n", romver_buffer, *endptr);
                rom_version_ulong = 0; 
            }
        } else {
            printf("Error: Could not read 4 bytes from rom0:ROMVER (read %zu bytes).\n", bytes_read);
            rom_version_ulong = 0; 
        }
    } else {
        printf("Error: Failed to open rom0:ROMVER (fopen returned NULL, errno: %d).\n", errno);
        return OT_TYPE_UNSUPPORTED;
    }

    if (rom_version_ulong == 0) { 
        printf("ROM version could not be determined or was zero. Cannot select payload type.\n");
        return OT_TYPE_UNSUPPORTED;
    }

    if (rom_version_ulong >= 0x190) {
        printf("ROM 0x%04lX categorized as SLIMS.\n", rom_version_ulong);
        return OT_TYPE_SLIMS;
    }
    if (rom_version_ulong == 0x170) { 
        printf("ROM 0x%04lX categorized as FAT170.\n", rom_version_ulong);
        return OT_TYPE_FAT170;
    }
    if ((rom_version_ulong >= 0x110) && (rom_version_ulong < 0x190)) { 
        printf("ROM 0x%04lX categorized as FATS.\n", rom_version_ulong);
        return OT_TYPE_FATS;
    }
    
    printf("ROM 0x%04lX is UNSUPPORTED by defined categories.\n", rom_version_ulong);
    return OT_TYPE_UNSUPPORTED;
}

static int opentuna_delete_existing_folders(int mc_port, int mc_slot) {
    int overall_result = 0; 
    int op_result;
    char mc_path[MAX_PATH]; 

    const char* folders_to_delete[] = {
        "FUNTUNA", "FUNTUNA-FORK", "BXEXEC-FUNTUNA", "BXEXEC-OPENTUNA",
        "FORTUNA", OPENTUNA_DIR_NAME, "DST_OPENTUNA-INSTALLER", "DST_FMCT-INSTALLER",
        NULL 
    };

    (void)mc_slot; 
    printf("opentuna_delete_existing_folders: Starting cleanup on mc%d:/\n", mc_port);

    for (int i = 0; folders_to_delete[i] != NULL; ++i) {
        sprintf(mc_path, "mc%d:/%s", mc_port, folders_to_delete[i]);
        printf("Attempting to delete folder: %s\n", mc_path);
        
        op_result = DeleteFolder(mc_path); 
        
        if (op_result < 0 && op_result != -ENOENT && op_result != -SCE_EENET_ENOENT) { 
            printf("Warning: Failed to delete %s (result: %d)\n", mc_path, op_result);
            overall_result = op_result; 
        } else if (op_result == -ENOENT || op_result == -SCE_EENET_ENOENT) {
            printf("Folder not found: %s\n", mc_path);
        } else if (op_result == 0) { 
            printf("Deleted successfully: %s\n", mc_path);
        } else { 
             printf("Deletion of %s returned an unexpected code: %d\n", mc_path, op_result);
             overall_result = op_result; 
        }
    }
    return overall_result; 
}

static int opentuna_perform_install_core(int mc_port, int mc_slot, enum OpenTunaPayloadType payload_type) {
    char installer_root_path[MAX_PATH];
    char install_assets_path[MAX_PATH]; 
    char source_file_path[MAX_PATH];
    char mc_dest_dir_path[MAX_PATH];    
    char mc_dest_file_path[MAX_PATH];
    char mc_manifest_path[MAX_PATH];
    const char* selected_payload_source_filename;
    int fd_manifest = -1;
    int result = 0;

    // mc_slot is used by createFolder.
    // It's not directly used by getcwd or CopyFiles if paths are full device paths like "mass:/..."
    // but it's good practice to pass it along if underlying mcio functions might need it implicitly.

    if (getcwd(installer_root_path, sizeof(installer_root_path)) == NULL) {
        printf("Error: opentuna_perform_install_core - getcwd failed (errno: %d).\n", errno);
        return -1; 
    }
    if (installer_root_path[strlen(installer_root_path) - 1] != '/') {
        strcat(installer_root_path, "/");
    }
    sprintf(install_assets_path, "%s%s", installer_root_path, OPENTUNA_INSTALL_SUBDIR);
    printf("Installer assets path: %s\n", install_assets_path);

    sprintf(mc_dest_dir_path, "mc%d:/%s", mc_port, OPENTUNA_DIR_NAME);
    printf("Ensuring directory: %s (using port %d, slot %d for createFolder)\n", mc_dest_dir_path, mc_port, mc_slot);
    result = createFolder(mc_port, mc_slot, OPENTUNA_DIR_NAME); 
    if (result < 0 && result != -EEXIST && result != -SCE_EENET_EEXIST ) { 
        printf("Error creating directory %s on mc%d (result: %d)\n", OPENTUNA_DIR_NAME, mc_port, result);
        return result; 
    }
    printf("Directory %s ensured on mc%d.\n", mc_dest_dir_path, mc_port);

    if (payload_type >= OT_TYPE_UNSUPPORTED || g_OpenTunaPayloadSourceFilenames[payload_type] == NULL) {
        printf("Error: Invalid or unsupported payload_type: %d\n", payload_type);
        return -1; 
    }
    selected_payload_source_filename = g_OpenTunaPayloadSourceFilenames[payload_type];
    
    sprintf(source_file_path, "%s%s", install_assets_path, selected_payload_source_filename);
    sprintf(mc_dest_file_path, "%s/%s", mc_dest_dir_path, OPENTUNA_DEST_ICON_ICN_FILENAME);
    printf("Copying payload: %s -> %s\n", source_file_path, mc_dest_file_path);
    result = CopyFiles(source_file_path, mc_dest_file_path, NULL, 0, FILE_TYPE_FILE); 
    if (result != 0) { 
        printf("Error copying payload %s to %s (result: %d)\n", source_file_path, mc_dest_file_path, result);
        return result; 
    }
    printf("Payload '%s' copied successfully as '%s'.\n", selected_payload_source_filename, OPENTUNA_DEST_ICON_ICN_FILENAME);

    sprintf(source_file_path, "%s%s", install_assets_path, OPENTUNA_ICON_SYS_SOURCE_FILENAME);
    sprintf(mc_dest_file_path, "%s/%s", mc_dest_dir_path, OPENTUNA_DEST_ICON_SYS_FILENAME);
    printf("Copying system icon: %s -> %s\n", source_file_path, mc_dest_file_path);
    result = CopyFiles(source_file_path, mc_dest_file_path, NULL, 0, FILE_TYPE_FILE);
    if (result != 0) {
        printf("Error copying system icon %s to %s (result: %d)\n", source_file_path, mc_dest_file_path, result);
        return result; 
    }
    printf("System icon '%s' copied successfully.\n", OPENTUNA_ICON_SYS_SOURCE_FILENAME);

    if (payload_type >= OT_TYPE_UNSUPPORTED || g_OpenTunaManifestFileNames[payload_type] == NULL) {
         printf("Error: Invalid payload_type for manifest filename: %d\n", payload_type);
        return -1;
    }
    sprintf(mc_manifest_path, "%s/icon_%s.cnf", mc_dest_dir_path, g_OpenTunaManifestFileNames[payload_type]);
    printf("Creating manifest file: %s with content: \"%s\"\n", mc_manifest_path, g_OpenTunaTypeAliases[payload_type]);
    
    fd_manifest = open(mc_manifest_path, O_CREAT | O_WRONLY | O_TRUNC); 
    if (fd_manifest < 0) {
        printf("Error creating manifest file %s (fd: %d, errno: %d)\n", mc_manifest_path, fd_manifest, errno); 
        return fd_manifest; 
    }
    
    size_t alias_len = strlen(g_OpenTunaTypeAliases[payload_type]);
    if (write(fd_manifest, g_OpenTunaTypeAliases[payload_type], alias_len) != (int)alias_len) { 
        printf("Error writing to manifest file %s (errno: %d)\n", mc_manifest_path, errno);
        close(fd_manifest);
        return -1; 
    }
    close(fd_manifest);
    printf("Manifest file created successfully.\n");

    return 0; 
}

static int opentuna_apply_timestamp(int mc_port, int mc_slot) {
    char mc_opentuna_dir_for_timestamp[MAX_PATH]; 
    sceMcStDateTime timestamp_2099;
    sceMcTblGetDir file_info_table_entry; 
    int result_setinfo, result_sync;

    sprintf(mc_opentuna_dir_for_timestamp, "%s", OPENTUNA_DIR_NAME); 

    timestamp_2099.Resv2 = 0; 
    timestamp_2099.Sec = 59;
    timestamp_2099.Min = 59;
    timestamp_2099.Hour = 23;
    timestamp_2099.Day = 31;
    timestamp_2099.Month = 12;
    timestamp_2099.Year = 2099;

    memset(&file_info_table_entry, 0, sizeof(sceMcTblGetDir)); 
    file_info_table_entry._Create = timestamp_2099;
    file_info_table_entry._Modify = timestamp_2099;
    
    printf("Setting timestamp for mc%d:/%s (slot %d) to 2099.\n", mc_port, mc_opentuna_dir_for_timestamp, mc_slot);
    
    result_setinfo = mcSetFileInfo(mc_port, mc_slot, mc_opentuna_dir_for_timestamp, &file_info_table_entry, sceMcFileInfoModify); 
    
    if (result_setinfo == sceMcResSucceed) {
        result_sync = sceMcResFail; 
        mcSync(0, NULL, &result_sync); 
        if (result_sync == sceMcResSucceed) {
            printf("Timestamp set successfully for mc%d:/%s.\n", mc_port, mc_opentuna_dir_for_timestamp);
            return 0; 
        } else {
            printf("Error during mcSync after mcSetFileInfo for mc%d:/%s (mcSync result: %d)\n", mc_port, mc_opentuna_dir_for_timestamp, result_sync);
            return result_sync; 
        }
    }
    
    printf("Error setting timestamp for mc%d:/%s (mcSetFileInfo result: %d)\n", mc_port, mc_opentuna_dir_for_timestamp, result_setinfo);
    return result_setinfo; 
}
