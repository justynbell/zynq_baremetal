#include <xil_cache.h>
#include <xil_mmu.h>
#include <xqspips.h>
#include <xil_printf.h>

#include "flasher.h"

#define START_FLAG_ADDR  0x00100000 // Where we poke to start
#define BINARY_SIZE_ADDR 0x00100004 // The size of the binary

#define FLASH_BASE_ADDR 0x00200000 // Where OpenOCD puts the file

int main() {
    uint32_t imageSize = 0;

    Xil_DCacheDisable();
    Xil_ICacheDisable();
    Xil_DisableMMU();

    // 1. Initialize QSPI driver
    flasherInit();

    // 2. Clear START_FLAG_ADDR
    *(volatile uint32_t*)START_FLAG_ADDR = 0;

    xil_printf("Flasher Ready. Waiting for JTAG upload...\n\r");

    // Verify by reading
    uint8_t readBuffer[64 + 4] = { 0 };
    flashRead(0x00, readBuffer, sizeof(readBuffer));

    // Wait for something (OpenOCD) to poke the Start Register
    while(*(volatile uint32_t*)START_FLAG_ADDR == 0);

    // Get the size of what was written
    imageSize = *(volatile uint32_t *)BINARY_SIZE_ADDR;
    // Safety check (16MB max)
    if (imageSize == 0 || imageSize > 0x1000000) {
        xil_printf("ERROR: Invalid binary size. Aborting.\n\r");
        return -1;
    }

    xil_printf("Image size: %d\n\r", imageSize);

    xil_printf("Starting Flash Write...\n\r");

    flasherProgram(0x00, *(volatile uint32_t *)FLASH_BASE_ADDR, imageSize);

    // 3. Loop: Erase Sector -> Page Program -> Verify
    // 4. Print "Done"
    return 0;
}