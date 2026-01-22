source ./arty-z7.cfg

proc flash_qspi {flasher_path bin_path} {
    adapter speed 10000

    # Need to call this in the tcl script
    init

    # Make sure the Zynq is setup without FSBL
    # having to run first
    arty_z7_reset

    # Load the flasher application into DDR
    echo "Loading ${flasher_path} into DDR"
    load_image ${flasher_path}
    # Start the flasher application so it's
    # ready to copy data
    resume 0x00100000
    sleep 1000

    # Load the BOOT.BIN file into DDR
    halt
    echo "Loading ${bin_path} into DDR"
    load_image ${bin_path} 0x00200400
    # Write the size of the file to the size register
    mww 0x00200004 [file size ${bin_path}]

    # Trigger the flasher to copy the BOOT.BIN into QSPI flash
    mww 0x00200000 0x01
    resume

    shutdown
}

flash_qspi "../flasher/flasher.elf" "../BOOT.BIN"