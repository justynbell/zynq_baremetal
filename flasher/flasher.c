#include <xqspips.h>

#include "flasher.h"

#define W25Q_PAGE_SIZE 256
#define W25Q_SECTOR_SIZE 65536 // 64KB

#define QSPI_DEVICE_ID XPAR_XQSPIPS_0_DEVICE_ID
#define QSPI_CONNECTION_MODE (XPAR_XQSPIPS_0_QSPI_MODE)
#define QSPI_BUS_WIDTH (XPAR_XQSPIPS_0_QSPI_BUS_WIDTH)

#define SINGLE_FLASH_CONNECTION			0
#define DUAL_STACK_CONNECTION			1
#define DUAL_PARALLEL_CONNECTION		2

#define QSPI_BUSWIDTH_ONE	0U
#define QSPI_BUSWIDTH_TWO	1U
#define QSPI_BUSWIDTH_FOUR	2U
#define FLASH_SIZE_16MB					0x1000000

#define LQSPI_CR_FAST_READ			0x0000000B
#define LQSPI_CR_FAST_DUAL_READ		0x0000003B
#define LQSPI_CR_FAST_QUAD_READ		0x0000006B /* Fast Quad Read output */
#define LQSPI_CR_1_DUMMY_BYTE		0x00000100 /* 1 Dummy Byte between
						     address and return data */

static XQspiPs QspiInstance;
static u32 QspiFlashSize;
static u32 QspiFlashMake;

/*
 * The following constants specify the max amount of data and the size of the
 * the buffer required to hold the data and overhead to transfer the data to
 * and from the FLASH.
 */
#define DATA_OFFSET			4 /* Start of Data for Read/Write */
#define DUMMY_OFFSET		4 /* Dummy byte offset for fast, dual and quad
				     reads */
#define DUMMY_SIZE			1 /* Number of dummy bytes for fast, dual and
				     quad reads */
/*
 * The following variables are used to read and write to the eeprom and they
 * are global to avoid having large buffers on the stack
 */
u8 readBuffer[W25Q_PAGE_SIZE + DATA_OFFSET + DUMMY_SIZE];
u8 writeBuffer[W25Q_PAGE_SIZE + DATA_OFFSET + DUMMY_SIZE];

#define READ_ID_CMD			0x9F
#define RD_ID_SIZE			4 /* Read ID command + 3 bytes ID response */

#define MICRON_ID		0x20
#define SPANSION_ID		0x01
#define WINBOND_ID		0xEF
#define MACRONIX_ID		0xC2
#define ISSI_ID			0x9D

#define FLASH_SIZE_ID_8M		0x14
#define FLASH_SIZE_ID_16M		0x15
#define FLASH_SIZE_ID_32M		0x16
#define FLASH_SIZE_ID_64M		0x17
#define FLASH_SIZE_ID_128M		0x18
#define FLASH_SIZE_ID_256M		0x19
#define FLASH_SIZE_ID_512M		0x20
#define FLASH_SIZE_ID_1G		0x21
#define FLASH_SIZE_ID_2G		0x22
/* Macronix size constants are different for 512M and 1G */
#define MACRONIX_FLASH_SIZE_ID_512M		0x1A
#define MACRONIX_FLASH_SIZE_ID_1G		0x1B
#define MACRONIX_FLASH_SIZE_ID_2G		0x1C
#define MACRONIX_FLASH_1_8_V_MX66_ID_512        (0x3A)

#define FLASH_SIZE_8M			0x0100000
#define FLASH_SIZE_16M			0x0200000
#define FLASH_SIZE_32M			0x0400000
#define FLASH_SIZE_64M			0x0800000
#define FLASH_SIZE_128M			0x1000000
#define FLASH_SIZE_256M			0x2000000
#define FLASH_SIZE_512M			0x4000000
#define FLASH_SIZE_1G			0x8000000
#define FLASH_SIZE_2G			0x10000000

// Helper to wait for the chip to finish an internal operation
static void flashWaitForBusy(void) {
    u8 statusReg = 0;
	writeBuffer[0] = 0x05;
	writeBuffer[1] = 0x00;

    while (1) {
        XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, readBuffer, 2);
        statusReg = readBuffer[1];
        if ((statusReg & 0x01) == 0) break; // Check WIP bit (Bit 0)
    }
}

