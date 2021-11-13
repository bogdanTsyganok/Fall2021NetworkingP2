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

#include "auth.pb.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define AUTH_PORT "27016"
#define DEFAULT_HEADERLEN 8
#define AUTH_SERVER "127.0.0.1"						// The IP of our server


//Commands that will be in the header of packets
enum Command
{
	Name = 1,
	Join = 2,
	Leave = 3,
	Message = 4,
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
	std::string name = "Anonymous";
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];
typedef std::multimap<std::string, int> roommap;
typedef std::multimap<std::string, int>::iterator roommapiterator;
roommap rooms;

void CreatePacket(cBuffer* buffer, Command type, std::vector< std::string> message);

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

	//Connection socket to auth server
	SOCKET connectSocket = INVALID_SOCKET;		// Our connection socket used to connect to the server

	struct addrinfo* auth_infoResult = NULL;			// Holds the address information of our server
	struct addrinfo* auth_ptr = NULL;
	struct addrinfo auth_hints;

	cBuffer auth_recvbuf(DEFAULT_BUFLEN);
	int auth_recvbuflen = DEFAULT_BUFLEN;			// The length of the buffer we receive from the server


	//For clients
	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	//Auth hints
	ZeroMemory(&auth_hints, sizeof(auth_hints));
	auth_hints.ai_family = AF_UNSPEC;
	auth_hints.ai_socktype = SOCK_STREAM;
	auth_hints.ai_protocol = IPPROTO_TCP;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Auth Step #2 Resolve the server address and port
	iResult = getaddrinfo(AUTH_SERVER, AUTH_PORT, &hints, &auth_infoResult);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//Auth Step #3 Attempt to connect to an address until one succeeds
	for (auth_ptr = auth_infoResult; auth_ptr != NULL; auth_ptr = auth_ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		connectSocket = socket(auth_ptr->ai_family, auth_ptr->ai_socktype, auth_ptr->ai_protocol);

		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(connectSocket, auth_ptr->ai_addr, (int)auth_ptr->ai_addrlen);
		DWORD NonBlock = 1;
		iResult = ioctlsocket(connectSocket, FIONBIO, &NonBlock);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(auth_infoResult);

	// Step #4 Send the message to the server
	//cBuffer* buffer = new cBuffer(DEFAULT_BUFLEN);
	//std::vector<std::string> messageVec;
	//messageVec.push_back("Hi");
	//CreatePacket(buffer, Command::Create, messageVec);
	//iResult = send(connectSocket, (char*)buffer->GetBuffer(), buffer->GetSize(), 0);
	/*if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}*/

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	else
	{
		std::cout << "connected to auth server!" << std::endl;
	}

	////Server acting as a server part
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
					//Create account
					case 101: 
					{
						///Protocol header -> email -> password
						messageLength = client->buffer.ReadShortBE();

						//Email
						std::string email = client->buffer.ReadStringBE(messageLength);
						messageLength = client->buffer.ReadShortBE();
						
						//Password
						std::string password = client->buffer.ReadStringBE(messageLength);
						
						authentication::CreateAccountWeb createbuffer;
						createbuffer.set_email(email);
						createbuffer.set_plaintextpass(password);
						createbuffer.set_reqid(i);

						std::string serializedprotobuf = createbuffer.SerializeAsString();

						response.WriteShortBE(serializedprotobuf.size());
						response.WriteStringBE(serializedprotobuf);

						response.AddHeader(commandtype);
						break;
					}
					//Authenticate account
					case 102:
					{
						///Protocol header -> email -> password
						messageLength = client->buffer.ReadShortBE();

						//Email
						std::string email = client->buffer.ReadStringBE(messageLength);
						messageLength = client->buffer.ReadShortBE();
						
						//Password
						std::string password = client->buffer.ReadStringBE(messageLength);
						authentication::AuthenticateWeb createbuffer;
						createbuffer.set_email(email);
						createbuffer.set_plaintextpass(password);
						createbuffer.set_reqid(i);

						std::string serializedprotobuf = createbuffer.SerializeAsString();

						
						response.WriteShortBE(serializedprotobuf.size());
						response.WriteStringBE(serializedprotobuf);

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
						//Create
						case 101: 
						{
							int result = send(connectSocket, (char*)response.GetBuffer(), response.GetSize(), 0);
							if (result == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}
						}
							break;
						//Authenticate
						case 102:
						{
							int result = send(connectSocket, (char*)response.GetBuffer(), response.GetSize(), 0);
							if (result == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}
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

		cBuffer recvbuf(DEFAULT_BUFLEN);

		int result = recv(connectSocket, (char*)recvbuf.GetBuffer(), recvbuf.GetSize(), 0);

		if (result > 0)
		{
			//1. Get the header out of the buffer
			//Packet size
			int packetSize = recvbuf.ReadIntBE();

			//Command type
			int commandtype = recvbuf.ReadIntBE();

			recvbuf.ResetSize(packetSize);

			if (packetSize > recvbuf.GetSize())
			{
				recv(connectSocket, (char*)recvbuf.GetBuffer() + recvbuf.GetSize(), packetSize - recvbuf.GetSize(), 0);
			}
			cBuffer responseBuffer(DEFAULT_BUFLEN);
			int reqId;
			//2. Get the message out of the buffer
			switch (commandtype)
			{
				//Set name
				case 101:
				{
					int protoLength = recvbuf.ReadShortBE();
					std::string serializedProto = recvbuf.ReadStringBE(protoLength);
					authentication::CreateAccountWebSuccess proto;
					proto.ParseFromString(serializedProto);
					std::string serverResponse = "Successfully registered!\n";
					reqId = proto.reqid();
					responseBuffer.WriteShortBE(serverResponse.size());
					responseBuffer.WriteStringBE(serverResponse);
					responseBuffer.AddHeader(commandtype);
					break;
				}
				//Join
				case 102:
				{
					int protoLength = recvbuf.ReadShortBE();
					std::string serializedProto = recvbuf.ReadStringBE(protoLength);
					authentication::AuthenticateWebSuccess proto;
					proto.ParseFromString(serializedProto);
					std::string serverResponse = "Successfully authenticated, account created on: " + proto.creationdate();
					reqId = proto.reqid();
					responseBuffer.WriteShortBE(serverResponse.size());
					responseBuffer.WriteStringBE(serverResponse);
					responseBuffer.AddHeader(commandtype);
					break; 
				}
				case 111: 
				{
					int protoLength = recvbuf.ReadShortBE();
					std::string serializedProto = recvbuf.ReadStringBE(protoLength);
					authentication::CreateAccountWebFail proto;
					proto.ParseFromString(serializedProto);
					std::string serverResponse = "Failed to register\n";
					reqId = proto.reqid();
					std::string reason;
					if (proto.reason() == authentication::CreateAccountWebFail_Reason::CreateAccountWebFail_Reason_ACCOUNT_ALREADY_EXISTS)
					{
						reason = "Account Already Exists";
					}
					else if(proto.reason() == authentication::CreateAccountWebFail_Reason::CreateAccountWebFail_Reason_INTERNAL_SERVER_ERROR)
					{
						reason = "504: Internal Server Error";
					}
					else if (proto.reason() == authentication::CreateAccountWebFail_Reason::CreateAccountWebFail_Reason_INVALID_PASSWORD)
					{
						reason = "Invalid Password";
					}
					serverResponse += "reason: " + reason;
					responseBuffer.WriteShortBE(serverResponse.size());
					responseBuffer.WriteStringBE(serverResponse);
					responseBuffer.AddHeader(commandtype);
					break;
				}
				case 112:
				{
					int protoLength = recvbuf.ReadShortBE();
					std::string serializedProto = recvbuf.ReadStringBE(protoLength);
					authentication::AuthenticateWebFail proto;
					proto.ParseFromString(serializedProto);
					std::string serverResponse = "Failed to login\n";
					reqId = proto.reqid();
					std::string reason;
					if (proto.reason() == authentication::AuthenticateWebFail_Reason::AuthenticateWebFail_Reason_INTERNAL_SERVER_ERROR)
					{
						reason = "Account Already Exists";
					}
					else if (proto.reason() == authentication::AuthenticateWebFail_Reason::AuthenticateWebFail_Reason_INVALID_CREDENTIALS)
					{
						reason = "504: Internal Server Error";
					}
					serverResponse += "reason: " + reason;
					responseBuffer.WriteShortBE(serverResponse.size());
					responseBuffer.WriteStringBE(serverResponse);
					responseBuffer.AddHeader(commandtype);
					break;
				}
			}//end of switch

			WSABUF resBuf;

			resBuf.buf = (char*)responseBuffer.GetBuffer();
			resBuf.len = responseBuffer.GetSize();


			DWORD Flags = 0;
			iResult = WSASend(
				ClientArray[reqId]->socket,
				&(resBuf),
				1,
				&SentBytes,
				Flags,
				NULL,
				NULL
			);

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

	//Auth step #5 cleanup
	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(connectSocket);
	closesocket(acceptSocket);
	WSACleanup();

	return 0;
}



/// <summary>
/// Takes type of commands and list of messages, creates a packet that will be fed into the buffer for serialization
/// </summary>
/// <param name="buffer"></param>
/// <param name="type"></param>
/// <param name="message"></param>
void CreatePacket(cBuffer* buffer, Command type, std::vector< std::string> message)
{
	for (std::string m : message)
	{
		//Steps to send a packet
		//1. Determine length of message and put in the buffer
		buffer->WriteShortBE(m.length());

		//2. Put message in the buffer
		buffer->WriteStringBE(m);
	}

	//3. Add header to the packet
	buffer->AddHeader((int)type);
	//Packet is ready to be sent
}