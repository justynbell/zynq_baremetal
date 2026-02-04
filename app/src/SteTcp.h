#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "lwip/sockets.h"

class SteTcpServer {
public:
    enum ErrorCode {
        PASS = 0,
        ERR_SOCK = -1,
        ERR_BIND = -2,
        ERR_LISTEN = -3,
    };

    ErrorCode init(uint16_t port)
    {
        memset(&servaddr, 0, sizeof(servaddr));

        if ((mSockFd = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            xil_printf("Error on socket create\n\r");
            return ERR_SOCK;
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        servaddr.sin_addr.s_addr = INADDR_ANY;

        if (lwip_bind(mSockFd, (struct sockaddr *)&servaddr, sizeof (servaddr)) < 0) {
            xil_printf("Failed to bind to socket\n\r");
            lwip_close(mSockFd);
            return ERR_BIND;
        }

        if (lwip_listen(mSockFd, 0) < 0) {
            xil_printf("Failed to listen on socket\n\r");
            lwip_close(mSockFd);
            return ERR_LISTEN;
        }

        return PASS;
    }

    bool acceptConnection(void)
    {
        socklen_t clientAddrlen = sizeof(clientaddr);

        if ((mConnFd = lwip_accept(mSockFd, (struct sockaddr *)&clientaddr, &clientAddrlen)) == -1) {
            xil_printf("Error on socket accept\n\r");
            return -1;
        }

        xil_printf("TCP Client connected from %s: %d\n\r", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        return true;
    }

    int32_t tx(uint8_t *buff, size_t len)
    {
        if (mConnFd) {
            return lwip_send(mConnFd, buff, len, 0);
        } else {
            xil_printf("Trying to send to disconnected client\n\r");
            return -1;
        }
    }

    int32_t rx(uint8_t *buff, size_t len)
    {
        return lwip_recv(mConnFd, buff, len, 0);
    }

    void closeClient(void)
    {
        xil_printf("Closing TCP connection\n\r");
        lwip_close(mConnFd);
        mConnFd = 0;
    }

    void closeServer(void)
    {
        xil_printf("Closing TCP listening socket\n\r");
        lwip_close(mSockFd);
        mSockFd = 0;
    }

private:
	int mSockFd = 0;
    int mConnFd = 0;
	int size;
	struct sockaddr_in servaddr, clientaddr;
};

#endif /* TCP_SERVER_H */