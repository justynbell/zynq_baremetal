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

#define COMMAND_OFFSET		0 /* FLASH instruction */
#define ADDRESS_1_OFFSET	1 /* MSB byte of address to read or write */
#define ADDRESS_2_OFFSET	2 /* Middle byte of address to read or write */
#define ADDRESS_3_OFFSET	3 /* LSB byte of address to read or write */

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
#define DATA_SIZE		4096
#define DATA_OFFSET			4 /* Start of Data for Read/Write */
#define DUMMY_OFFSET		4 /* Dummy byte offset for fast, dual and quad
				     reads */
#define DUMMY_SIZE			1 /* Number of dummy bytes for fast, dual and
				     quad reads */
/*
 * The following variables are used to read and write to the eeprom and they
 * are global to avoid having large buffers on the stack
 */
u8 ReadBuffer[DATA_SIZE + DATA_OFFSET + DUMMY_SIZE];
u8 WriteBuffer[DATA_OFFSET + DUMMY_SIZE];

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

u32 flashRead(uint32_t address, uint8_t *rdPtr, uint32_t size)
{
    WriteBuffer[COMMAND_OFFSET]   = 0x03;
    WriteBuffer[ADDRESS_1_OFFSET] = (uint8_t)(address >> 16);
    WriteBuffer[ADDRESS_2_OFFSET] = (uint8_t)(address >> 8);
    WriteBuffer[ADDRESS_3_OFFSET] = (uint8_t)address;

    return XQspiPs_PolledTransfer(&QspiInstance, WriteBuffer, rdPtr, size);
}

static u32 FlashReadID(void)
{
	u32 Status;

	// Read ID in Auto mode.
	WriteBuffer[COMMAND_OFFSET]   = READ_ID_CMD;
	WriteBuffer[ADDRESS_1_OFFSET] = 0x00;		/* 3 dummy bytes */
	WriteBuffer[ADDRESS_2_OFFSET] = 0x00;
	WriteBuffer[ADDRESS_3_OFFSET] = 0x00;

	Status = XQspiPs_PolledTransfer(&QspiInstance, WriteBuffer, ReadBuffer,
				RD_ID_SIZE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	xil_printf("Single Flash Information\r\n");

	xil_printf("FlashID=0x%x 0x%x 0x%x\r\n", ReadBuffer[1],
			ReadBuffer[2],
			ReadBuffer[3]);

	/*
	 * Deduce flash make
	 */
	if (ReadBuffer[1] == MICRON_ID) {
		QspiFlashMake = MICRON_ID;
		xil_printf("MICRON ");
	} else if(ReadBuffer[1] == SPANSION_ID) {
		QspiFlashMake = SPANSION_ID;
		xil_printf("SPANSION ");
	} else if(ReadBuffer[1] == WINBOND_ID) {
		QspiFlashMake = WINBOND_ID;
		xil_printf("WINBOND ");
	} else if(ReadBuffer[1] == MACRONIX_ID) {
		QspiFlashMake = MACRONIX_ID;
		xil_printf("MACRONIX ");
	} else if(ReadBuffer[1] == ISSI_ID) {
		QspiFlashMake = ISSI_ID;
		xil_printf("ISSI ");
	}

	/*
	 * Deduce flash Size
	 */
	if (ReadBuffer[3] == FLASH_SIZE_ID_8M) {
		QspiFlashSize = FLASH_SIZE_8M;
		xil_printf("8M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_16M) {
		QspiFlashSize = FLASH_SIZE_16M;
		xil_printf("16M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_32M) {
		QspiFlashSize = FLASH_SIZE_32M;
		xil_printf("32M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_64M) {
		QspiFlashSize = FLASH_SIZE_64M;
		xil_printf("64M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_128M) {
		QspiFlashSize = FLASH_SIZE_128M;
		xil_printf("128M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_256M) {
		QspiFlashSize = FLASH_SIZE_256M;
		xil_printf("256M Bits\r\n");
	} else if ((ReadBuffer[3] == FLASH_SIZE_ID_512M)
			|| (ReadBuffer[3] == MACRONIX_FLASH_1_8_V_MX66_ID_512)
			|| (ReadBuffer[3] == MACRONIX_FLASH_SIZE_ID_512M)) {
		QspiFlashSize = FLASH_SIZE_512M;
		xil_printf("512M Bits\r\n");
	} else if ((ReadBuffer[3] == FLASH_SIZE_ID_1G)
			|| (ReadBuffer[3] == MACRONIX_FLASH_SIZE_ID_1G)) {
		QspiFlashSize = FLASH_SIZE_1G;
		xil_printf("1G Bits\r\n");
	} else if ((ReadBuffer[3] == FLASH_SIZE_ID_2G)
			|| (ReadBuffer[3] == MACRONIX_FLASH_SIZE_ID_2G)) {
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

// Helper to wait for the chip to finish an internal operation
static void flashWaitForBusy(void) {
    u8 statusReg = 0;
    u8 readBuffer[2]; 
    u8 writeBuffer[2] = {0x05, 0x00}; // Read Status Register-1 command

    while (1) {
        XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, readBuffer, 2);
        statusReg = readBuffer[1];
        if ((statusReg & 0x01) == 0) break; // Check WIP bit (Bit 0)
    }
}

// Function to program a large buffer from DDR to Flash
void flasherProgram(u32 flashAddr, u8 *sourceAddr, u32 byteCount) {
    u32 left = byteCount;
    u32 currAddr = flashAddr;
    u8 *currSource = sourceAddr;
    u8 writeBuffer[W25Q_PAGE_SIZE + 4];

    xil_printf("Erasing needed sectors...\n\r");

    for (u32 i = 0; i < byteCount; i += W25Q_SECTOR_SIZE) {
        u8 eraseCmd[4];
        eraseCmd[0] = 0xD8; // Block Erase (64KB)
        eraseCmd[1] = (u8)((currAddr + i) >> 16);
        eraseCmd[2] = (u8)((currAddr + i) >> 8);
        eraseCmd[3] = (u8)(currAddr + i);

        u8 writeEnable = 0x06;

        // A Write Enable instruction must be executed before the device
        // will accept the Block Erase Instruction
        XQspiPs_PolledTransfer(&QspiInstance, &writeEnable, NULL, 1);
        // Call the 64KB block erase function
        XQspiPs_PolledTransfer(&QspiInstance, eraseCmd, NULL, 4);

		xil_printf("%d\\% Erase Complete\r", (i * W25Q_SECTOR_SIZE) / byteCount);
        flashWaitForBusy();
    }

    xil_printf("\r\nProgramming pages...\n\r");

    while (left > 0) {
        u32 sendSize = (left > W25Q_PAGE_SIZE) ? W25Q_PAGE_SIZE : left;

        // 1. Write Enable
        u8 WriteEnable = 0x06;
        XQspiPs_PolledTransfer(&QspiInstance, &WriteEnable, NULL, 1);

        // 2. Prepare Page Program Command
        writeBuffer[0] = 0x02;
        writeBuffer[1] = (u8)(currAddr >> 16);
        writeBuffer[2] = (u8)(currAddr >> 8);
        writeBuffer[3] = (u8)(currAddr);
        memcpy(&writeBuffer[4], currSource, sendSize);

        // 3. Transfer
        XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, NULL, sendSize + 4);
        flashWaitForBusy();

        left -= sendSize;
        currAddr += sendSize;
        currSource += sendSize;

		xil_printf("Writing QSPI flash\n\r");
    }

	xil_printf("Done\n\r");
}