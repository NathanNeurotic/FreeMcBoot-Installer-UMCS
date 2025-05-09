#include <iopcontrol.h>
#include <iopcontrol_special.h> // For SifIopRebootBuffer
#include <iopheap.h>
#include <kernel.h>         // For SleepThread, CreateSema, SignalSema, ExitDeleteThread, SysCreateThread, etc.
#include <libcdvd.h>
#include <libmc.h>
#include <libpwroff.h>      // For poweroffInit, poweroffSetCallback
#include <loadfile.h>       // For SifExecModuleBuffer, SifLoadFileInit/Exit
#include <libpad.h>         // For padInit, padPortOpen/Close, padEnd (though we use local pad.h wrappers)
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>
#include <fileXio_rpc.h>    // For fileXioInit, fileXioExit

// FMCB Project Specific Includes
#include "main.h"           // For DEBUG_PRINTF, EXFAT define, poweroffCallback
#include "iop.h"            // For IOP_MOD_ flags, IopInitStart, IopDeinit
#include "system.h"         // Potentially for system-specific functions (not directly used here but good practice)
#include "pad.h"            // <<< ADDED: For PadInitPads, PadDeinitPads
#include "libsecr.h"        // <<< ADDED: For SecrInit, SecrDeinit
#include "mctools_rpc.h"    // <<< ADDED: For InitMCTOOLS, DeinitMCTOOLS

#define IMPORT_IRX(_IRX) \
extern unsigned char _IRX[]; \
extern unsigned int size_##_IRX;

IMPORT_IRX(IOMANX_irx);
IMPORT_IRX(FILEXIO_irx);
IMPORT_IRX(SIO2MAN_irx);
IMPORT_IRX(PADMAN_irx);
IMPORT_IRX(MCMAN_irx);
IMPORT_IRX(MCSERV_irx);
IMPORT_IRX(SECRSIF_irx);
IMPORT_IRX(MCTOOLS_irx);
IMPORT_IRX(USBD_irx);
#ifdef EXFAT
IMPORT_IRX(usbmass_bd_irx);
IMPORT_IRX(bdm_irx);
IMPORT_IRX(bdmfs_fatfs_irx);
#else
IMPORT_IRX(USBHDFSD_irx);
#endif
IMPORT_IRX(POWEROFF_irx);
IMPORT_IRX(DEV9_irx);
IMPORT_IRX(ATAD_irx);
IMPORT_IRX(HDD_irx);
IMPORT_IRX(PFS_irx);
IMPORT_IRX(IOPRP_img);

u8 dev9Loaded; // Global flag indicating if DEV9.irx loaded successfully

#define SYSTEM_INIT_THREAD_STACK_SIZE 0x1000

struct SystemInitParams
{
    int InitCompleteSema;
    unsigned int flags;
};

static void SystemInitThread(struct SystemInitParams *SystemInitParams)
{
    static const char PFS_args[] = "-n\0"
                                   "24\0"
                                   "-o\0"
                                   "8";
    // int i; // <<< REMOVED: Unused variable

    if (SystemInitParams->flags & IOP_MOD_HDD) {
        if (SifExecModuleBuffer(ATAD_irx, size_ATAD_irx, 0, NULL, NULL) >= 0) {
            SifExecModuleBuffer(HDD_irx, size_HDD_irx, 0, NULL, NULL);
            SifExecModuleBuffer(PFS_irx, size_PFS_irx, sizeof(PFS_args), PFS_args, NULL);
        }
    }

    if (SystemInitParams->flags & IOP_MOD_SECRSIF) {
        SifExecModuleBuffer(SECRSIF_irx, size_SECRSIF_irx, 0, NULL, NULL);
        SecrInit(); // Declaration should be in libsecr.h or secrman.h (if module specific)
    }

    if (SystemInitParams->flags & IOP_MOD_MCTOOLS) {
        SifExecModuleBuffer(MCTOOLS_irx, size_MCTOOLS_irx, 0, NULL, NULL);
        InitMCTOOLS(); // Declaration should be in mctools_rpc.h
    }

    SifExitIopHeap();
    SifLoadFileExit();

    SignalSema(SystemInitParams->InitCompleteSema);
    ExitDeleteThread();
}

