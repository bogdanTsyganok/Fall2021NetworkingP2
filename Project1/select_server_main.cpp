/*
* Author:		Jarrid Steeper 0883583, Bogdan Tsyganok 0886354
* Class:		INFO6016 Network Programming
* Teacher:		Lukas Gustafson
* Project:		Project01
* Due Date:		Oct 22
* Filename:		select_server_main.cpp
* Purpose:		Server for chat applications, can host rooms, maintain connections with and 
				receive/send messages from multiple clients and act on those messages.
*/

#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cBuffer.h>
#include <map>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_HEADERLEN 8

//Commands that will be in the header of packets
enum Command
{
	Name = 1,
	Join = 2,
	Leave = 3,
	Message = 4
};

// Client structure
struct ClientInfo {
	SOCKET socket;

	// Buffer information (this is basically you buffer class)
	WSABUF dataBuf;
	cBuffer buffer = cBuffer(DEFAULT_BUFLEN);
	int bytesRECV;
	std::string name = "Anonymous";
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];
typedef std::multimap<std::string, int> roommap;
typedef std::multimap<std::string, int>::iterator roommapiterator;
roommap rooms;

void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index];
	closesocket(client->socket);
	printf("Closing socket %d\n", (int)client->socket);

	for (int clientIndex = index; clientIndex < TotalClients; clientIndex++)
	{
		ClientArray[clientIndex] = ClientArray[clientIndex + 1];
	}

	TotalClients--;

	// We also need to cleanup the ClientInfo data
	// TODO: Delete Client
}

