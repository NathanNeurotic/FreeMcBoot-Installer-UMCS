#ifndef __MAIN_H__
#define __MAIN_H__

// FMCB Installer version will be defined by the Makefile via CFLAGS
// #define FMCB_INSTALLER_VERSION "1.001-MOD-OpenTuna" // REMOVED THIS LINE

/* Debugging TTY output - comment out to disable */
// #define DEBUG_TTY_FEEDBACK 
#ifdef DEBUG_TTY_FEEDBACK
#include <sio.h> // For sio_printf
#define DEBUG_PRINTF(args...) sio_printf(args)
#else
#define DEBUG_PRINTF(args...) (void)0
#endif

/* The number of files and folders to crosslink (For multi-regional and cross-model installations). */
#define NUM_CROSSLINKED_FILES 8 

/* Maximum path length */
#define MAX_PATH 256 

/* File types for CopyFiles function in system.c */
#define FILE_TYPE_FILE 0 // Assuming 0 for regular file
#define FILE_TYPE_DIR  1 // Assuming 1 for directory

/* Global variables for current MC port and slot - defined in main.c */
extern int g_Port;    // 0 for mc0, 1 for mc1
extern int g_Slot;    // Always 0 for PS2 MC slots

/* Global variables for pad buttons (confirm/cancel) - defined in UI.c */
extern unsigned short int SelectButton; 
extern unsigned short int CancelButton; 

/* Event numbers for menu navigation and actions */
enum MainMenuEvents {
    EVENT_INSTALL = 0,
    EVENT_MULTI_INSTALL,
    EVENT_CLEANUP,
    EVENT_CLEANUP_MULTI,
    EVENT_FORMAT,
    EVENT_DUMP_MC,
    EVENT_RESTORE_MC,
    EVENT_INSTALL_FHDB,
    EVENT_CLEANUP_FHDB,
    EVENT_INSTALL_CROSS_PSX,
    EVENT_FORMAT_HDD,
    
    MENU_EVENT_GO_TO_OPENTUNA_INSTALL, // Added new event for OpenTuna

    EVENT_EXIT,                     
    EVENT_OPTION_COUNT              /* This MUST be last! */
};

/* Menu return events (often shared across menus) */
#define MENU_EVENT_NONE             0xFFFF  
#define MENU_EVENT_RETURN_TO_MAIN   0xFFFE  

/* Memory card state flags */
#define MC_FLAG_CARD_HAS_MULTI_INST 0x01

struct McData
{
    int SpaceFree;
    int Type;
    int Format;
    unsigned char flags;
};

/* Operation mode prerequisite check parameters */
#define CHECK_MULTI_INSTALL           0x01
#define CHECK_MUST_HAVE_MULTI_INSTALL 0x02 
#define CHECK_FREE_SPACE              0x04

// extern struct UIDrawGlobal UIDrawGlobal; // Defined in UI.c, if needed globally by other .c files

#endif /* __MAIN_H__ */
