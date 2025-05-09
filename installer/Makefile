# Makefile for the FreeMcBoot Installer EE (Emotion Engine) application

EE_BIN_DIR = ./ 
EE_BIN_BASE_NAME = UNC_FMCBInstaller
EE_BIN_PACKED_BASE_NAME = FMCBInstaller

EXFAT ?= 0 
HAS_EXFAT =
ifeq ($(EXFAT), 1)
    HAS_EXFAT = _EXFAT
endif

EE_BIN = $(EE_BIN_DIR)$(EE_BIN_BASE_NAME)$(HAS_EXFAT).elf
EE_BIN_PACKED = $(EE_BIN_DIR)$(EE_BIN_PACKED_BASE_NAME)$(HAS_EXFAT).elf

IRX_DIR = irx/compiled

EE_IOP_OBJS = IOPRP_img.o IOMANX_irx.o FILEXIO_irx.o SIO2MAN_irx.o PADMAN_irx.o MCMAN_irx.o \
              MCSERV_irx.o SECRSIF_irx.o MCTOOLS_irx.o USBD_irx.o POWEROFF_irx.o DEV9_irx.o \
              ATAD_irx.o HDD_irx.o PFS_irx.o

EE_RES_OBJS = background.o buttons.o

EE_OBJS = main.o iop.o UI.o menu.o libsecr.o pad.o system.o graphics.o ReqSpaceCalc.o font.o \
          menu_opentuna.o \
          $(EE_RES_OBJS) $(EE_IOP_OBJS) mctools_rpc.o

EE_INCS := -I. \
           -I$(PS2SDK)/ports/include \
           -I$(PS2SDK)/ee/include \
           -I$(PS2SDK)/common/include \
           -I./irx/source/secrsif/src \
           -I./irx/source/mctools/src/

EE_LDFLAGS := -L$(PS2SDK)/ports/lib \
              -L$(PS2SDK)/ee/lib \
              -L$(PS2DEV)/ee/ee/lib \
              -Tlinkfile \
              -s 

EE_LIBS = -lgs -lpng -lz -lm -lfreetype -lpoweroff -lcdvd -lmc -lpadx -lhdd -lfileXio -lpatches -liopreboot -lc -lkernel

EE_GPVAL = -G8192

EE_CFLAGS += -Os -mgpopt $(EE_GPVAL) -Wall -std=gnu99 # <<< ENSURE -std=gnu99 IS HERE

ifeq ($(EXFAT), 1)
    EE_CFLAGS += -DEXFAT
    EE_OBJS += usbmass_bd_irx.o bdm_irx.o bdmfs_fatfs_irx.o
else
    EE_OBJS += USBHDFSD_irx.o
endif

ifeq ($(DEBUG),1)
    EE_CFLAGS += -DDEBUG_TTY_FEEDBACK
endif

ifdef FMCB_INSTALLER_VERSION
    EE_CFLAGS += -DFMCB_INSTALLER_VERSION=\"$(FMCB_INSTALLER_VERSION)\"
endif

all: $(EE_BIN_PACKED)

%.o : %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

$(EE_BIN) : $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS)
	$(EE_CC) $(EE_CFLAGS) -nostartfiles $(EE_LDFLAGS) -o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS) $(EE_LIBS)

$(EE_BIN_PACKED): $(EE_BIN)
	@echo " -- Compressing ELF $(EE_BIN) -> $(EE_BIN_PACKED) --"
	ps2-packer -v $< $@

clean:
	@echo " -- Cleaning compiled files --"
	rm -f $(EE_BIN) $(EE_BIN_PACKED) $(EE_OBJS) \
	      IOPRP_img.c IOMANX_irx.c FILEXIO_irx.c SIO2MAN_irx.c PADMAN_irx.c MCMAN_irx.c \
	      MCSERV_irx.c SECRSIF_irx.c MCTOOLS_irx.c USBD_irx.c POWEROFF_irx.c DEV9_irx.c \
	      ATAD_irx.c HDD_irx.c PFS_irx.c \
	      background.c buttons.c \
	      usbmass_bd_irx.c bdm_irx.c bdmfs_fatfs_irx.c USBHDFSD_irx.c 

rebuild: clean all

# bin2c rules... (rest of your Makefile)
background.c: resources/background.png
	bin2c $< $@ background
buttons.c: resources/buttons.png
	bin2c $< $@ buttons
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
HDD_irx.c: $(PS2SDK)/iop/irx/ps2hdd-osd.irx 
	bin2c $< $@ HDD_irx
PFS_irx.c: $(PS2SDK)/iop/irx/ps2fs.irx 
	bin2c $< $@ PFS_irx
SECRSIF_irx.c: $(IRX_DIR)/secrsif.irx
	bin2c $< $@ SECRSIF_irx
MCTOOLS_irx.c: $(IRX_DIR)/mctools.irx
	bin2c $< $@ MCTOOLS_irx
IOPRP_img.c: $(IRX_DIR)/IOPRP.img 
	bin2c $< $@ IOPRP_img
ifeq ($(EXFAT), 1)
usbmass_bd_irx.c: $(IRX_DIR)/usbmass_bd.irx
	bin2c $< $@ usbmass_bd_irx
bdm_irx.c: $(IRX_DIR)/bdm.irx
	bin2c $< $@ bdm_irx
bdmfs_fatfs_irx.c: $(IRX_DIR)/bdmfs_fatfs.irx
	bin2c $< $@ bdmfs_fatfs_irx
else
USBHDFSD_irx.c: $(PS2SDK)/iop/irx/usbhdfsd.irx
	bin2c $< $@ USBHDFSD_irx
endif

include $(PS2SDK)/Defs.make
