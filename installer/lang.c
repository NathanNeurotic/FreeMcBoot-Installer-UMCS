// lang.c
// Default English string tables for FMCB Installer.
// Ensure this file is consistent with the enums in lang.h (Canvas artifact lang_h_final_with_desc_plus_ot_desc).

// If your lang.c has #include directives or other definitions at the top, they should remain.
// The functions (GetString, GetUILabel, etc.) that are typically at the end of lang.c should also remain.

static const char *DefaultLanguageStringTable[SYS_UI_MSG_COUNT] = {
    // Block 1: Original "Description-like" Messages (Indices 0-11 from Canvas lang.h)
    /* SYS_UI_MSG_INSTALL_NORMAL_DESC = 0 */ "Install FreeMcBoot for this console.", // Example, original might differ
    /* SYS_UI_MSG_INSTALL_MULTI_DESC */      "Install FreeMcBoot for all consoles of the same region.",
    /* SYS_UI_MSG_CLEANUP_DESC */            "Uninstall existing FMCB/FHDB installations.",
    /* SYS_UI_MSG_CLEANUP_MULTI_DESC */      "Downgrade a multi-installation to a normal installation.",
    /* SYS_UI_MSG_FORMAT_DESC */             "Format the Memory Card, erasing all its contents.",
    /* SYS_UI_MSG_DUMP_MC_DESC */            "Dump Memory Card contents to a file on a mass storage device.",
    /* SYS_UI_MSG_RESTORE_MC_DESC */         "Restore Memory Card contents from a file on a mass storage device.",
    /* SYS_UI_MSG_INSTALL_FHDB_DESC */       "Install FreeMcBoot onto the Harddisk Drive (FHDB).",
    /* SYS_UI_MSG_CLEANUP_FHDB_DESC */       "Uninstall FreeMcBoot from the Harddisk Drive (FHDB).",
    /* SYS_UI_MSG_INSTALL_CROSS_PSX_DESC */  "Install FreeMcBoot (PSX/DESR) for this Memory Card.",
    /* SYS_UI_MSG_FORMAT_HDD_DESC */         "Format the Harddisk Drive and create basic partitions.",
    /* SYS_UI_MSG_EXIT_DESC */               "Exit the installer application.",

    // Block 2: Original "Prompt" Messages (Indices 12-19 from Canvas lang.h)
    /* SYS_UI_MSG_PROMPT_CONTINUE */         "Do you want to continue?", // Generic continue
    /* SYS_UI_MSG_PROMPT_INSTALL_NORMAL */   "Install FreeMcBoot to mc%d:/ Continue?",
    /* SYS_UI_MSG_PROMPT_INSTALL_MULTI */    "Install FreeMcBoot (Multi-Install) to mc%d:/ Continue?",
    /* SYS_UI_MSG_PROMPT_CLEANUP */          "Uninstall FMCB from mc%d:/ Continue?",
    /* SYS_UI_MSG_PROMPT_CLEANUP_MULTI */    "Downgrade Multi-Install on mc%d:/ Continue?",
    /* SYS_UI_MSG_PROMPT_INSTALL_FHDB */     "Install FHDB to hdd0:/ Continue?",
    /* SYS_UI_MSG_PROMPT_CLEANUP_FHDB */     "Uninstall FHDB from hdd0:/ Continue?",
    /* SYS_UI_MSG_PROMPT_INSTALL_CROSS_PSX */"Install FreeMcBoot (PSX) to mc%d:/ Continue?",

    // Block 3: Original "Installing" Messages (Indices 20-25 from Canvas lang.h)
    /* SYS_UI_MSG_INSTALLING_NORMAL */       "Installing FreeMcBoot (Normal)...",
    /* SYS_UI_MSG_INSTALLING_MULTI */        "Installing FreeMcBoot (Multi-Install)...",
    /* SYS_UI_MSG_INSTALLING_FHDB */         "Installing FreeMcBoot (HDD)...",
    /* SYS_UI_MSG_INSTALLING_CROSS_PSX */    "Installing FreeMcBoot (PSX)...",
    /* SYS_UI_MSG_INSTALL_COMPLETED */       "Installation completed successfully.",
    /* SYS_UI_MSG_INSTALL_FAILED */          "Installation failed.",

    // Block 4: Original "Cleaning up" Messages (Indices 26-28 from Canvas lang.h)
    /* SYS_UI_MSG_CLEANING_UP */             "Cleaning up installation...",
    /* SYS_UI_MSG_CLEANUP_FAILED */          "Cleanup failed.",
    /* SYS_UI_MSG_CLEANUP_COMPLETED */       "Cleanup completed successfully.",

    // Block 5: Original "Formatting" Messages (Indices 29-32 from Canvas lang.h)
    /* SYS_UI_MSG_FORMATTING_MC */           "Formatting Memory Card on mc%d:/...",
    /* SYS_UI_MSG_FORMATTING_HDD */          "Formatting Harddisk Drive...",
    /* SYS_UI_MSG_FORMAT_COMPLETED */        "Format completed successfully.",
    /* SYS_UI_MSG_FORMAT_FAILED */           "Format failed.",

    // Block 6: Original "Dumping" Messages (Indices 33-35 from Canvas lang.h)
    /* SYS_UI_MSG_DUMPING_MC */              "Dumping Memory Card mc%d:/...",
    /* SYS_UI_MSG_DUMPING_COMPLETED */       "Memory Card dump completed.",
    /* SYS_UI_MSG_DUMPING_FAILED */          "Memory Card dump failed.",

    // Block 7: Original "Restoring" Messages (Indices 36-38 from Canvas lang.h)
    /* SYS_UI_MSG_RESTORING_MC */            "Restoring Memory Card mc%d:/...",
    /* SYS_UI_MSG_RESTORING_COMPLETED */     "Memory Card restore completed.",
    /* SYS_UI_MSG_RESTORING_FAILED */        "Memory Card restore failed.",

    // Block 8: Original "Status/Error" Messages (Indices 39-53 from Canvas lang.h)
    /* SYS_UI_MSG_HAS_MULTI_INST */          "Multi-installation detected! Please uninstall first.",
    /* SYS_UI_MSG_MULTI_INST_REQ */          "Multi-installation required, but not found on card.",
    /* SYS_UI_MSG_NO_MC */                   "No PlayStation 2 Memory Card detected.",
    /* SYS_UI_MSG_MC_INVALID_FMT */          "Memory Card has an invalid format.",
    /* SYS_UI_MSG_NO_SPACE */                "Insufficient free space on Memory Card.",
    /* SYS_UI_MSG_NO_SPACE_HDD */            "Insufficient free space on the Harddisk Drive.",
    /* SYS_UI_MSG_NO_SPACE_HDD_APPS */       "Insufficient free space in the APPS partition on HDD.",
    /* SYS_UI_MSG_LOADING_LANG */            "Loading language files...",
    /* SYS_UI_MSG_UNSUPP_CONSOLE */          "This console model does not support System Updates (FHDB).",
    /* SYS_UI_MSG_NO_HDD */                  "No Harddisk Drive (HDD) unit detected or unit is not ready.",
    /* SYS_UI_MSG_HDD_CORRUPTED */           "The Harddisk Drive (HDD) unit appears to be corrupted.",
    /* SYS_UI_MSG_PLEASE_WAIT */             "Please wait...",
    /* SYS_UI_MSG_QUIT_DUMPING */            "Stop dumping Memory Card?",
    /* SYS_UI_MSG_QUIT_RESTORING */          "Stop restoring Memory Card?",
    /* Placeholder for 15th string in this block if lang.h had one */ "FMCB_STRING_MISSING_ID_53", // Corresponds to original index 53

    // Block 9: Original "DSC_" Messages (Indices 54-65 from Canvas lang.h, assuming they start after QUIT_RESTORING)
    /* SYS_UI_MSG_DSC_INST_FMCB */           "Install FreeMcBoot for this specific PS2 console.",
    /* SYS_UI_MSG_DSC_MI_FMCB */             "Install FreeMcBoot (Multi-Install) compatible with all PS2s of the same region.",
    /* SYS_UI_MSG_DSC_UINST_FMCB */          "Uninstall FreeMcBoot from the selected Memory Card.",
    /* SYS_UI_MSG_DSC_DOWNGRADE_MI */        "Convert a Multi-Installation to a normal installation for this PS2.",
    /* SYS_UI_MSG_DSC_FORMAT_MC */           "Format the selected Memory Card. All data will be lost!",
    /* SYS_UI_MSG_DSC_DUMP_MC */             "Create a backup image of the Memory Card to a USB device.",
    /* SYS_UI_MSG_DSC_REST_MC */             "Restore a Memory Card image from a USB device.",
    /* SYS_UI_MSG_DSC_INST_CROSS_PSX */      "Install FreeMcBoot for PSX (DESR) consoles.",
    /* SYS_UI_MSG_DSC_INST_FHDB */           "Install FreeMcBoot to the Harddisk Drive.",
    /* SYS_UI_MSG_DSC_UINST_FHDB */          "Uninstall FreeMcBoot from the Harddisk Drive.",
    /* SYS_UI_MSG_DSC_FORMAT_HDD */          "Format the Harddisk Drive. All data will be lost!",
    /* SYS_UI_MSG_DSC_QUIT */                "Exit the FreeMcBoot Installer application.",

    // Block 10: New OpenTuna Installer Messages (Indices follow DSC_QUIT)
    /* SYS_UI_MSG_OT_AUTO_START */           "Starting Auto OpenTuna Install...",
    /* SYS_UI_MSG_OT_MANUAL_SELECT_PAYLOAD */"Manual Install: Select Payload Type",
    /* SYS_UI_MSG_OT_CLEANUP_START */        "Starting OpenTuna & Forks Cleanup...",
    /* SYS_UI_MSG_OT_ERR_UNSUPPORTED_MODEL */"Error: Unsupported PS2 model for OpenTuna.",
    /* SYS_UI_MSG_OT_WARN_CLEANUP_ISSUES */  "Notice: Problem during cleanup. Install may still proceed.",
    /* SYS_UI_MSG_OT_ERR_INSTALL_FAILED */   "Error: OpenTuna installation failed.",
    /* SYS_UI_MSG_OT_WARN_TIMESTAMP_FAILED */"Warning: Failed to set timestamp for OpenTuna folder.",
    /* SYS_UI_MSG_OT_SUCCESS_AUTO */         "OpenTuna Auto-Install Successful!",
    /* SYS_UI_MSG_OT_SUCCESS_MANUAL */       "OpenTuna Manual Install Successful!",
    /* SYS_UI_MSG_OT_MANUAL_CANCELLED */     "Manual Install Cancelled.",
    /* SYS_UI_MSG_OT_CLEANUP_SUCCESS */      "Cleanup Successful! (Or no targeted folders found).",
    /* SYS_UI_MSG_OT_AUTO_CONFIRM */         "Install OpenTuna (Auto) to mc%d:/OPENTUNA?",
    /* SYS_UI_MSG_OT_MANUAL_CONFIRM */       "Install OpenTuna (%s type) to mc%d:/OPENTUNA?",
    /* SYS_UI_MSG_OT_CLEANUP_CONFIRM */      "Cleanup OpenTuna and forks from mc%d:/ ?",

    // Block 11: New OpenTuna Description String
    /* SYS_UI_MSG_DSC_OPENTUNA_INSTALLER */  "Install the OpenTuna exploit for Memory Card booting.",

    // Block 12: Original FMCB Installer messages that followed descriptions in Canvas lang.h
    /* SYS_UI_MSG_MULTI_WARN */              "While the multi-installation can boot on all PlayStation 2 consoles with the least space requirements, it goes against the design of the Memory Card by introducing controlled filesystem corruption.\n\nIt is hence strongly recommended to create a normal installation instead.",
    /* SYS_UI_MSG_QUIT */                    "Quit program?",
    /* SYS_UI_MSG_FORMAT_HDD_MANUAL */       "Format the HardDisk Drive (HDD) unit?\nWarning: All data will be erased.",
    /* SYS_UI_MSG_FORMAT_HDD_FAILED */       "Failed to format the HardDisk Drive (HDD) unit.",
    /* SYS_UI_MSG_RARE_ROMVER */             "The installer has detected that your console is a very rare unit\nIf you want to colaborate with the homebrew PS2 comunity please contact me: \n\nhttps://github.com/israpps",
    /* SYS_UI_MSG_ROM_UNSUPPORTED */         "This console does not support System Updates.\n\nHowever, you can still make installations with it.",
    /* SYS_UI_MSG_CNF_FOUND */               "Existing FMCB CNF found!\nDo you want to keep it? Yes Recommended.",
    /* SYS_UI_MSG_CNF_HDD_FOUND */           "Existing FHDB CNF found!\nDo you want to keep it? Yes Recommended.",
    /* SYS_UI_MSG_MULTIPLE_CARDS */          "Memory Cards were detected in both slot 1 and 2.\n\nSelect the target memory card.",
    /* SYS_UI_MSG_INST_CFM_SLOT1 */          "PS2BBL will be installed onto the memory card in slot 1. Continue?",
    /* SYS_UI_MSG_INST_CFM_SLOT2 */          "PS2BBL will be installed onto the memory card in slot 2. Continue?",
    /* SYS_UI_MSG_INST_CFM_HDD */            "PS2BBL will be installed onto the Harddisk Drive. Continue?",
    /* SYS_UI_MSG_INST_PROMPT_INST_TYPE */   "Install PS2BBL for:",
    /* SYS_UI_MSG_CLNUP_CFM_SLOT1 */         "System Update on the memory card in slot 1 will be cleaned up. Continue?",
    /* SYS_UI_MSG_CLNUP_CFM_SLOT2 */         "System Update on the memory card in slot 2 will be cleaned up. Continue?",
    /* SYS_UI_MSG_CLNUP_CFM_HDD */           "System Update on the Harddisk Drive will be cleaned up. Continue?",
    /* SYS_UI_MSG_FORMAT_CFM_SLOT1 */        "The memory card in slot 1 will be formatted.\n\nContinue?",
    /* SYS_UI_MSG_FORMAT_CFM_SLOT2 */        "The memory card in slot 2 will be formatted.\n\nContinue?",
    /* SYS_UI_MSG_DUMP_CFM_SLOT1 */          "The memory card in slot 1 will be dumped.\n\nContinue?",
    /* SYS_UI_MSG_DUMP_CFM_SLOT2 */          "The memory card in slot 2 will be dumped.\n\nContinue?",
    /* SYS_UI_MSG_RESTORE_CFM_SLOT1 */       "The memory card in slot 1 will be restored.\n\nContinue?",
    /* SYS_UI_MSG_RESTORE_CFM_SLOT2 */       "The memory card in slot 2 will be restored.\n\nContinue?",
    /* SYS_UI_MSG_UNINSTALLING_FMCB */       "Uninstalling System Update.\n\nPlease do not switch off the power or remove the card.",
    /* SYS_UI_MSG_UNINSTALLING_FHDB */       "Uninstalling System Update.\n\nPlease do not switch off the power or disconnect the HDD unit.",
    /* SYS_UI_MSG_NO_ENT_ERROR */            "File not found.\nPlease check your installation media.",
    /* SYS_UI_MSG_READ_INST_ERROR */         "Read error.\nPlease check your installation media.",
    /* SYS_UI_MSG_WRITE_INST_ERROR */        "Write error.\nPlease check your installation media.",
    /* SYS_UI_MSG_READ_MC_ERROR */           "Read error.\nPlease check your memory card.",
    /* SYS_UI_MSG_WRITE_MC_ERROR */          "Write error.\nPlease check your memory card.",
    /* SYS_UI_MSG_WRITE_HDD_ERROR */         "Write error.\nPlease check your Harddisk Drive.",
    /* SYS_UI_MSG_NO_MEM_ERROR */            "Out of memory.",
    /* SYS_UI_MSG_CACHE_INIT_ERROR */        "Cache initialization failed.",
    /* SYS_UI_MSG_CROSSLINK_FAIL */          "Failed to create crosslinked files.",
    /* SYS_UI_MSG_MG_BIND_FAIL */            "Failed to bind MagicGate file to Memory Card.",
    /* SYS_UI_MSG_UNSUPPORTED_UINST_FILE_ERROR */ "Can't remove multi-installation made by an incompatible installer.",
    /* SYS_UI_MSG_HDD_SMART_FAILED */        "S.M.A.R.T. has reported that the HardDisk Drive (HDD) unit has failed.\n\nThe HDD unit must be replaced.",
    /* SYS_UI_MSG_INSUF_FREE_SPC */          "Insufficient free space on card.",
    /* SYS_UI_MSG_INSUF_FREE_SPC_HDD_APPS */ "Insufficient free space on in the APPS partition.",
    /* SYS_UI_MSG_INSUF_FREE_SPC_HDD */      "Insufficient free space on the HardDisk Drive."
};

