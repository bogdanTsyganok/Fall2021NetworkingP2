#include "DBMaster.h"
#include "sha256.h"
#include "cSecurity.h"
#include "auth.pb.h"

DBMaster database;
#define WIN32_LEAN_AND_MEAN

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
#define DEFAULT_PORT "27016"
#define DEFAULT_HEADERLEN 8

//Commands that will be in the header of packets
enum Command
{
	Create = 101,
	Authenticate = 102
};

// Client structure
struct ClientInfo {
	SOCKET socket;

	// Buffer information (this is basically you buffer class)
	WSABUF dataBuf;
	cBuffer buffer = cBuffer(DEFAULT_BUFLEN);
	int bytesRECV;
	std::string name = "Anonymous Chat Server";
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];

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
	database.Connect("127.0.0.1", "root", "password");

	WSADATA wsaData;
	int iResult;

	//Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSA startup failed with error: " << iResult << std::endl;
	}
	else
	{
		std::cout << "WSA startu was successful!" << std::endl;
	}

	//Step 1: Sockets
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	//Address info
	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; //Need tcp for project 2
	hints.ai_flags = AI_PASSIVE;

	socklen_t addrlen;

	//Resolve the server address and port 
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		std::cout << "Obtain address info failed with error: " << iResult << std::endl;
	}
	else
	{
		std::cout << "Address info obtained" << std::endl;
	}

	//Create a socket for connecting to the auth server
	listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
		//Abort and clean up
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		std::cout << "Socket was created!" << std::endl;
	}
	//Step 2: Bind, setup the tcp listening socket
	iResult = bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
		//Abort and clean up
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		std::cout << "Bind was successful" << std::endl;
	}

	//No longer required
	freeaddrinfo(addrResult);

	//Step 3: Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Listen failed with error: " << WSAGetLastError();
		//Abort and clean up
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		std::cout << "Listen was successful!" << std::endl;
	}

	//Set non-blocking
	DWORD nonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &nonBlock);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Failed to set non blocking with error: " << WSAGetLastError() << std::endl;
		//Abort and clean up
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		std::cout << "Set socket to non blocking!" << std::endl;
	}

	//Enter listening loop
	FD_SET readSet;
	int total;
	DWORD flags;
	DWORD recvBytes;
	DWORD sentBytes = 0;
	bool listening = true;

	std::cout << "Entering loop to accept/receive/send" << std::endl;
	while (listening)
	{
		timeval tv = { 0 };
		tv.tv_sec = 2;
		// Initialize our read set
		FD_ZERO(&readSet);

		// Always look for connection attempts
		FD_SET(listenSocket, &readSet);

		// Set read notification for each socket.
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &readSet);
		}

		//Call our select function to find the sockets that
		//require our attention
		//std::cout << "Waiting for select..." << std::endl;
		total = select(0, &readSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			std::cout << "Select failed with error: " << WSAGetLastError() << std::endl;
			return 1;
		}
		else
		{
			//std::cout << "Select was successful!" << std::endl;
		}

		//Step 4: check for new connections on the listening socket
		if (FD_ISSET(listenSocket, &readSet))
		{
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				std::cout << "Accept failed with error: " << WSAGetLastError() << std::endl;
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &nonBlock);
				if (iResult == SOCKET_ERROR)
				{
					std::cout << "ioctlsocket failed with error: " << WSAGetLastError() << std::endl;
				}
				else
				{
					std::cout << "ioctlsocket was successful!" << std::endl;
					ClientInfo* info = new ClientInfo();
					info->socket = acceptSocket;
					info->bytesRECV = 0;
					ClientArray[TotalClients] = info;
					TotalClients++;
					std::cout << "New client connected on socket: " << (int)acceptSocket << std::endl;
				}
			}
		}

		//Step 5: Receive and send 
		for (int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];

			// If the ReadSet is marked for this socket, then this means data
			// is available to be read on the socket
			client->buffer.ResetSize(DEFAULT_BUFLEN);
			if (FD_ISSET(client->socket, &readSet))
			{
				total--;
				client->dataBuf.buf = (char*)client->buffer.GetBuffer();
				client->dataBuf.len = DEFAULT_BUFLEN;

				DWORD Flags = 0;
				iResult = WSARecv(
					client->socket,
					&(client->dataBuf),
					1,
					&recvBytes,
					&Flags,
					NULL,
					NULL
				);

				//Steps to receive a packet
				//1. Extract header from the buffer
				//Packet size
				int packetSize = client->buffer.ReadIntBE();
				//Command type
				int commandType = client->buffer.ReadIntBE();

				if (packetSize > recvBytes)
				{
					client->buffer.ResetSize(packetSize);
					client->dataBuf.len = packetSize;
					client->dataBuf.buf = (char*)client->buffer.GetBuffer() + recvBytes;


					DWORD Flags = 0;
					iResult = WSARecv(
						client->socket,
						&(client->dataBuf),
						1,
						&recvBytes,
						&Flags,
						NULL,
						NULL
					);
				}
				short messageLength;
				std::string serializedResponse = ""; 
				cBuffer responseBuffer(DEFAULT_BUFLEN);
				//Get the message out of the buffer
				switch (commandType)
				{
				case 101://Create account
				{
					int protobufsize = client->buffer.ReadShortBE();
					std::string rawproto = client->buffer.ReadStringBE(protobufsize);

					authentication::CreateAccountWeb createinfo;
					if (createinfo.ParseFromString(rawproto))
					{
						std::cout << "succesfully parsed proto";
					}
					else
					{
						std::cout << "could not parse proto";
					}

					std::string date;
					int id;
					CreateAccountWebResult dbresult = database.CreateAccount(createinfo.email(), createinfo.plaintextpass(), date, id);

					if (dbresult != SUCCESS)
					{
						authentication::CreateAccountWebFail response;
						response.set_reqid(createinfo.reqid());
						authentication::CreateAccountWebFail_Reason reason = authentication::CreateAccountWebFail_Reason::CreateAccountWebFail_Reason_INTERNAL_SERVER_ERROR;
						switch (dbresult)
						{
						case INTERNAL_SERVER_ERROR:
							reason = authentication::CreateAccountWebFail_Reason::CreateAccountWebFail_Reason_INTERNAL_SERVER_ERROR;
							break;
						case ACCOUNT_ALREADY_EXISTS:
							reason = authentication::CreateAccountWebFail_Reason::CreateAccountWebFail_Reason_ACCOUNT_ALREADY_EXISTS;
							break;
						}
						response.set_reason(reason);
						serializedResponse = response.SerializeAsString();

						responseBuffer.WriteShortBE(serializedResponse.size());
						responseBuffer.WriteStringBE(serializedResponse);

						responseBuffer.AddHeader(commandType + 10);
					}
					else
					{
						authentication::CreateAccountWebSuccess response;
						response.set_reqid(createinfo.reqid());
						response.set_userid(id);
						serializedResponse = response.SerializeAsString();

						responseBuffer.WriteShortBE(serializedResponse.size());
						responseBuffer.WriteStringBE(serializedResponse);

						responseBuffer.AddHeader(commandType);
					}

					break;
				}
				case 102://Authenticate 
				{
					int protobufsize = client->buffer.ReadShortBE();
					std::string rawproto = client->buffer.ReadStringBE(protobufsize);

					authentication::AuthenticateWeb clientinfo;
					if (clientinfo.ParseFromString(rawproto))
					{
						std::cout << "succesfully parsed proto";
					}
					else
					{
						std::cout << "could not parse proto";
					}

					std::string date;
					int id;
					CreateAccountWebResult dbresult = database.AuthenticateAccount(clientinfo.email(), clientinfo.plaintextpass(), date, id);
					if (dbresult != SUCCESS)
					{
						authentication::AuthenticateWebFail response;
						response.set_reqid(clientinfo.reqid());
						authentication::AuthenticateWebFail_Reason reason = authentication::AuthenticateWebFail_Reason::AuthenticateWebFail_Reason_INTERNAL_SERVER_ERROR;
						switch (dbresult)
						{
						case INTERNAL_SERVER_ERROR:
							reason = authentication::AuthenticateWebFail_Reason::AuthenticateWebFail_Reason_INTERNAL_SERVER_ERROR;
							break;
						case INVALID_CREDENTIALS:
							reason = authentication::AuthenticateWebFail_Reason::AuthenticateWebFail_Reason_INVALID_CREDENTIALS;
							break;
						}
						response.set_reason(reason);
						serializedResponse = response.SerializeAsString();

						responseBuffer.WriteShortBE(serializedResponse.size());
						responseBuffer.WriteStringBE(serializedResponse);

						responseBuffer.AddHeader(commandType + 10);
					}
					else
					{
						authentication::AuthenticateWebSuccess response;
						response.set_reqid(clientinfo.reqid());
						response.set_userid(id);
						response.set_creationdate(date);
						serializedResponse = response.SerializeAsString();
						
						responseBuffer.WriteShortBE(serializedResponse.size());
						responseBuffer.WriteStringBE(serializedResponse);

						responseBuffer.AddHeader(commandType);
					}

					break;
				}
				default:
					break;
				}//end of switch

				

				responseBuffer.GetBuffer();
				responseBuffer.GetSize();

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
					if (recvBytes == 0)
					{
						RemoveClient(i);
					}
					else if (recvBytes == SOCKET_ERROR)
					{
						printf("recv: There was an error..%d\n", WSAGetLastError());
						continue;
					}
					else
					{
						//Actual response

						int result = send(client->socket, (char*)responseBuffer.GetBuffer(), responseBuffer.GetSize(), 0);
						if (result == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(client->socket);
							WSACleanup();
							return 1;
						}
					}
				}
			}
		}
	}


	//Step 6: close and cleanup
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	//DB Stuff
	//database.Connect("127.0.0.1", "root", "password");
	//database.CreateAccount("test@gmail.com", "password");
	//system("Pause");
	return 0;
}