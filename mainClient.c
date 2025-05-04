#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
	void OSInit(void) {
		WSADATA wsaData;
		int WSAError = WSAStartup(MAKEWORD(2, 0), &wsaData);
		if (WSAError != 0) {
			fprintf(stderr, "WSAStartup errno = %d\n", WSAError);
			exit(-1);
		}
	}
	void OSCleanup(void) {
		WSACleanup();
	}
	#define perror(string) fprintf(stderr, string ": WSA errno = %d\n", WSAGetLastError())
#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h>
	#include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
	void OSInit(void) {}
	void OSCleanup(void) {}
#endif

int initialization(char * server_ip);
void execution(int internet_socket);
void cleanup(int internet_socket);

int main(int argc, char *argv[]) {
	//////////////////
	// Initialization //
	//////////////////
	OSInit();

	char server_ip[100];
	printf("Enter server IP address: ");
	scanf("%99s", server_ip);

	int internet_socket = initialization(server_ip);

	/////////////
	// Execution //
	/////////////
	execution(internet_socket);

	////////////
	// Cleanup //
	////////////
	cleanup(internet_socket);
	OSCleanup();

	return 0;
}

int initialization(char * server_ip) {
	// Step 1.1
	struct addrinfo hints;
	struct addrinfo *result;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int getaddrinfo_return = getaddrinfo(server_ip, "8888", &hints, &result);
	if (getaddrinfo_return != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
		exit(1);
	}

	int internet_socket = -1;
	struct addrinfo *p;
	for (p = result; p != NULL; p = p->ai_next) {
		// Step 1.2
		internet_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (internet_socket == -1) {
			perror("socket");
			continue;
		}

		// Step 1.3
		if (connect(internet_socket, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
			close(internet_socket);
			internet_socket = -1;
			continue;
		}

		break; // connection successful
	}

	freeaddrinfo(result);

	if (internet_socket == -1) {
		fprintf(stderr, "socket: no valid socket address found\n");
		exit(2);
	}

	printf("Connected to server.\n");
	return internet_socket;
}

void execution(int internet_socket) {
	// Step 2: Guessing loop
	int guess, network_guess;
	char server_reply[2000];
	int recv_size;

	while (1) {
		printf("Enter your guess (-1 to quit): ");
		scanf("%d", &guess);

		network_guess = htonl(guess);

		int bytes_sent = send(internet_socket, (char *)&network_guess, sizeof(network_guess), 0);
		if (bytes_sent == -1) {
			perror("send");
			break;
		}

		recv_size = recv(internet_socket, server_reply, sizeof(server_reply) - 1, 0);
		if (recv_size == -1) {
			perror("recv");
			break;
		}

		server_reply[recv_size] = '\0';
		printf("Server response: %s\n", server_reply);

		if (strcmp(server_reply, "Correct") == 0 || guess == -1) {
			break;
		}
	}
}

void cleanup(int internet_socket) {
	// Step 3.2
	int shutdown_return = shutdown(internet_socket, SD_SEND);
	if (shutdown_return == -1) {
		perror("shutdown");
	}

	// Step 3.1
	close(internet_socket);
}
