/*
* Author:		Jarrid Steeper 0883583, Bogdan Tsyganok 0886354
* Class:		INFO6016 Network Programming
* Teacher:		Lukas Gustafson
* Project:		Project01
* Due Date:		Oct 22
* Filename:		client_main.cpp
* Purpose:		Client for chat applications, can connect to and send messages to a server.
*/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cBuffer.h>
#include <conio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 10						// Default buffer length of our buffer in characters
#define DEFAULT_PORT "27015"					// The default port to use
#define SERVER "127.0.0.1"						// The IP of our server

//Commands that will be in the header of packets
enum class Command
{
	Name = 1,
	Join = 2,
	Leave = 3,
	Message = 4,
	Create = 101,
	Authenticate = 102
};

struct ClientInfo {
	ClientInfo() {};
	std::vector<std::string> roomList;
	std::string clientName = "";
};

ClientInfo* gClient = new ClientInfo();

void CreatePacket(cBuffer* buffer, Command type, std::vector< std::string> message);

int main(int argc, char **argv)
{
	WSADATA wsaData;							// holds Winsock data
	SOCKET connectSocket = INVALID_SOCKET;		// Our connection socket used to connect to the server

	struct addrinfo *infoResult = NULL;			// Holds the address information of our server
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	//const char *sendbuf = "Hello World!";		// The messsage to send to the server

	//char recvbuf[DEFAULT_BUFLEN];				// The maximum buffer size of a message to send
	cBuffer recvbuf(DEFAULT_BUFLEN);
	int result;									// code of the result of any command we use
	int recvbuflen = DEFAULT_BUFLEN;			// The length of the buffer we receive from the server
	bool isLoggedIn = false;

	// Step #1 Initialize Winsock
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed with error: %d\n", result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Step #2 Resolve the server address and port
	result = getaddrinfo(SERVER, DEFAULT_PORT, &hints, &infoResult);
	if (result != 0)
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}

	// Step #3 Attempt to connect to an address until one succeeds
	for (ptr = infoResult; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		DWORD NonBlock = 1;
		result = ioctlsocket(connectSocket, FIONBIO, &NonBlock);
		if (result == SOCKET_ERROR)
		{
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(infoResult);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	//Header
	std::cout << "Welcome to the chat server!" << std::endl
		<< "Type your command or /help for help" << std::endl;

	bool acceptInput = true;
	bool sendMsg = false;
	std::string msg;
	while(acceptInput)
	{
		if (_kbhit())
		{
			char key = _getch();

			if (key == 27)
			{
				acceptInput = false;
			}
			else
			if (key == '\r')
			{
				sendMsg = true;
				std::cout << std::endl;
			}
			else if (key == '\b' && !msg.empty())
			{
				msg.erase(msg.end() - 1);
				std::cout << "\b \b";
			}
			else if(true || (key <= 'z' && key >='a'))
			{
				msg += key;
				std::cout << key;
			}
		}

		if (sendMsg)
		{
			cBuffer* buffer = new cBuffer(DEFAULT_BUFLEN);
			std::vector<std::string> messageVec;
			//Check start of user input for the command
			if (isLoggedIn == true)
			{
				//Set Name
				if (msg.rfind("/name", 0) == 0)
				{
					msg.erase(0, 6);

					//Set locally
					gClient->clientName = msg;

					//Send new name to server
					messageVec.push_back(msg);
					CreatePacket(buffer, Command::Name, messageVec);

					messageVec.clear();
					msg.clear();
				}
				//Join room
				else if (msg.rfind("/join ", 0) == 0)
				{
					msg.erase(0, 6);
					messageVec.push_back(msg);
					gClient->roomList.push_back(msg);
					CreatePacket(buffer, Command::Join, messageVec);
					messageVec.clear();
					msg.clear();
				}
				//Leave room
				else if (msg.rfind("/leave ", 0) == 0)
				{
					//Clear command from the string
					msg.erase(0, 7);
					messageVec.push_back(msg);

					std::string roomName;
					for (int i = 0; i < gClient->roomList.size(); i++)
					{
						roomName = gClient->roomList.at(i);
						if (roomName == msg)
						{
							gClient->roomList.erase(gClient->roomList.begin() + i);

						}
					}

					CreatePacket(buffer, Command::Leave, messageVec);
					messageVec.clear();
					msg.clear();
				}
				//Send message to a room
				else if (msg.rfind("/message ", 0) == 0)
				{
					msg.erase(0, 9);

					size_t roomIndex = msg.find('[');
					size_t roomEnd = msg.find(']');
					if (roomIndex != std::string::npos)
					{
						std::string roomName = msg.substr(roomIndex + 1, roomEnd - 1);
						msg.erase(roomIndex, roomEnd + 2);
						messageVec.push_back(roomName);
					}


					messageVec.push_back(msg);
					CreatePacket(buffer, Command::Message, messageVec);
					messageVec.clear();
					msg.clear();
				}
				//Display help
				else if (msg.rfind("/help", 0) == 0)
				{
					std::cout << "Commands:" << std::endl;
					std::cout << "/name \"New Name\" :\t\t\tSet new display name (no quotation marks)" << std::endl
						<< "/join \"Room Name\" :\t\t\tJoin a room or create it if the name isn't in use (no quotation marks)" << std::endl
						<< "/leave \"Room Name\" :\t\t\tLeave a room (no quotation marks)" << std::endl
						<< "/message [Room Name] \"Message\" :\tSend a message to a room (Square brackets around room name required, quotations are not)" << std::endl
						<< "/help :\t\t\t\t\tRe-display this help text" << std::endl;
					msg.clear();
					sendMsg = false;
					continue;
				}
				//Display info
				else if (msg.rfind("/info", 0) == 0)
				{
					std::cout << "Information:" << std::endl;
					std::cout << "Username: " << gClient->clientName << std::endl;
					std::cout << "Rooms: " << std::endl;
					for (std::string room : gClient->roomList)
					{
						std::cout << room << std::endl;
					}
					msg.clear();
					sendMsg = false;
					continue;
				}
				//Invalid input
				else
				{
					std::cout << "Invalid input please use input:" << std::endl;
					std::cout << "/name" << std::endl << "/join" << std::endl
						<< "/leave" << std::endl << "/message" << std::endl
						<< "/help" << std::endl;
					msg.clear();
					sendMsg = false;
					continue;
				}
			}
			if (isLoggedIn == false)
			{
				if (msg.rfind("/register ", 0) == 0)
				{
					msg.erase(0, 10);
					//Check that there is only 1 white space 
					int count = std::count(msg.begin(), msg.end(), ' ');
					if (count > 1)
					{
						std::cout << "invalid input" << std::endl;
						msg.clear();
						sendMsg = false;
						continue;
					}
					else
					{
						//Find white space between email and password
						std::size_t spaceIndex = msg.find_first_of(" ");

						//Split email and password
						std::string email = msg.substr(0, spaceIndex);
						messageVec.push_back(email);

						std::string password = msg.substr(spaceIndex, msg.size());
						messageVec.push_back(password);

						//std::cout << msg << std::endl;
						CreatePacket(buffer, Command::Create, messageVec);
						messageVec.clear();
						msg.clear();
					}
				}
				else if (msg.rfind("/login ", 0) == 0)
				{
					msg.erase(0, 7);
					//Check that there is only 1 white space 
					int count = std::count(msg.begin(), msg.end(), ' ');
					if (count > 1)
					{
						std::cout << "invalid input" << std::endl;
						msg.clear();
						sendMsg = false;
						continue;
					}
					else
					{
						//Find white space between email and password
						std::size_t spaceIndex = msg.find_first_of(" ");
						
						//Split email and password
						std::string email = msg.substr(0, spaceIndex);
						messageVec.push_back(email);

						std::string password = msg.substr(spaceIndex, msg.size());
						messageVec.push_back(password);

						//std::cout << msg << std::endl;
						CreatePacket(buffer, Command::Authenticate, messageVec);
						messageVec.clear();
						msg.clear();
					}
				}
				//Display help
				else if (msg.rfind("/help", 0) == 0)
				{
					std::cout << "Commands:" << std::endl;
					std::cout << "/register \"Email\" \"Password\" :\t\t\Register an account withthe chat server (no quotation marks)" << std::endl
						<< "/login \"Email\" \"Password\" :\t\t\tLogin to the chat server with an existing account(no quotation marks)" << std::endl;
					msg.clear();
					sendMsg = false;
					continue;
				}
				//Invalid input
				else
				{
					std::cout << "Invalid input please use input:" << std::endl;
					std::cout << "/login" << std::endl << "/signup" << std::endl
						<< "/help" << std::endl;
					msg.clear();
					sendMsg = false;
					continue;
				}
			}


			// Step #4 Send the message to the server
			result = send(connectSocket, (char*)buffer->GetBuffer(), buffer->GetSize(), 0);
			if (result == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
			sendMsg = false;
		}
		recvbuf.Flush();
		recvbuf.ResetSize(DEFAULT_BUFLEN);
		// Step #6 Receive until the peer closes the connection
		result = recv(connectSocket, (char*)recvbuf.GetBuffer(), recvbuflen, 0);
		if (result > 0)
		{
			//1. Get the header out of the buffer
			//Packet size
			int packetSize = recvbuf.ReadIntBE();

			//Command type
			int commandtype = recvbuf.ReadIntBE();

			recvbuf.ResetSize(packetSize);

			if (packetSize > recvbuflen)
			{
				recv(connectSocket, (char*)recvbuf.GetBuffer() + recvbuflen, packetSize - recvbuflen, 0);
			}

			//2. Get the message out of the buffer
			switch (commandtype)
			{
			//Set name
			case 1:
			{
				break;
			}
			//Join
			case 2:
			{
				//Join room message
				short messageLength;
				messageLength = recvbuf.ReadShortBE();
				std::string message = recvbuf.ReadStringBE(messageLength);
				std::cout << message << std::endl;
				break;
			}
			//Leave
			case 3:
			{
				//Leave room message
				short messageLength;
				messageLength = recvbuf.ReadShortBE();
				std::string message = recvbuf.ReadStringBE(messageLength);
				std::cout << message << std::endl;
				break;
			}
			//Message
			case 4:
			{
				short messageLength;
				//Sender name
				messageLength = recvbuf.ReadShortBE();
				std::string senderName = recvbuf.ReadStringBE(messageLength);

				//Room name
				messageLength = recvbuf.ReadShortBE();
				std::string roomName = recvbuf.ReadStringBE(messageLength);

				//Message
				messageLength = recvbuf.ReadShortBE();
				std::string message = recvbuf.ReadStringBE(messageLength);

				std::cout << "[" << roomName << "] " << senderName << ": " << message << std::endl;

				break;
			}
			//Create account success
			case 101:
			{
				short messageLength;

				//Response
				messageLength = recvbuf.ReadShortBE();
				std::string response = recvbuf.ReadStringBE(messageLength);
				std::cout << response << std::endl;

				break;
			}
			//Log in success
			case 102:
			{
				short messageLength;

				//Response
				messageLength = recvbuf.ReadShortBE();
				std::string response = recvbuf.ReadStringBE(messageLength);
				std::cout << response << std::endl;

				//Log in
				isLoggedIn = true;
				break;
			}
			//Create account fail
			case 111:
			{
				short messageLength;

				//Response
				messageLength = recvbuf.ReadShortBE();
				std::string response = recvbuf.ReadStringBE(messageLength);
				std::cout << response << std::endl;
				break;
			}
			//Login fail
			case 112:
			{
				short messageLength;

				//Response
				messageLength = recvbuf.ReadShortBE();
				std::string response = recvbuf.ReadStringBE(messageLength);
				std::cout << response << std::endl;
				break;
			}
			default:
				break;
			}
		
		}
		else if (result == 0)
		{
			printf("Connection closed\n");
		}
		else if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
		}
	}

	// Step #5 shutdown the connection since no more data will be sent
	result = shutdown(connectSocket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	

	// Step #7 cleanup
	closesocket(connectSocket);
	WSACleanup();

	system("pause");

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