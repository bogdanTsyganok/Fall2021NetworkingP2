#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
 
int main(void)
{
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
 
    if (-1 == SocketFD)
    {
		perror("cannot create socket");
		exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(stSockAddr));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(8080);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);
 
    if (0 > Res)
    {
		perror("error: first parameter is not a valid address family");
		close(SocketFD);
		exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
		perror("char string (second parameter does not contain valid ipaddress)");
		close(SocketFD);
		exit(EXIT_FAILURE);
    }
 
    if (-1 == connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
    {
		perror("connect failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
    }
	
	char *packet = "Hello World";
	printf("sending \n");

	size_t bytesSent = send(SocketFD, packet, strlen(packet), 0);
	printf("bytesSent %zu \n", bytesSent);

	char buffer[256];
	size_t bytesReceived = recv(SocketFD, &buffer, sizeof(buffer), 0);
	printf("%s \n", buffer);

    (void) shutdown(SocketFD, SHUT_RDWR);
 
    close(SocketFD);
    return EXIT_SUCCESS;
}
