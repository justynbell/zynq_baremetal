#include <stdio.h>
#include <string.h>

#include "lwip/sockets.h"
#include "netif/xadapter.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"

#include "SteTcp.h"

#define THREAD_STACKSIZE 1024

static SteTcpServer tcpServer;
static SteTcpServer tcpCtrlServer;
static SteTcpServer tcpTelnet;

void echo_application_thread(void *)
{
	tcpServer.init(16154);
	tcpCtrlServer.init(16155);
	tcpTelnet.init(7);

	xil_printf("Waiting for connections from both ports\n\r");
	tcpServer.acceptConnection();
	xil_printf("Received connection from Data Port\n\r");
	tcpCtrlServer.acceptConnection();
	xil_printf("Received connection from Control Port\n\r");

	/////////////////////////////////////////////
	int RECV_BUF_SIZE = 8;
	uint8_t recv_buf[RECV_BUF_SIZE];
	int n, nwrote;

	while (1) {
		xil_printf("Waiting for telnet connection on port 7\n\r");
		tcpTelnet.acceptConnection();
		xil_printf("Received connection from Telnet port\n\r");

		while (1) {
			/* read a max of RECV_BUF_SIZE bytes from socket */
			if ((n = tcpTelnet.rx(recv_buf, RECV_BUF_SIZE)) < 0) {
				xil_printf("%s: error reading from Telnet socket, closing socket\r\n", __FUNCTION__);
				break;
			}

			/* break if client closed connection */
			if (n <= 0)
				break;

			xil_printf("Received:\n\r");
			for (uint8_t i = 0; i < n; i++) {
				xil_printf("%d ", recv_buf[i]);
			}
			xil_printf("\n\r");

			/* handle request */
			if ((nwrote = tcpTelnet.tx(recv_buf, n)) < 0) {
				xil_printf("%s: ERROR responding to client echo request. received = %d, written = %d\r\n",
						__FUNCTION__, n, nwrote);
				xil_printf("Closing socket\r\n");
				break;
			}
		}
	}

	/* close connection */
	tcpTelnet.closeClient();
	tcpTelnet.closeServer();

	xil_printf("Maximum number of connections reached, No further connections will be accepted\r\n");
	vTaskSuspend(NULL);
}
