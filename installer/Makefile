# Makefile for the FreeMcBoot Installer EE (Emotion Engine) application

# Target ELF name parts
EE_BIN_DIR = ./
EE_BIN_BASE_NAME = UNC_FMCBInstaller
EE_BIN_PACKED_BASE_NAME = FMCBInstaller

# exFAT support option (0 = no, 1 = yes)
EXFAT ?= 0 # Default to 0 if not set externally

# Determine suffix based on EXFAT option
HAS_EXFAT =
ifeq ($(EXFAT), 1)
    HAS_EXFAT = _EXFAT
endif

# Final ELF names
EE_BIN = $(EE_BIN_DIR)$(EE_BIN_BASE_NAME)$(HAS_EXFAT).elf
EE_BIN_PACKED = $(EE_BIN_DIR)$(EE_BIN_PACKED_BASE_NAME)$(HAS_EXFAT).elf

# Directory for pre-compiled IOP modules (referenced by bin2c rules)
IRX_DIR = irx/compiled

# IOP modules that are converted to C arrays and linked into the EE binary
EE_IOP_OBJS = IOPRP_img.o IOMANX_irx.o FILEXIO_irx.o SIO2MAN_irx.o PADMAN_irx.o MCMAN_irx.o \
              MCSERV_irx.o SECRSIF_irx.o MCTOOLS_irx.o USBD_irx.o POWEROFF_irx.o DEV9_irx.o \
              ATAD_irx.o HDD_irx.o PFS_irx.o

# Resource files (images) converted to C arrays
EE_RES_OBJS = background.o buttons.o

# EE Object files
# Added menu_opentuna.o to this list
EE_OBJS = main.o iop.o UI.o menu.o libsecr.o pad.o system.o graphics.o ReqSpaceCalc.o font.o \
          menu_opentuna.o \
          $(EE_RES_OBJS) $(EE_IOP_OBJS) mctools_rpc.o

# Include paths for EE compilation
EE_INCS := -I$(PS2SDK)/ports/include \
           -I$(PS2SDK)/ee/include \
           -I$(PS2SDK)/common/include \
           -I./irx/source/secrsif/src \
           -I./irx/source/mctools/src/ \
           -I. # Add current directory for project headers like main.h, UI.h etc.

# Linker flags for EE
EE_LDFLAGS := -L$(PS2SDK)/ports/lib \
              -L$(PS2SDK)/ee/lib \
              -L$(PS2DEV)/ee/ee/lib \
              -Tlinkfile \
              -s # Strip symbols from final ELF

# Libraries to link against for EE
EE_LIBS = -lgs -lpng -lz -lm -lfreetype -lpoweroff -lcdvd -lmc -lpadx -lhdd -lfileXio -lpatches -liopreboot -lc -lkernel

# GP value for MIPS architecture (optimizes access to global variables)
EE_GPVAL = -G8192

# CFLAGS for EE compilation
# -Os for size optimization, -mgpopt for GP optimization
EE_CFLAGS += -Os -mgpopt $(EE_GPVAL)

# Conditional compilation for exFAT support
ifeq ($(EXFAT), 1)
    EE_CFLAGS += -DEXFAT
    EE_OBJS += usbmass_bd_irx.o bdm_irx.o bdmfs_fatfs_irx.o
else
    EE_OBJS += USBHDFSD_irx.o
endif

# Conditional compilation for Debug TTY feedback
ifeq ($(DEBUG),1)
    EE_CFLAGS += -DDEBUG_TTY_FEEDBACK
endif

# Conditional compilation for Installer Version
ifdef FMCB_INSTALLER_VERSION
    EE_CFLAGS += -DFMCB_INSTALLER_VERSION=\"$(FMCB_INSTALLER_VERSION)\"
endif

# Default rule to build all
all: $(EE_BIN_PACKED)

# Rule to compile .c files to .o
%.o : %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

# Rule to compile .S (assembly with C preprocessor) files to .o
%.o : %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

# Rule to compile .s (pure assembly) files to .o
%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

