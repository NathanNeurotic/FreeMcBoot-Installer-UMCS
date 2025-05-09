#include <iopheap.h>
#include <kernel.h>
#include <libcdvd.h>    // For sceCd* functions if used (e.g. in system.c)
#include <libmc.h>      // For mc* functions (e.g. in system.c)
#include <fileXio_rpc.h>// For fileXioDevctl, etc. (used in system.c)
#include <hdd-ioctl.h>  // For HDDCTL_DEV9_SHUTDOWN (used here)
#include <loadfile.h>   // For LoadExecPS2, SifLoadModule (if used directly, often in iop.c)
#include <malloc.h>     // For memalign (used in system.c)
#include <sbv_patches.h>// For sbv_patch* (used in iop.c)
#include <sifcmd.h>     // For SifInitRpc, SifExitRpc, etc.
#include <sifrpc.h>
#include <stdio.h>      // For sprintf, printf (if DEBUG_PRINTF uses sio_printf, this might not be strictly needed for that)
#include <string.h>     // For strcpy, strcmp, etc.
#include <wchar.h>      // If any wide char functions were used by UI/Font (not directly in main.c usually)

#include <libgs.h>      // For GS_BGCOLOUR and graphics init/deinit if used directly here

// FMCB Project Specific Includes
#include "main.h"       // Contains MainMenuEvents enum, extern g_Port/g_Slot, FMCB_INSTALLER_VERSION etc.
#include "iop.h"        // For IopInitStart, IopDeinit, IOP_MOD_SET_MAIN
#include "pad.h"        // For PadInitPads, PadDeinitPads (if not handled in UI)
#include "graphics.h"   // For graphics functions (RedrawLoadingScreen, etc.)
#include "font.h"       // For font handling (if used directly here)

// #include "libsecr.h"   // libsecr functions are usually wrapped in system.c
// #include "mctools_rpc.h"// MCTools RPC functions are usually wrapped in system.c
#include "system.h"     // For GetBootDeviceID, GetPs2Type, HDDCheckStatus, UpdateRegionalPaths, StartWorkerThread, StopWorkerThread, IsHDDBootingEnabled etc.
// #include "ReqSpaceCalc.h"// If used directly here
#include "UI.h"         // For InitializeUI, DeinitializeUI, DisplayErrorMessage, etc.
#include "menu.h"       // For MainMenu() and menu_opentuna_install() declarations

// --- Global Variable Definitions ---
// These are declared extern in main.h and used by other modules like menu_opentuna.c
int g_Port = 0;             // Default MC port (mc0)
int g_Slot = 0;             // Default MC slot (slot 0) - PS2 only has one slot per port controller
int IsHDDUnitConnected = 0; // Tracks HDD status

// Semaphores (defined here as they are fundamental to main's operation)
int VBlankStartSema;
int InstallLockSema; 

// --- Static Function Implementations ---

// VBlank Handler (from original main.c)
static int VBlankStartHandler(int cause)
{
    ee_sema_t sema_status; 
    iReferSemaStatus(VBlankStartSema, &sema_status);
    if (sema_status.count < sema_status.max_count) {
        iSignalSema(VBlankStartSema);
    }
    ExitHandler(); // From PS2SDK kernel.h
    return 0;
}

// Deinitialization services (from original main.c)
static void DeinitServices(void)
{
    DisableIntc(kINTC_VBLANK_START);
    RemoveIntcHandler(kINTC_VBLANK_START, 0); 
    DeleteSema(VBlankStartSema);
    DeleteSema(InstallLockSema);

    IopDeinit(); // From iop.c
    // PadDeinitPads(); // This is called within DeinitializeUI() in UI.c
}

