#include <xil_cache.h>
#include <xil_mmu.h>
#include <xqspips.h>
#include <xil_printf.h>

#include "flasher.h"

#define START_FLAG_ADDR  0x00100000 // Where we poke to start
#define BINARY_SIZE_ADDR 0x00100004 // The size of the binary
#define FLASH_BASE_ADDR  0x00200000 // Where OpenOCD puts the file

int main() {
    uint32_t imageSize = 0;
    int32_t status;

    Xil_DCacheDisable();
    Xil_ICacheDisable();
    Xil_DisableMMU();

    // Initialize QSPI driver
    flasherInit();

    // Clear start and done flags
    *(volatile uint32_t*)START_FLAG_ADDR = 0x00;

    xil_printf("Flasher Ready. Waiting for JTAG upload...\n\r");

    // Wait for something (OpenOCD) to poke the Start Register
    while(*(volatile uint32_t*)START_FLAG_ADDR == 0);

    // Get the size of what was written
    imageSize = *(volatile uint32_t *)BINARY_SIZE_ADDR;
    // 16MB max image size
    if (imageSize == 0 || imageSize > 0x1000000) {
        xil_printf("ERROR: Invalid binary size. Aborting.\n\r");
        return 0;
    }

    xil_printf("Image size: %d\n\r", imageSize);
    xil_printf("Starting Flash Write...\n\r");

    status = flasherProgram(0x00, (uint8_t *)FLASH_BASE_ADDR, imageSize);
    if (status != XST_SUCCESS) {
        xil_printf("FAILED LOADING QSPI\n\r");
        return 0;
    }

    xil_printf("\n\r******************************************\n\r");
    xil_printf("   FLASH UPDATE SUCCESSFUL!               \n\r");
    xil_printf("   You may now power cycle the board.     \n\r");
    xil_printf("******************************************\n\r");
    return 0;
}