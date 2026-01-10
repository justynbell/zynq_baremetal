#include <stdio.h>
#include <limits.h>

#include "xil_printf.h"
#include "sleep.h"
#include "xscugic.h"

#include "FreeRTOS.h"
#include "task.h"

static void setup(void)
{
    BaseType_t xStatus;
    XScuGic_Config *pxGICConfig;
    extern XScuGic xInterruptController;

	/* Ensure no interrupts execute while the scheduler is in an inconsistent
	state.  Interrupts are automatically enabled when the scheduler is
	started. */
	portDISABLE_INTERRUPTS();

	/* Obtain the configuration of the GIC. */
	pxGICConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);

	/* Sanity check the FreeRTOSConfig.h settings are correct for the
	hardware. */
	configASSERT(pxGICConfig);
	configASSERT(pxGICConfig->CpuBaseAddress == (configINTERRUPT_CONTROLLER_BASE_ADDRESS + configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET));
	configASSERT(pxGICConfig->DistBaseAddress == configINTERRUPT_CONTROLLER_BASE_ADDRESS);

	/* Install a default handler for each GIC interrupt. */
	xStatus = XScuGic_CfgInitialize(&xInterruptController, pxGICConfig, pxGICConfig->CpuBaseAddress);
	configASSERT(xStatus == XST_SUCCESS);
	(void) xStatus; /* Remove compiler warning if configASSERT() is not defined. */

	/* Initialise the LED port. */
	//vParTestInitialise();

	/* The Xilinx projects use a BSP that do not allow the start up code to be
	altered easily.  Therefore the vector table used by FreeRTOS is defined in
	FreeRTOS_asm_vectors.S, which is part of this project.  Switch to use the
	FreeRTOS vector table. */
	vPortInstallFreeRTOSVectorTable();
}

void vTestTask1(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        printf("FREERTOS\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vTestTask2(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        printf("freertos\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main()
{
    setup();

    xTaskCreate(vTestTask1, "TestTask1", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vTestTask2, "TestTask2", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    // Should never reach here.
    for(;;);
}