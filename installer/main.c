#include <iopheap.h>
#include <kernel.h>
#include <libcdvd.h>
#include <libmc.h>
#include <fileXio_rpc.h>
#include <hdd-ioctl.h>
#include <loadfile.h>
#include <malloc.h>
#include <sbv_patches.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <stdio.h> // For general C functions like sprintf, if not for sio_printf
#include <string.h>
#include <wchar.h>

#include <libgs.h> 
#include <sio.h>       // <<< ADDED: For sio_init, even if DEBUG_TTY_FEEDBACK is off in main.h for DEBUG_PRINTF

// FMCB Project Specific Includes
#include "main.h"      
#include "iop.h"       
#include "pad.h"       
#include "graphics.h"  
#include "font.h"      

#include "system.h"    
#include "UI.h"        
#include "menu.h"      

// --- Global Variable Definitions ---
int g_Port = 0;            
int g_Slot = 0;            
int IsHDDUnitConnected = 0; 

// Semaphores
int VBlankStartSema;
int InstallLockSema; 

// --- Static Function Implementations ---

static int VBlankStartHandler(int cause)
{
    ee_sema_t sema_status; 
    iReferSemaStatus(VBlankStartSema, &sema_status);
    if (sema_status.count < sema_status.max_count) {
        iSignalSema(VBlankStartSema);
    }
    ExitHandler(); 
    return 0;
}

static void DeinitServices(void)
{
    DisableIntc(kINTC_VBLANK_START);
    RemoveIntcHandler(kINTC_VBLANK_START, 0); 
    DeleteSema(VBlankStartSema);
    DeleteSema(InstallLockSema);
    IopDeinit(); 
}

// --- Main Application Entry Point ---
int main(int argc, char *argv[])
{
    int SystemType, InitSemaID, BootDeviceID; 
    int result_generic; 
    unsigned int FrameNum;
    ee_sema_t sema_config; 
    int current_event = MENU_EVENT_NONE; 

    // sio_init is called unconditionally, so <sio.h> must be included.
    // The DEBUG_PRINTF macro in main.h will handle whether sio_printf actually outputs.
    sio_init(38400, 0, 0, 0, 0); 
    DEBUG_PRINTF("FMCB Installer v%s starting...\n", FMCB_INSTALLER_VERSION); // FMCB_INSTALLER_VERSION is now from Makefile

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
    sema_config.attr = 0; // <<< MODIFIED: Changed EA_THPRI to 0 for default attributes
    sema_config.option = 0;
    VBlankStartSema = CreateSema(&sema_config);

    sema_config.init_count = 1; 
    sema_config.max_count = 1;
    // Attribute for InstallLockSema can also be 0, or EA_THPRI if needed and available
    sema_config.attr = 0; // Ensuring consistency, was likely 0 in original too.
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

    current_event = MENU_EVENT_RETURN_TO_MAIN; 

    while(current_event != EVENT_EXIT) { 
        DEBUG_PRINTF("main.c: Top of event loop, current_event = %d\n", current_event);
        switch (current_event) {
            case MENU_EVENT_RETURN_TO_MAIN:
                DEBUG_PRINTF("main.c: Calling MainMenu()...\n");
                current_event = MainMenu(); 
                DEBUG_PRINTF("main.c: MainMenu() returned event: %d\n", current_event);
                break;

            case MENU_EVENT_GO_TO_OPENTUNA_INSTALL:
                DEBUG_PRINTF("main.c: Calling menu_opentuna_install()...\n");
                current_event = menu_opentuna_install(MENU_EVENT_GO_TO_OPENTUNA_INSTALL, 0); 
                DEBUG_PRINTF("main.c: menu_opentuna_install() returned event: %d\n", current_event);
                break;
            
            case MENU_EVENT_NONE: 
                DEBUG_PRINTF("main.c: Warning - MENU_EVENT_NONE received. Returning to MainMenu.\n");
                current_event = MENU_EVENT_RETURN_TO_MAIN;
                break;

            case EVENT_EXIT: 
                DEBUG_PRINTF("main.c: EVENT_EXIT received. Loop will terminate.\n");
                break;

            default:
                DEBUG_PRINTF("main.c: Unhandled event %d received. Defaulting to MainMenu.\n", current_event);
                current_event = MENU_EVENT_RETURN_TO_MAIN; 
                break;
        }
    } 

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
