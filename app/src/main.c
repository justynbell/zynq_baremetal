#include <stdio.h>
#include <limits.h>

#include "xil_printf.h"
#include "xscuwdt.h"
#include "xscugic.h"
#include "xscutimer.h"
#include "sleep.h"

#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"

XScuWdt xWatchDogInstance;
XScuGic xInterruptController;
XScuTimer xTimer;

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
    (void)pcFile;
    (void)ulLine;

    for (;;);
}

void vInitialiseTimerForRunTimeStats( void )
{
    XScuWdt_Config *pxWatchDogInstance;
    uint32_t ulValue;
    const uint32_t ulMaxDivisor = 0xff, ulDivisorShift = 0x08;

    pxWatchDogInstance = XScuWdt_LookupConfig(XPAR_SCUWDT_0_DEVICE_ID);
    XScuWdt_CfgInitialize(&xWatchDogInstance, pxWatchDogInstance, pxWatchDogInstance->BaseAddr);

    ulValue = XScuWdt_GetControlReg(&xWatchDogInstance);
    ulValue |= ulMaxDivisor << ulDivisorShift;
    XScuWdt_SetControlReg(&xWatchDogInstance, ulValue);

    XScuWdt_LoadWdt(&xWatchDogInstance, UINT_MAX);
    XScuWdt_SetTimerMode(&xWatchDogInstance);
    XScuWdt_Start(&xWatchDogInstance);
}

static uint32_t tickHookCnt = 0;
void vApplicationTickHook( void )
{
    tickHookCnt++;
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace, xMinimumEverFreeHeapSpace;

    /* This is just a trivial example of an idle hook.  It is called on each
    cycle of the idle task.  It must *NOT* attempt to block.  In this case the
    idle task just queries the amount of FreeRTOS heap that remains.  See the
    memory management section on the http://www.FreeRTOS.org web site for memory
    management options.  If there is a lot of heap memory free then the
    configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
    RAM. */
    xFreeHeapSpace = xPortGetFreeHeapSize();
    xMinimumEverFreeHeapSpace = xPortGetMinimumEverFreeHeapSize();

    /* Remove compiler warning about xFreeHeapSpace being set but never used. */
    (void)xFreeHeapSpace;
    (void)xMinimumEverFreeHeapSpace;
}

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vConfigureTickInterrupt(void)
{
    BaseType_t xStatus;
    extern void FreeRTOS_Tick_Handler(void);
    XScuTimer_Config *pxTimerConfig;
    XScuGic_Config *pxGICConfig;
    const uint8_t ucRisingEdge = 3;

	/* This function is called with the IRQ interrupt disabled, and the IRQ
	interrupt should be left disabled.  It is enabled automatically when the
	scheduler is started. */

	/* Ensure XScuGic_CfgInitialize() has been called.  In this demo it has
	already been called from prvSetupHardware() in main(). */
	pxGICConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	xStatus = XScuGic_CfgInitialize(&xInterruptController, pxGICConfig, pxGICConfig->CpuBaseAddress);
	configASSERT(xStatus == XST_SUCCESS);
	(void)xStatus; /* Remove compiler warning if configASSERT() is not defined. */

	/* The priority must be the lowest possible. */
	XScuGic_SetPriorityTriggerType(&xInterruptController, XPAR_SCUTIMER_INTR, portLOWEST_USABLE_INTERRUPT_PRIORITY << portPRIORITY_SHIFT, ucRisingEdge);

	/* Install the FreeRTOS tick handler. */
	xStatus = XScuGic_Connect(&xInterruptController, XPAR_SCUTIMER_INTR, (Xil_ExceptionHandler) FreeRTOS_Tick_Handler, (void *)&xTimer);
	configASSERT(xStatus == XST_SUCCESS);
	(void)xStatus; /* Remove compiler warning if configASSERT() is not defined. */

	/* Initialise the timer. */
	pxTimerConfig = XScuTimer_LookupConfig(XPAR_SCUTIMER_DEVICE_ID);
	xStatus = XScuTimer_CfgInitialize(&xTimer, pxTimerConfig, pxTimerConfig->BaseAddr);
	configASSERT(xStatus == XST_SUCCESS);
	(void)xStatus; /* Remove compiler warning if configASSERT() is not defined. */

	/* Enable Auto reload mode. */
	XScuTimer_EnableAutoReload(&xTimer);

	/* Ensure there is no prescale. */
	XScuTimer_SetPrescaler(&xTimer, 0);

	/* Load the timer counter register. */
	XScuTimer_LoadTimer(&xTimer, (XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2UL) / configTICK_RATE_HZ);

	/* Start the timer counter and then wait for it to timeout a number of
	times. */
	XScuTimer_Start(&xTimer);

	/* Enable the interrupt for the xTimer in the interrupt controller. */
	XScuGic_Enable(&xInterruptController, XPAR_SCUTIMER_INTR);

	/* Enable the interrupt in the xTimer itself. */
	vClearTickInterrupt();
	XScuTimer_EnableInterrupt(&xTimer);
}

void vClearTickInterrupt(void)
{
	XScuTimer_ClearInterruptStatus(&xTimer);
}

static void prvSetupHardware(void)
{
    BaseType_t xStatus;
    XScuGic_Config *pxGICConfig;

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

int main()
{
    prvSetupHardware();

    while (1) {
        printf("Hello, jworld\n");
        sleep(1);
    }
}