int IopInitStart(unsigned int flags)
{
    ee_sema_t sema;
    static struct SystemInitParams InitThreadParams;
    static unsigned char SysInitThreadStack[SYSTEM_INIT_THREAD_STACK_SIZE] __attribute__((aligned(16)));
    int stat, ret;

    if (!(flags & IOP_REBOOT)) {
        SifInitRpc(0);
    } else {
        PadDeinitPads();    // Declaration in local pad.h
        sceCdInit(SCECdEXIT);
        DeinitMCTOOLS();    // Declaration in mctools_rpc.h
        SecrDeinit();       // Declaration in libsecr.h or secrman.h
        fileXioExit();
    }

    if (!(flags & IOP_LIBSECR_IMG)) { // Assuming this means standard IOP reset
        SifIopReset("", 0);
    } else { // Assuming this means reset with a specific image (IOPRP.img)
        SifIopRebootBuffer(IOPRP_img, size_IOPRP_img);
    }

    sema.init_count = 0;
    sema.max_count = 1;
    sema.attr = 0; // Default attributes
    sema.option = 0;
    InitThreadParams.InitCompleteSema = CreateSema(&sema);
    InitThreadParams.flags = flags;

    while (!SifIopSync()) {}; // Wait for IOP to sync after reset/reboot

    SifInitRpc(0);      // Re-initialize RPC after IOP reset
    SifInitIopHeap();
    SifLoadFileInit();

    sbv_patch_enable_lmb(); // Kernel/sbv_patches.h

    // Load essential IOP modules
    SifExecModuleBuffer(IOMANX_irx, size_IOMANX_irx, 0, NULL, NULL);
    SifExecModuleBuffer(FILEXIO_irx, size_FILEXIO_irx, 0, NULL, NULL);

    fileXioInit();      // Initialize FILEXIO RPC client
    sceCdInit(SCECdINoD); // Initialize CDVDMAN without checking for disc

    SifExecModuleBuffer(POWEROFF_irx, size_POWEROFF_irx, 0, NULL, NULL);
    ret = SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, &stat);
    dev9Loaded = (ret >= 0 && stat == 0); // dev9.irx must load and return RESIDENT END

    SifExecModuleBuffer(SIO2MAN_irx, size_SIO2MAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(PADMAN_irx, size_PADMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(MCMAN_irx, size_MCMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(MCSERV_irx, size_MCSERV_irx, 0, NULL, NULL);

#ifdef EXFAT
    SifExecModuleBuffer(bdm_irx, size_bdm_irx, 0, NULL, NULL);
    SifExecModuleBuffer(bdmfs_fatfs_irx, size_bdmfs_fatfs_irx, 0, NULL, NULL);
#endif

    SifExecModuleBuffer(USBD_irx, size_USBD_irx, 0, NULL, NULL);

#ifdef EXFAT
    SifExecModuleBuffer(usbmass_bd_irx, size_usbmass_bd_irx, 0, NULL, NULL);
#else
    SifExecModuleBuffer(USBHDFSD_irx, size_USBHDFSD_irx, 0, NULL, NULL);
#endif

    // The original `sleep(5)` is problematic.
    // PS2SDK uses `SleepThread()` (from kernel.h) for task yielding,
    // or `DelayThread(microseconds)` (from timer.h, which needs timer_irx).
    // A simple delay of 5 "somethings" is ambiguous.
    // If it's a short delay for hardware init, a few VBlanks might be intended.
    // Let's use SleepThread() to yield, or a short DelayThread if timer is available.
    // Since timer_irx isn't loaded, SleepThread() is safer.
    // If a 5-second delay was intended, it would be DelayThread(5 * 1000 * 1000).
    // For now, just a short yield.
    SleepThread(); // Yield execution, allows other threads/IOP to process.
    SleepThread(); // Call a few times for a noticeable short delay if needed.
    SleepThread();
    SleepThread();
    SleepThread();


    SysCreateThread(SystemInitThread, SysInitThreadStack, SYSTEM_INIT_THREAD_STACK_SIZE, &InitThreadParams, 0x2); // Priority 2

    // Initialize EE-side components that rely on IOP modules
    poweroffInit();                             // libpwroff.h
    poweroffSetCallback(&poweroffCallback, NULL); // poweroffCallback is in main.c
    mcInit(MC_TYPE_XMC);                        // libmc.h
    PadInitPads();                              // local pad.h

    return InitThreadParams.InitCompleteSema;
}

void IopDeinit(void)
{
    PadDeinitPads();    // local pad.h
    sceCdInit(SCECdEXIT);
    DeinitMCTOOLS();    // mctools_rpc.h
    SecrDeinit();       // libsecr.h or secrman.h

    fileXioExit();
    SifExitRpc();
    // SifExitCmd(); // SifExitCmd is usually called at the very end of main or by Exit()
}