static u32 FlashReadID(void)
{
	u32 Status;

	// Read ID in Auto mode.
	writeBuffer[0]   = READ_ID_CMD;
	// 3 dummy bytes
	writeBuffer[1] = 0x00;
	writeBuffer[2] = 0x00;
	writeBuffer[3] = 0x00;

	Status = XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, readBuffer,
				RD_ID_SIZE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	xil_printf("Single Flash Information\r\n");

	xil_printf("FlashID=0x%x 0x%x 0x%x\r\n", readBuffer[1],
			readBuffer[2],
			readBuffer[3]);

	// Deduce flash make
	if (readBuffer[1] == MICRON_ID) {
		QspiFlashMake = MICRON_ID;
		xil_printf("MICRON ");
	} else if(readBuffer[1] == SPANSION_ID) {
		QspiFlashMake = SPANSION_ID;
		xil_printf("SPANSION ");
	} else if(readBuffer[1] == WINBOND_ID) {
		QspiFlashMake = WINBOND_ID;
		xil_printf("WINBOND ");
	} else if(readBuffer[1] == MACRONIX_ID) {
		QspiFlashMake = MACRONIX_ID;
		xil_printf("MACRONIX ");
	} else if(readBuffer[1] == ISSI_ID) {
		QspiFlashMake = ISSI_ID;
		xil_printf("ISSI ");
	}

	// Deduce flash Size
	if (readBuffer[3] == FLASH_SIZE_ID_8M) {
		QspiFlashSize = FLASH_SIZE_8M;
		xil_printf("8M Bits\r\n");
	} else if (readBuffer[3] == FLASH_SIZE_ID_16M) {
		QspiFlashSize = FLASH_SIZE_16M;
		xil_printf("16M Bits\r\n");
	} else if (readBuffer[3] == FLASH_SIZE_ID_32M) {
		QspiFlashSize = FLASH_SIZE_32M;
		xil_printf("32M Bits\r\n");
	} else if (readBuffer[3] == FLASH_SIZE_ID_64M) {
		QspiFlashSize = FLASH_SIZE_64M;
		xil_printf("64M Bits\r\n");
	} else if (readBuffer[3] == FLASH_SIZE_ID_128M) {
		QspiFlashSize = FLASH_SIZE_128M;
		xil_printf("128M Bits\r\n");
	} else if (readBuffer[3] == FLASH_SIZE_ID_256M) {
		QspiFlashSize = FLASH_SIZE_256M;
		xil_printf("256M Bits\r\n");
	} else if ((readBuffer[3] == FLASH_SIZE_ID_512M)
			|| (readBuffer[3] == MACRONIX_FLASH_1_8_V_MX66_ID_512)
			|| (readBuffer[3] == MACRONIX_FLASH_SIZE_ID_512M)) {
		QspiFlashSize = FLASH_SIZE_512M;
		xil_printf("512M Bits\r\n");
	} else if ((readBuffer[3] == FLASH_SIZE_ID_1G)
			|| (readBuffer[3] == MACRONIX_FLASH_SIZE_ID_1G)) {
		QspiFlashSize = FLASH_SIZE_1G;
		xil_printf("1G Bits\r\n");
	} else if ((readBuffer[3] == FLASH_SIZE_ID_2G)
			|| (readBuffer[3] == MACRONIX_FLASH_SIZE_ID_2G)) {
		QspiFlashSize = FLASH_SIZE_2G;
		xil_printf("2G Bits\r\n");
	}

	return XST_SUCCESS;
}

