#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "module_process.h"

#define BUFFER_SIZE 1024

static void  cmd_echoserver(struct cmd_arg *arg, int argc, char **argv)
{
    if(argc != 2) {
        printf("Usage : %s <port>\n",argv[0]);
        return ;
    }
    int socketFd, clientFd, receivedLength;
    int socketType = SOCK_STREAM;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    /* create struct sockaddr_in params */
    struct sockaddr_in serverAddr, clientAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(atoi(argv[1]));
    unsigned int clientAddrLen = sizeof(clientAddr);

    socketFd = socket(AF_INET, socketType, 0);
    if(socketFd < 0)
        printf("Open socket failed\n");
    if(bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        printf("Binding error\n");
    if(listen(socketFd, 5) < 0)
        printf("Listen error\n");

    clientFd = accept(socketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if(clientFd < 0) {
        printf("Accept error\n");
        return ;
    }
    receivedLength = recv(clientFd, buffer, BUFFER_SIZE, 0);
    printf("Received a message from %s:%u:  ; Message: %s", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port),buffer);
    send(clientFd, buffer, receivedLength, 0);
    close(clientFd);
    close(socketFd);

}

void init_cmd_iperf_tool(void)
{
    shell_cmd_register(cmd_echoserver, "echoserver", NULL, "Tcp echo server");
}
