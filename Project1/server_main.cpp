///*
//* Author:		Jarrid Steeper 0883583, Bogdan Tsyganok 0886354
//* Class:		INFO6016 Network Programming
//* Teacher:		Lukas Gustafson
//* Project:		Project01
//* Due Date:		Oct 22 
//* Filename:		main.cpp
//* Purpose:		Contans the main function for the server
//*/
//
//#define WIN32_LEAN_AND_MEAN
//
//#include <windows.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <stdlib.h>
//#include <stdio.h>
//
//// Need to link with Ws2_32.lib
//#pragma comment (lib, "Ws2_32.lib")
//
//// #pragma comment (lib, "Mswsock.lib")
//#define DEFAULT_BUFLEN 512						// Default buffer length of our buffer in characters
//#define DEFAULT_PORT "27015"					// The default port to listen on
//
//int main(void) 
//{
//    WSADATA wsaData;							// holds Winsock data
//    int result;									// code of the result of any command we use
//
//    SOCKET listenSocket = INVALID_SOCKET;		// The socket used to listen for connections
//    SOCKET clientSocket = INVALID_SOCKET;		// The socket of our client
//	
//    struct addrinfo *info = NULL;				// address info 
//    struct addrinfo hints;						// address flags
//
//    int sendResult;								// result of our send command
//    char recvbuf[DEFAULT_BUFLEN];				// THe buffer to store our recv message
//    int recvbuflen = DEFAULT_BUFLEN;			// Max size of our recv buffer
//    
//    // Step #1Initialize Winsock
//    result = WSAStartup(MAKEWORD(2,2), &wsaData);
//    if (result != 0)
//	{
//        printf("WSAStartup failed with error: %d\n", result);
//        return 1;
//    }
//
//    ZeroMemory(&hints, sizeof(hints));
//    hints.ai_family = AF_INET;
//    hints.ai_socktype = SOCK_STREAM;
//    hints.ai_protocol = IPPROTO_TCP;
//    hints.ai_flags = AI_PASSIVE;
//
//    // Step #2 Resolve the server address and port
//    result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
//    if ( result != 0 )
//	{
//        printf("getaddrinfo failed with error: %d\n", result);
//        WSACleanup();
//        return 1;
//    }
//
//    // Step #3 Create a SOCKET for connecting to server
//    listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
//    if (listenSocket == INVALID_SOCKET)
//	{
//        printf("socket failed with error: %ld\n", WSAGetLastError());
//        freeaddrinfo(info);
//        WSACleanup();
//        return 1;
//    }
//
//    // Step #4 Setup the TCP listening socket
//    result = bind( listenSocket, info->ai_addr, (int)info->ai_addrlen);
//    if (result == SOCKET_ERROR)
//	{
//        printf("bind failed with error: %d\n", WSAGetLastError());
//        freeaddrinfo(info);
//        closesocket(listenSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    freeaddrinfo(info);
//
//    result = listen(listenSocket, SOMAXCONN);
//    if (result == SOCKET_ERROR)
//	{
//        printf("listen failed with error: %d\n", WSAGetLastError());
//        closesocket(listenSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    // Step #5 Accept a client socket
//    clientSocket = accept(listenSocket, NULL, NULL);
//    if (clientSocket == INVALID_SOCKET)
//	{
//        printf("accept failed with error: %d\n", WSAGetLastError());
//        closesocket(listenSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    // No longer need server socket
//    closesocket(listenSocket);
//
//    // Step #6 Receive until the peer shuts down the connection
//    do {
//
//        result = recv(clientSocket, recvbuf, recvbuflen, 0);
//        if (result > 0)
//		{
//            printf("Bytes received: %d\n", result);
//			printf("Message: %s\n", &recvbuf);
//
//        // Echo the buffer back to the sender
//            sendResult = send( clientSocket, recvbuf, result, 0 );
//            if (sendResult == SOCKET_ERROR)
//			{
//                printf("send failed with error: %d\n", WSAGetLastError());
//                closesocket(clientSocket);
//                WSACleanup();
//                return 1;
//            }
//            printf("Bytes sent: %d\n", sendResult);
//        }
//		else if (result == 0)
//		{
//			printf("Connection closing...\n");
//		}
//        else
//		{
//            printf("recv failed with error: %d\n", WSAGetLastError());
//            closesocket(clientSocket);
//            WSACleanup();
//            return 1;
//        }
//
//    } while (result > 0);
//
//    // Step #7 shutdown the connection since we're done
//    result = shutdown(clientSocket, SD_SEND);
//    if (result == SOCKET_ERROR)
//	{
//        printf("shutdown failed with error: %d\n", WSAGetLastError());
//        closesocket(clientSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    // cleanup
//    closesocket(clientSocket);
//    WSACleanup();
//
//	system("pause");
//
//    return 0;
//}