# Rule to link EE objects into the final ELF
$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) $(EE_CFLAGS) -nostartfiles $(EE_LDFLAGS) -o $(EE_BIN) $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o $(EE_LIBS)

# Rule to pack the ELF using ps2-packer
$(EE_BIN_PACKED): $(EE_BIN)
	@echo " -- Compressing ELF --"
	ps2-packer -v $< $@

# Rule to clean compiled files
clean:
	@echo " -- Cleaning compiled files --"
	rm -f $(EE_BIN) $(EE_BIN_PACKED) $(EE_OBJS) \
	      *_irx.c background.c buttons.c IOPRP_img.c \
	      usbmass_bd_irx.c bdm_irx.c bdmfs_fatfs_irx.c # Clean exFAT specific generated files too

# Rule to rebuild (clean then build)
rebuild: clean all

# Rules for converting binary resources/IRXs to C arrays using bin2c
# These .c files will then be compiled into .o files and linked.

background.c: resources/background.png
	bin2c $< $@ background

buttons.c: resources/buttons.png
	bin2c $< $@ buttons

# Standard PS2SDK IRXs
POWEROFF_irx.c: $(PS2SDK)/iop/irx/poweroff.irx
	bin2c $< $@ POWEROFF_irx

DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	bin2c $< $@ DEV9_irx

IOMANX_irx.c: $(PS2SDK)/iop/irx/iomanX.irx
	bin2c $< $@ IOMANX_irx

FILEXIO_irx.c: $(PS2SDK)/iop/irx/fileXio.irx
	bin2c $< $@ FILEXIO_irx

SIO2MAN_irx.c: $(PS2SDK)/iop/irx/freesio2.irx
	bin2c $< $@ SIO2MAN_irx

PADMAN_irx.c: $(PS2SDK)/iop/irx/freepad.irx
	bin2c $< $@ PADMAN_irx

MCMAN_irx.c: $(PS2SDK)/iop/irx/mcman.irx
	bin2c $< $@ MCMAN_irx

MCSERV_irx.c: $(PS2SDK)/iop/irx/mcserv.irx
	bin2c $< $@ MCSERV_irx

USBD_irx.c: $(PS2SDK)/iop/irx/usbd.irx
	bin2c $< $@ USBD_irx

ATAD_irx.c: $(PS2SDK)/iop/irx/ps2atad.irx
	bin2c $< $@ ATAD_irx

HDD_irx.c: $(PS2SDK)/iop/irx/ps2hdd-osd.irx # Or ps2hdd.irx depending on SDK version/config
	bin2c $< $@ HDD_irx

PFS_irx.c: $(PS2SDK)/iop/irx/ps2fs.irx # Or pfsaux.irx / pfs.irx depending on SDK
	bin2c $< $@ PFS_irx

# Custom/Project-specific IRXs (from IRX_DIR)
SECRSIF_irx.c: $(IRX_DIR)/secrsif.irx
	bin2c $< $@ SECRSIF_irx

MCTOOLS_irx.c: $(IRX_DIR)/mctools.irx
	bin2c $< $@ MCTOOLS_irx

IOPRP_img.c: $(IRX_DIR)/IOPRP.img # This is an IOP Reset Program image
	bin2c $< $@ IOPRP_img

# Conditional IRX to C conversion based on EXFAT support
ifeq ($(EXFAT), 1)
usbmass_bd_irx.c: $(IRX_DIR)/usbmass_bd.irx
	bin2c $< $@ usbmass_bd_irx

bdm_irx.c: $(IRX_DIR)/bdm.irx
	bin2c $< $@ bdm_irx

bdmfs_fatfs_irx.c: $(IRX_DIR)/bdmfs_fatfs.irx
	bin2c $< $@ bdmfs_fatfs_irx
else
# Standard USBHDFSD if not using exFAT
USBHDFSD_irx.c: $(PS2SDK)/iop/irx/usbhdfsd.irx
	bin2c $< $@ USBHDFSD_irx
endif

# Include common PS2SDK makefile definitions
include $(PS2SDK)/Defs.make
