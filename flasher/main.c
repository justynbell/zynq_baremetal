#include <xil_cache.h>
#include <xil_mmu.h>
#include <xqspips.h>
#include <xil_printf.h>

#include "flasher.h"

extern uint32_t _STAGING_START;
extern uint32_t _STAGING_END;

int32_t copy_flash(uint32_t dstAddr, uint32_t srcAddr, uint32_t imageSize)
{
    int32_t status;

    // 16MB max image size
    if (imageSize == 0 || imageSize > 0x1000000) {
        xil_printf("ERROR: Invalid binary size. Aborting.\n\r");
        return XST_FAILURE;
    }

    // Make sure the source address is within the staging zone
    if (srcAddr < (uint32_t)&_STAGING_START || srcAddr > (uint32_t)&_STAGING_END) {
        xil_printf("ERROR: Invalid source address, outside of DDR region.\n\r");
        return XST_FAILURE;
    }

    xil_printf("Image size: %d\n\r", imageSize);
    xil_printf("Starting Flash Write...\n\r");

    status = flasherProgram(dstAddr, srcAddr, imageSize);
    if (status != XST_SUCCESS) {
        xil_printf("FAILED LOADING QSPI\n\r");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

int main() {
    uint32_t imageSize = 0;
    int32_t status;

    Xil_DCacheDisable();
    Xil_ICacheDisable();
    Xil_DisableMMU();

    // Initialize QSPI driver
    flasherInit();

    // Clear start and done flags
    CONFIG_REGISTER->START = 0x00;

    xil_printf("Flasher Ready. Waiting for JTAG upload...\n\r");

    // Wait for something (OpenOCD) to poke the Start Register
    while(CONFIG_REGISTER->START == 0);

    // Get the size of what was written
    imageSize = CONFIG_REGISTER->IMAGE_SIZE;

    status = copy_flash(QSPI_ADDR, (uint32_t)&_STAGING_START, imageSize);
    if (status != XST_SUCCESS) {
        xil_printf("Failed to copy image to flash: (addr: 0x%08X, size: %d)",
            &_STAGING_START, imageSize);
            return 0;
    }

    xil_printf("\n\r******************************************\n\r");
    xil_printf("   FLASH UPDATE SUCCESSFUL!               \n\r");
    xil_printf("   You may now power cycle the board.     \n\r");
    xil_printf("******************************************\n\r");
    return 0;
}