// --- Main Application Entry Point ---
int main(int argc, char *argv[])
{
    int SystemType, InitSemaID, BootDeviceID; 
    int result_generic; 
    unsigned int FrameNum;
    ee_sema_t sema_config; 
    int current_event = MENU_EVENT_NONE; // Variable to hold event from MainMenu or submenus

    sio_init(38400, 0, 0, 0, 0); 
    DEBUG_PRINTF("FMCB Installer v%s starting...\n", FMCB_INSTALLER_VERSION);

    for (result_generic = 0; result_generic < argc; result_generic++) { 
        DEBUG_PRINTF("argv[%d] = %s\n", result_generic, argv[result_generic]); 
    }

    if ((BootDeviceID = GetBootDeviceID()) == BOOT_DEVICE_UNKNOWN) {
        sio_printf("Error: Boot device is not recognized or not supported. Exiting.\n");
        SifExitRpc(); 
        Exit(-1); 
    }
    DEBUG_PRINTF("Boot device ID: %d (%s)\n", BootDeviceID, (BootDeviceID == BOOT_DEVICE_MASS ? "MASS" : "Other"));

    InitSemaID = IopInitStart(IOP_MOD_SET_MAIN); 

    sema_config.init_count = 0;
    sema_config.max_count = 1;
    sema_config.attr = EA_THPRI; 
    sema_config.option = 0;
    VBlankStartSema = CreateSema(&sema_config);

    sema_config.init_count = 1; 
    sema_config.max_count = 1;
    InstallLockSema = CreateSema(&sema_config);

    AddIntcHandler(kINTC_VBLANK_START, VBlankStartHandler, 0);
    EnableIntc(kINTC_VBLANK_START);

    if (InitializeUI(0) != 0) { 
        sio_printf("Error: InitializeUI(0) failed. Exiting.\n");
        SifExitRpc(); 
        Exit(-1);
    }
    DEBUG_PRINTF("UI Initialized.\n");

    FrameNum = 0;
    do {
        RedrawLoadingScreen(FrameNum); 
        FrameNum++;
    } while (PollSema(InitSemaID) != InitSemaID); 
    DeleteSema(InitSemaID);
    DEBUG_PRINTF("IOP Modules loaded.\n");

    StartWorkerThread();    
    UpdateRegionalPaths();  

    SystemType = GetPs2Type(); 
    if (SystemType == PS2_SYSTEM_TYPE_PS2 || SystemType == PS2_SYSTEM_TYPE_DEX) {
        result_generic = HDDCheckStatus(); 
        if (result_generic >= 0 && result_generic <= 1) { 
            IsHDDUnitConnected = 1;
            DEBUG_PRINTF("HDD Unit Connected. Status: %d\n", result_generic);
        } else {
            IsHDDUnitConnected = 0; 
            DEBUG_PRINTF("HDD Unit not usable or not connected. Status: %d\n", result_generic);
        }
    }

    // Main application loop
    current_event = MENU_EVENT_RETURN_TO_MAIN; // Start by entering the main menu

    while(current_event != EVENT_EXIT) { 
        DEBUG_PRINTF("main.c: Top of event loop, current_event = %d\n", current_event);
        switch (current_event) {
            case MENU_EVENT_RETURN_TO_MAIN:
                DEBUG_PRINTF("main.c: Calling MainMenu()...\n");
                current_event = MainMenu(); // MainMenu now returns an int event
                DEBUG_PRINTF("main.c: MainMenu() returned event: %d\n", current_event);
                break;

            case MENU_EVENT_GO_TO_OPENTUNA_INSTALL:
                DEBUG_PRINTF("main.c: Calling menu_opentuna_install()...\n");
                // Parameters for menu_opentuna_install are (int connecting_event, int current_selection)
                // We pass the event that led here, and 0 for default selection in the OpenTuna menu.
                current_event = menu_opentuna_install(MENU_EVENT_GO_TO_OPENTUNA_INSTALL, 0); 
                DEBUG_PRINTF("main.c: menu_opentuna_install() returned event: %d\n", current_event);
                break;
            
            case MENU_EVENT_NONE: 
                DEBUG_PRINTF("main.c: Warning - MENU_EVENT_NONE received. Returning to MainMenu.\n");
                current_event = MENU_EVENT_RETURN_TO_MAIN;
                break;

            case EVENT_EXIT: 
                DEBUG_PRINTF("main.c: EVENT_EXIT received. Loop will terminate.\n");
                // The while loop condition (current_event != EVENT_EXIT) will handle actual termination.
                break;

            default:
                DEBUG_PRINTF("main.c: Unhandled event %d received. Defaulting to MainMenu.\n", current_event);
                current_event = MENU_EVENT_RETURN_TO_MAIN; 
                break;
        }
    } // End of while(current_event != EVENT_EXIT)

    // Shutdown sequence
    DEBUG_PRINTF("Exiting application (final event was %d)...\n", current_event);
    StopWorkerThread(); 

    if (IsHDDUnitConnected && !IsHDDBootingEnabled()) { 
         DEBUG_PRINTF("Shutting down HDD DEV9...\n");
         fileXioDevctl("hdd0:", HDDCTL_DEV9_SHUTDOWN, NULL, 0, NULL, 0);
    }

    DeinitializeUI(); 
    DeinitServices();   

    DEBUG_PRINTF("FMCB Installer terminated.\n");
    SifExitRpc(); 
    return 0;     
}
