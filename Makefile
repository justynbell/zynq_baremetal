BIF_FILE := boot.bif
BOOTGEN  := bootgen

BOOTBIN  := BOOT.BIN

all: bsp fsbl app bootbin

bsp:
	$(MAKE) -C bsp

fsbl: bsp
	$(MAKE) -C fsbl

app: bsp
	$(MAKE) -C app

bootbin: $(BIF_FILE)
	$(BOOTGEN) -image $(BIF_FILE) -o $(BOOTBIN) -w

# ------------------------------------------------------------
# Clean everything
# ------------------------------------------------------------
clean:
	$(MAKE) -C bsp clean
	$(MAKE) -C fsbl clean
	$(MAKE) -C app clean
	rm -f $(BOOTBIN)

.PHONY: all bsp fsbl app bootbin clean