int main(int argc, char** argv)
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		// Something went wrong, tell the user the error id
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		printf("WSAStartup() was successful!\n");
	}

	// #1 Socket
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() is good!\n");
	}

	// Create a SOCKET for connecting to the server
	listenSocket = socket(
		addrResult->ai_family,
		addrResult->ai_socktype,
		addrResult->ai_protocol
	);
	if (listenSocket == INVALID_SOCKET)
	{
		// -1 -> Actual Error Code
		// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		printf("socket() failed with error %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("socket() is created!\n");
	}

	// #2 Bind - Setup the TCP listening socket
	iResult = bind(
		listenSocket,
		addrResult->ai_addr,
		(int)addrResult->ai_addrlen
	);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("bind() is good!\n");
	}

	// We don't need this anymore
	freeaddrinfo(addrResult);

	// #3 Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("listen() was successful!\n");
	}

	// Change the socket mode on the listening socket from blocking to
	// non-blocking so the application will not block waiting for requests
	DWORD NonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	FD_SET ReadSet;
	int total;
	DWORD flags;
	DWORD RecvBytes;
	DWORD SentBytes = 0;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 2;
		// Initialize our read set
		FD_ZERO(&ReadSet);

		// Always look for connection attempts
		FD_SET(listenSocket, &ReadSet);

		// Set read notification for each socket.
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &ReadSet);
		}

		// Call our select function to find the sockets that
		// require our attention
		//printf("Waiting for select()...\n");
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			//printf("select() is successful!\n");
		}

		// #4 Check for arriving connections on the listening socket
		if (FD_ISSET(listenSocket, &ReadSet))
		{
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &NonBlock);
				if (iResult == SOCKET_ERROR)
				{
					printf("ioctsocket() failed with error %d\n", WSAGetLastError());
				}
				else
				{
					printf("ioctlsocket() success!\n");

					ClientInfo* info = new ClientInfo();
					info->socket = acceptSocket;
					info->bytesRECV = 0;
					ClientArray[TotalClients] = info;
					TotalClients++;
					printf("New client connected on socket %d\n", (int)acceptSocket);
				}
			}
		}

		// #5 recv & send
		for (int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];
			client->buffer.Flush();
			client->buffer.ResetSize(DEFAULT_BUFLEN);

			// If the ReadSet is marked for this socket, then this means data
			// is available to be read on the socket
			if (FD_ISSET(client->socket, &ReadSet))
			{
				total--;
				client->dataBuf.buf = (char *)client->buffer.GetBuffer();
				client->dataBuf.len = DEFAULT_BUFLEN;

				DWORD Flags = 0;
				iResult = WSARecv(
					client->socket,
					&(client->dataBuf),
					1,
					&RecvBytes,
					&Flags,
					NULL,
					NULL
				);

				//Steps to recieve a packet
				//1. Get the header out of the buffer
				//Packet size
				int packetSize = client->buffer.ReadIntBE();
				//Command type
				int commandtype = client->buffer.ReadIntBE();

				if (packetSize > RecvBytes)
				{
					client->buffer.ResetSize(packetSize);
					client->dataBuf.len = packetSize;
					client->dataBuf.buf = (char*)client->buffer.GetBuffer() + RecvBytes;


					DWORD Flags = 0;
					iResult = WSARecv(
						client->socket,
						&(client->dataBuf),
						1,
						&RecvBytes,
						&Flags,
						NULL,
						NULL
					);
				}

				std::string received;
				std::string roomName;


				cBuffer response(DEFAULT_BUFLEN);
				short messageLength;

				//2. Get the message out of the buffer
				switch (commandtype)
				{
				case 1://setname
				{
					messageLength = client->buffer.ReadShortBE();
					client->name = client->buffer.ReadStringBE(messageLength);
					break;
				}
				case 2: //join
				{
					//Room name
					messageLength = client->buffer.ReadShortBE();
					roomName = client->buffer.ReadStringBE(messageLength);
					rooms.insert(std::make_pair(roomName, i));

					std::string responseMessage = client->name + " has joined the room: " + roomName;

					response.ResetSize(responseMessage.length() + DEFAULT_HEADERLEN);
					response.WriteShortBE(responseMessage.length());
					response.WriteStringBE(responseMessage);

					response.AddHeader(commandtype);

					break;
				}
				case 3: //leave
				{
					messageLength = client->buffer.ReadShortBE();
					roomName = client->buffer.ReadStringBE(messageLength);

					for (roommapiterator it = rooms.begin(); it != rooms.end(); )
					{
						roommapiterator eraseIt = it++;
						if (eraseIt->second == i && eraseIt->first == roomName)
						{
							rooms.erase(eraseIt);
						}
					}

					std::string responseMessage = client->name + " has left room:" + roomName;

					response.WriteShortBE(responseMessage.size());
					response.WriteStringBE(responseMessage);

					response.AddHeader(commandtype);

					break;
				}
				case 4: //message
				{
					messageLength = client->buffer.ReadShortBE();
					roomName = client->buffer.ReadStringBE(messageLength);
					messageLength = client->buffer.ReadShortBE();
					received = client->buffer.ReadStringBE(messageLength);

					//Sender name
					response.WriteShortBE(client->name.size());
					response.WriteStringBE(client->name);

					//Room name
					response.WriteShortBE(roomName.size());
					response.WriteStringBE(roomName);

					//Message
					response.WriteShortBE(received.size());
					response.WriteStringBE(received);

					//Header
					response.AddHeader(commandtype);

					break;
				}
				default:
					break;
				}

				client->buffer.Flush();

				//2. Get the message out of the buffer
				/*short messageLength = client->buffer.ReadShortBE();

				std::string received = client->buffer.ReadStringBE(messageLength);*/

				//std::cout << "RECVd: " << received << std::endl;


				WSABUF resBuf;

				resBuf.buf = (char*)response.GetBuffer();
				resBuf.len = response.GetSize();

				if (iResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveClient(i);
					}
				}
				else
				{
					//Here we'll be sending responses back to clients
					printf("WSARecv() is OK!\n");
					if (RecvBytes == 0)
					{
						RemoveClient(i);
					}
					else if (RecvBytes == SOCKET_ERROR)
					{
						printf("recv: There was an error..%d\n", WSAGetLastError());
						continue;
					}
					else
					{
						roommapiterator it;
						switch (commandtype)
						{
						case 1:
							break;

						case 2:	//Join
						{
							it = rooms.find(roomName);
							// RecvBytes > 0, we got data

							while (it != rooms.end())
							{
								if (it->first != roomName)
									break;
									// RecvBytes > 0, we got data
									iResult = WSASend(
										ClientArray[it->second]->socket,
										&(resBuf),
										1,
										&SentBytes,
										Flags,
										NULL,
										NULL
									);
								it++;
							}
							break;
						}
						case 3:	//Leave
						{
							it = rooms.find(roomName);
							// RecvBytes > 0, we got data

							while (it != rooms.end())
							{
								if (it->first != roomName)
									break;
								// RecvBytes > 0, we got data
								iResult = WSASend(
									ClientArray[it->second]->socket,
									&(resBuf),
									1,
									&SentBytes,
									Flags,
									NULL,
									NULL
								);
								it++;
							}
							break;
						}
						case 4: //send message

							it = rooms.find(roomName);

							if (it != rooms.end())
							{
								while (it != rooms.end())
								{
									if (it->first != roomName)
										break;
									if (ClientArray[it->second] != client)
									{
										// RecvBytes > 0, we got data
										iResult = WSASend(
											ClientArray[it->second]->socket,
											&(resBuf),
											1,
											&SentBytes,
											Flags,
											NULL,
											NULL
										);
									}
									it++;
								}
							}
							if (SentBytes == SOCKET_ERROR)
							{
								printf("send error %d\n", WSAGetLastError());
							}
							else if (SentBytes == 0)
							{
								printf("Send result is 0\n");
							}
							else
							{
								printf("Successfully sent %d bytes!\n", SentBytes);
							}
							break;
						default:
							break;
						}
					}

						// Example using send instead of WSASend...
						//int iSendResult = send(client->socket, client->dataBuf.buf, iResult, 0);

						
				}
			}
		}

	}



	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();

	return 0;
}