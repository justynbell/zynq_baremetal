BIF_FILE     := boot.bif
BOOTGEN      := bootgen
OPENOCD      := openocd
OPENOCD_DIR  := openocd
FLASH_SCRIPT := flash_qspi.tcl

BOOTBIN  := BOOT.BIN

all: bsp fsbl app flasher bootbin

bsp:
	$(MAKE) -C bsp

fsbl: bsp
	$(MAKE) -C fsbl

app: bsp
	$(MAKE) -C app

flasher:
	$(MAKE) -C flasher

bootbin: $(BIF_FILE)
	$(BOOTGEN) -image $(BIF_FILE) -o $(BOOTBIN) -w

flash: bootbin
	@echo "Starting SQPI Flash process..."
	cd $(OPENOCD_DIR) && $(OPENOCD) -f $(FLASH_SCRIPT)

# ------------------------------------------------------------
# Clean everything
# ------------------------------------------------------------
clean:
	$(MAKE) -C bsp clean
	$(MAKE) -C fsbl clean
	$(MAKE) -C app clean
	$(MAKE) -C flasher clean
	rm -f $(BOOTBIN)

.PHONY: all bsp fsbl app flasher bootbin clean flash