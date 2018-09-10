// This is a sample from the Microsoft site that I stepped throught the tutorial with, so I could
// brush up on Windows Sockets programming concepts.
//

#include "server.h"

extern "C" {
#include "JQR.Debug.Core.h"
}

#pragma comment(lib, "Ws2_32.lib")

bool WINAPI initialize_winsock(LPWSADATA p_wsa_data, int* p_result)
{
	log_debug("In initialize_winsock");

	log_debug("initialize_winsock: Checking inputs...");

	// Input validation -- both the passed pointers must have valid addresses
	if (p_wsa_data == nullptr)
		return false;

	if (p_result == nullptr)
		return false;

	log_debug("initialize_winsock: Input checks passed.  Calling WSAStartup...");

	*p_result = WSAStartup(MAKEWORD(2, 2), p_wsa_data);
	if (*p_result != 0) {
		log_error("initialize_winsock: WSAStartup failed: %d", *p_result);

		log_debug("initialize_winsock: Done.");

		return false;
	}

	log_debug("initialize_winsock: The operation completed successfully.");

	log_debug("initialize_winsock: Done.");

	return true;
}

int _cdecl main() {
	log_debug("In main");

	auto i_result = 0;

	WSADATA wsa_data;
	ZeroMemory(&wsa_data, sizeof(wsa_data));

	auto listen_socket = INVALID_SOCKET;
	auto client_socket = INVALID_SOCKET;

	struct addrinfo *result = nullptr;
	struct addrinfo hints {};

	auto i_send_result = 0;
	char recvbuf[DEFAULT_BUFLEN];
	const auto recvbuflen = DEFAULT_BUFLEN;

	log_debug("main: Initializing the Winsock stack...");

	if (!initialize_winsock(&wsa_data, &i_result))
		return i_result;

	log_debug("main: Winsock stack has been initialized.");

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	i_result = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
	if (i_result != 0) {
		log_error("main: getaddrinfo failed with error: %d\n", i_result);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		log_error("main: socket failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	i_result = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	if (i_result == SOCKET_ERROR) {
		log_error("main: bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	i_result = listen(listen_socket, SOMAXCONN);
	if (i_result == SOCKET_ERROR) {
		log_error("main: listen failed with error: %d\n", WSAGetLastError());
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	client_socket = accept(listen_socket, nullptr, nullptr);
	if (client_socket == INVALID_SOCKET) {
		log_error("main: accept failed: %d\n", WSAGetLastError());
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	log_info("main: Connection has been made to us by a client.");

	log_debug("main: Throwing away the listening socket...");

	// No longer need server socket
	closesocket(listen_socket);

	log_debug("main: Listening socket has been closed.");

	log_debug("main: Attempting to receive data...");

	// Receive until the peer shuts down the connection
	do {
		i_result = recv(client_socket, recvbuf, recvbuflen, 0);

		log_debug("main: i_result = %d", i_result);

		if (i_result > 0) {
			log_debug("main: Bytes received: %d\n", i_result);

			// Echo the buffer back to the sender
			i_send_result = send(client_socket, recvbuf, i_result, 0);
			if (i_send_result == SOCKET_ERROR) {
				log_error("main: send failed with error: %d\n", WSAGetLastError());
				closesocket(client_socket);
				WSACleanup();
				return 1;
			}
			log_debug("main: Bytes sent: %d\n", i_send_result);
		}
		else if (i_result == 0)
			log_debug("main: Connection closing...\n");
		else {
			log_error("main: recv failed with error: %d\n", WSAGetLastError());
			closesocket(client_socket);
			WSACleanup();
			return 1;
		}
	} while (i_result > 0);

	// shutdown the connection since we're done
	i_result = shutdown(client_socket, SD_SEND);
	if (i_result == SOCKET_ERROR) {
		log_error("main: shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(client_socket);
		WSACleanup();
		return 1;
	}

	log_debug("main: Freeing system resources...");

	// cleanup
	closesocket(client_socket);
	WSACleanup();

	log_debug("main: The client socket and additional system resources have been freed.");

	log_debug("main: i_result = %d", i_result);

	log_debug("main: Done.");

	return i_result;
}