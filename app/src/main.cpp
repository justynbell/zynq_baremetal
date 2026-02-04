#include <stdio.h>

#include "xparameters.h"
#include "xstatus.h"
#include "netif/xadapter.h"
#include "xil_printf.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/init.h"

#include "qspi.h"

#define PLATFORM_EMAC_BASEADDR XPAR_XEMACPS_0_BASEADDR
#define THREAD_STACKSIZE 1024

static QSpiFlash qspi;

void echo_application_thread(void *);

static struct netif server_netif;
struct netif *echo_netif;

static void print_ip(char *msg, ip_addr_t *ip)
{
    xil_printf(msg);
    xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
            ip4_addr3(ip), ip4_addr4(ip));
}

static void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
    print_ip("Board IP: ", ip);
    print_ip("Netmask : ", mask);
    print_ip("Gateway : ", gw);
}

static void network_thread(void *p)
{
    struct netif *netif;
    ip_addr_t ipaddr, netmask, gw;
    int mscnt = 0;
    /* the mac address of the board. this should be unique per board */
    unsigned char mac_ethernet_address[6] = {0};

    xil_printf("\r\n\r\n");
    xil_printf("-----lwIP Socket Mode Echo server Demo Application ------\r\n");

    /* Read the mac from the Arty board */
    if (qspi.mac_read(mac_ethernet_address) != XST_SUCCESS) {
        xil_printf("Failed to read MAC address from QSPI flash. Using default:\n\r");
    } else {
        xil_printf("MAC address read from QSPI flash:\n\r");
    }

    xil_printf("0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n\r",
        mac_ethernet_address[0], mac_ethernet_address[1], mac_ethernet_address[2],
        mac_ethernet_address[3], mac_ethernet_address[4], mac_ethernet_address[5]
    );

    netif = &server_netif;

    /* Print out IP settings of the board */
    ipaddr.addr = 0;
    gw.addr = 0;
    netmask.addr = 0;

    /* Add network interface to the netif_list, and set it as default */
    if (!xemac_add(netif, &ipaddr, &netmask, &gw, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
        xil_printf("Error adding N/W interface\r\n");
        return;
    }

    netif_set_default(netif);

    /* Specify that the network if is up */
    netif_set_up(netif);

    /* Start packet receive thread - required for lwIP operation */
    sys_thread_new("xemacif_input_thread", (void(*)(void*))xemacif_input_thread, netif,
            THREAD_STACKSIZE,
            DEFAULT_THREAD_PRIO);

    dhcp_start(netif);

    while (1) {
        vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
        dhcp_fine_tmr();

        mscnt += DHCP_FINE_TIMER_MSECS;
        if (mscnt >= DHCP_COARSE_TIMER_SECS * 1000) {
            dhcp_coarse_tmr();
            mscnt = 0;
        }
    }

    return;
}

/* This thread waits for either a DHCP-assigned IP address, or
   falls back to a link-local IP address.
   When one is obtained, it'll spawn the *actual* application
   threads, and free itself. */
static int main_thread(void)
{
    /* Initialie the QSPI driver */
    qspi.init();

    /* initialize lwIP before calling sys_thread_new */
    lwip_init();

    /* any thread using lwIP should be created using sys_thread_new */
    sys_thread_new("NW_THRD", network_thread, NULL,
        THREAD_STACKSIZE,
            DEFAULT_THREAD_PRIO);

    while (1) {
        vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);

        if (dhcp_supplied_address(&server_netif)) {
            xil_printf("IP assigned via DHCP\r\n");
        } else if (autoip_supplied_address(&server_netif)) {
            xil_printf("IP assigned via Link-Local (AutoIP)\r\n");
        }

        if (server_netif.ip_addr.addr) {
            xil_printf("IP address received:\n\r");
            print_ip_settings(&(server_netif.ip_addr), &(server_netif.netmask), &(server_netif.gw));

            sys_thread_new("echod", echo_application_thread, 0,
                    THREAD_STACKSIZE,
                    DEFAULT_THREAD_PRIO);
            break;
        }
    }

    vTaskDelete(NULL);
    return 0;
}

/* Main entry point. Spawns the main thread */
int main()
{
    xil_printf("Starting application\n\r");

    sys_thread_new("main_thrd", (void(*)(void*))main_thread, 0,
                    THREAD_STACKSIZE,
                    DEFAULT_THREAD_PRIO);
    vTaskStartScheduler();

    while(1);
    return 0;
}