static const char *DefaultLanguageLabelStringTable[SYS_UI_LBL_COUNT] = {
    // Original FMCB Labels (Indices 0-9 from Canvas lang.h)
    /* SYS_UI_LBL_OK = 0 */ "OK",
    /* SYS_UI_LBL_CANCEL */ "Cancel",
    /* SYS_UI_LBL_YES */    "Yes",
    /* SYS_UI_LBL_NO */     "No",
    /* SYS_UI_LBL_WARNING */"Warning", 
    /* SYS_UI_LBL_ERROR */  "Error",   
    /* SYS_UI_LBL_INFO */   "Information",    
    /* SYS_UI_LBL_CONFIRM */"Confirmation", 
    /* SYS_UI_LBL_WAIT */   "Please wait",    
    /* SYS_UI_LBL_NOTICE */ "Notice",  
	
    // Original "Standard FMCB Labels" (Indices 10-36 from Canvas lang.h)
	/* SYS_UI_LBL_SLOT1 */                 "Slot 1",
	/* SYS_UI_LBL_SLOT2 */                 "Slot 2",
	/* SYS_UI_LBL_MEMORY_CARD */           "Memory Card", 
	/* SYS_UI_LBL_HDD */                   "HDD",         
	/* SYS_UI_LBL_INSTALLING */            "Now installing...",
	/* SYS_UI_LBL_DUMPING_MC_PROG */       "Now dumping...",  
	/* SYS_UI_LBL_RESTORING_MC_PROG */     "Now restoring...",
	/* SYS_UI_LBL_INSTALL_THIS_PS2 */      "This PS2 and similar units", 
	/* SYS_UI_LBL_INSTALL_SAME_REGION */   "Every PS2 of the same Region",
	/* SYS_UI_LBL_INSTALL_ALL_PS2 */       "Every PS2",    
	/* SYS_UI_LBL_RATE */                  "Rate:",
	/* SYS_UI_LBL_ETA */                   "Time\nremaining:",                
	/* SYS_UI_LBL_INSTALL */               "Install OSDMenu", // Note: Original lang.c had "Install OSDMenu" for SYS_UI_LBL_INSTALL
	/* SYS_UI_LBL_MI */                    "Install OSDMenu with LaunchKeys", // Note: Original lang.c had this for SYS_UI_LBL_MI
	/* SYS_UI_LBL_UINSTALL */              "Uninstall Existing System Updates",           
	/* SYS_UI_LBL_UMI */                   "Uninstall Multi-Install",                
	/* SYS_UI_LBL_FORMAT_MC */             "Format MemoryCard",          
	/* SYS_UI_LBL_DUMP_MC */               "Dump MemoryCard",            
	/* SYS_UI_LBL_REST_MC */               "Restore MemoryCard",            
	/* SYS_UI_LBL_INSTALL_CROSS_PSX */     "Install PS2BBL (PSX/DESR)",  
	/* SYS_UI_LBL_INSTALL_FHDB */          "Install PS2BBL-HDD",       
	/* SYS_UI_LBL_UINSTALL_FHDB */         "Uninstall Existing System Updates", //This was a duplicate string in fetched lang.c      
	/* SYS_UI_LBL_EXIT */                  "Exit",               
	/* SYS_UI_LBL_B */                     "B",
	/* SYS_UI_LBL_KB */                    "KB",
	/* SYS_UI_LBL_MB */                    "MB",
	/* SYS_UI_LBL_GB */                    "GB",
	/* SYS_UI_LBL_TB */                    "TB", // Index 36

    // Original "BPS" labels etc. (Indices 37-49 from Canvas lang.h)
	/* SYS_UI_LBL_BPS */                   "B/s",  
	/* SYS_UI_LBL_KBPS */                  "KB/s", 
	/* SYS_UI_LBL_MBPS */                  "MB/s", 
	/* SYS_UI_LBL_AVAILABLE_SPC */         "Available space:",
	/* SYS_UI_LBL_REQUIRED_SPC */          "Required space:",
	/* SYS_UI_LBL_MENU_MAIN */             "Main Menu",
	/* SYS_UI_LBL_MENU_EXTRAS */           "Extras Menu",
	/* SYS_UI_LBL_MENU_MC */               "Memory Card Menu",
	/* SYS_UI_LBL_FORMAT_HDD */            "Format HDD",         
	/* SYS_UI_LBL_ENABLED */               "Enabled",            
	/* SYS_UI_LBL_DISABLED */              "Disabled",           
	/* SYS_UI_LBL_INST_TYPE_NORMAL */      "Normal",   
	/* SYS_UI_LBL_INST_TYPE_CRS_MDL */     "Cross-Model",  
	/* SYS_UI_LBL_INST_TYPE_CRS_REG */     "Cross-Region",  // Index 49

    // New OpenTuna Installer Labels (Indices 50-57 from Canvas lang.h)
    /* SYS_UI_LBL_OT_MENU_TITLE */          "OpenTuna Installer",
    /* SYS_UI_LBL_OT_ITEM_AUTO */           "Auto Install OpenTuna",
    /* SYS_UI_LBL_OT_ITEM_MANUAL */         "Manual Install OpenTuna",
    /* SYS_UI_LBL_OT_ITEM_CLEANUP */        "Cleanup OpenTuna/Forks",
    /* SYS_UI_LBL_OT_MANUAL_SELECT_TITLE */ "Manual Install: Select Payload Type",
    /* SYS_UI_LBL_OT_PAYLOAD_SLIMS */       "Install for SLIMS / newer FATS",
    /* SYS_UI_LBL_OT_PAYLOAD_FATS */        "Install for older FATS",
    /* SYS_UI_LBL_OT_PAYLOAD_FAT170 */      "Install for FAT (ROM 0170 only)"
    // Ensure SYS_UI_LBL_COUNT in lang.h reflects this new total.
};

// The functions like GetString(), GetUILabel(), LoadLanguageStrings(), etc.,
// that are part of your original lang.c file should remain below these two arrays.
// (These functions are NOT part of this update, only the two arrays above are modified)
