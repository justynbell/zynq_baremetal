#ifndef QSPI_H
#define QSPI_H

#include <xqspips.h>

#define QSPI_DEVICE_ID XPAR_XQSPIPS_0_DEVICE_ID

class QSpiFlash {
private:
	XQspiPs QspiInstance;

public:
	uint8_t init(void)
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
		XQspiPs_SetClkPrescaler(&QspiInstance, XQSPIPS_CLK_PRESCALE_32);

		// Assert the FLASH chip select.
		XQspiPs_SetSlaveSelect(&QspiInstance);

		return XST_SUCCESS;
	}

	uint8_t mac_read(uint8_t *mac)
	{
		/* Size of the Read Security Registers command */
		#define HEADER_SIZE 5
		#define MAC_ADDR_SIZE 6

		uint8_t readBuffer[HEADER_SIZE + MAC_ADDR_SIZE] = {0};
		/* TODO: I don't know why we need a +1 here */
		uint8_t writeBuffer[HEADER_SIZE + MAC_ADDR_SIZE + 1] = {0};
		uint8_t status;

		/* Location of MAC address in security registers */
		uint32_t address = 0x001000;

		writeBuffer[0] = 0x48;
		writeBuffer[1] = (uint8_t)(address >> 16);
		writeBuffer[2] = (uint8_t)(address >>  8);
		writeBuffer[3] = (uint8_t)(address >>  0);
		writeBuffer[4] = 0; /* Dummy byte per datasheet */

		status = XQspiPs_PolledTransfer(&QspiInstance, writeBuffer, readBuffer, sizeof(writeBuffer));
		if (status == XST_SUCCESS) {
			memcpy(mac, readBuffer + HEADER_SIZE, MAC_ADDR_SIZE);
		}

		return status;
	}
};

#endif /* QSPI_H */