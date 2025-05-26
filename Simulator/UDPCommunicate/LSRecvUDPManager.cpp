#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <stdexcept>

#include "LSRecvUDPManager.h"

LSRecvUDPManager::LSRecvUDPManager()
{
	// Constructor implementation
}

LSRecvUDPManager::~LSRecvUDPManager()
{
	// Destructor implementation
	closeSocket();
}

void LSRecvUDPManager::closeSocket()
{
	if (ls_socket_ >= 0)
	{
		close(ls_socket_);
		ls_socket_ = -1;
	}
}

bool LSRecvUDPManager::LSSocketOpen(int port)
{
	// Create a socket
	ls_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (ls_socket_ < 0)
	{
		perror("socket");
		return false;
	}

	// Set the socket to non-blocking mode
	int flags = fcntl(ls_socket_, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl");
		closeSocket();
		return false;
	}
	if (fcntl(ls_socket_, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		perror("fcntl");
		closeSocket();
		return false;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(ls_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		closeSocket();
		return false;
	}

	// successfully bound the socket
	std::cout << "Socket bound to port " << port << std::endl;

	return true;
}

bool LSRecvUDPManager::receiveData(char *buffer, int bufferSize)
{
	// Receive data from the socket
	recv_len_ = recvfrom(ls_socket_, buffer, bufferSize, 0, (struct sockaddr *)&server_addr_, &addr_len_);
	if (recv_len_ < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("recvfrom");
			return false;
		}
	}
	else
	{
		std::cout << "ls udp size : " << recv_len_ << std::endl;
		buffer[recv_len_] = '\0'; // Null-terminate the received data
		std::cout << "Received data: " << buffer << std::endl;
	}

	return true;
}