u32 flasherInit(void)
{
	XQspiPs_Config *QspiConfig;
	int Status;

	// Initialize the QSPI driver so that it's ready to use
	QspiConfig = XQspiPs_LookupConfig(QSPI_DEVICE_ID);
	if (NULL == QspiConfig) {
		return XST_FAILURE;
	}

	Status = XQspiPs_CfgInitialize(&QspiInstance, QspiConfig,
					QspiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Set Manual Chip select options and drive HOLD_B pin high.
	XQspiPs_SetOptions(&QspiInstance, XQSPIPS_FORCE_SSELECT_OPTION |
			XQSPIPS_HOLD_B_DRIVE_OPTION);

	// Set the prescaler for QSPI clock
	XQspiPs_SetClkPrescaler(&QspiInstance, XQSPIPS_CLK_PRESCALE_8);

	// Assert the FLASH chip select.
	XQspiPs_SetSlaveSelect(&QspiInstance);

	// Read Flash ID and extract Manufacture and Size information
	Status = FlashReadID();
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

s32 flashRead(uint32_t address, uint8_t *rdPtr, uint32_t size)
{
    writeBuffer[0] = 0x03;
    writeBuffer[1] = (uint8_t)(address >> 16);
    writeBuffer[2] = (uint8_t)(address >> 8);
    writeBuffer[3] = (uint8_t)address;

	// Need to read the total size + 4 (for the write command and 3 addr bytes)
    return XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, rdPtr, size + 4);
}

// Function to program a large buffer from DDR to Flash
s32 flasherProgram(u32 flashAddr, u32 sourceAddr, u32 byteCount) {
	s32 status = XST_SUCCESS;
    u32 left = byteCount;
    u32 currAddr = flashAddr;
	u32 percent;
	u32 pagesProcessed;
	u32 totalSize;
    u8 *currSource = (u8 *)sourceAddr;

    xil_printf("Erasing needed sectors...\n\r");

	// Erase
    for (u32 i = 0; i < byteCount; i += W25Q_SECTOR_SIZE) {
        // A Write Enable instruction must be executed before the device
        // will accept the Block Erase Instruction
		writeBuffer[0] = 0x06;
        status = XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, NULL, 1);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}

        // Call the 64KB block erase function
		writeBuffer[0] = 0xD8;
        writeBuffer[1] = (u8)((currAddr + i) >> 16);
        writeBuffer[2] = (u8)((currAddr + i) >> 8);
        writeBuffer[3] = (u8)(currAddr + i);
        status = XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, NULL, 4);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}

        flashWaitForBusy();

		// Display progress
		percent = (u32)(((u64)i * 100) / byteCount);
		xil_printf("\rErase Progress: %3d%% [%08X]", percent, currAddr + i);
    }

	xil_printf("\n\r");

	// Write
	totalSize = left;
	pagesProcessed = 0;
    while (left > 0) {
        u32 sendSize = (left > W25Q_PAGE_SIZE) ? W25Q_PAGE_SIZE : left;

        // A Write Enable instruction must be executed before the device
        // will accept the Page Program command
        writeBuffer[0] = 0x06;
        status = XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, NULL, 1);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}

        // Prepare Page Program Command
        writeBuffer[0] = 0x02;
        writeBuffer[1] = (u8)(currAddr >> 16);
        writeBuffer[2] = (u8)(currAddr >> 8);
        writeBuffer[3] = (u8)(currAddr);
        memcpy(&writeBuffer[4], currSource, sendSize);

        // Transfer data
        status = XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, NULL, sendSize + 4);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}

        flashWaitForBusy();

        left -= sendSize;
        currAddr += sendSize;
        currSource += sendSize;
		pagesProcessed++;

		// Display progress
		if ((pagesProcessed % 64) == 0 || left == 0) {
			percent = (u32)(((u64)(totalSize - left) * 100) / totalSize);
			xil_printf("\rProgramming Progress: %3d%% [%08X]", percent, currAddr);
		}
    }

	xil_printf("\n\r");

	left = byteCount;
	currAddr = flashAddr;
	currSource = (u8 *)sourceAddr;
	pagesProcessed = 0;
	while (left > 0) {
		// Verify pages if possible
        u32 readSize = (left > W25Q_PAGE_SIZE) ? W25Q_PAGE_SIZE : left;

		status = flashRead(currAddr, readBuffer, readSize);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		if (memcmp(&readBuffer[4], currSource, readSize) != 0) {
			// Find where it failed within this chunk
			for (u32 i = 0; i < readSize; i++) {
				if (readBuffer[i + 4] != currSource[i]) {
					xil_printf("\r\nVerification Error at 0x%08X\n\r", currAddr + i);
					status = XST_FAILURE;
				}
			}
		}

		left -= readSize;
		currAddr += readSize;
		currSource += readSize;
		pagesProcessed++;

		// Display progress
		if ((pagesProcessed % 64) == 0 || left == 0) {
			percent = (u32)(((u64)(totalSize - left) * 100) / totalSize);
			xil_printf("\rVerification Progress: %3d%% [%08X]", percent, currAddr);
		}
	}

	xil_printf("\n\r");

	return status;
}