#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    int c, recv_size;
    int random_number, guess;

    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock initialized successfully.\n");

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Unable to create socket: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket successfully created.\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // <--- Note: accepts all incoming IP addresses
    server.sin_port = htons(8888); // Port 8888

    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Binding failed: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Binding successful.\n");

    listen(server_socket, 3);

    printf("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);

    client_socket = accept(server_socket, (struct sockaddr *)&client, &c);
    if (client_socket == INVALID_SOCKET) {
        printf("Connection accept failed: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Connection accepted from %s\n", inet_ntoa(client.sin_addr));

    srand(time(NULL));
    random_number = rand() % 100 + 1; // Number between 1-100
    printf("Generated number: %d\n", random_number);

    while (1) {
        recv_size = recv(client_socket, (char *)&guess, sizeof(guess), 0);
        if (recv_size == SOCKET_ERROR || recv_size == 0) {
            printf("Connection lost.\n");
            break;
        }

        guess = ntohl(guess); // Convert from network byte order

        printf("Client guessed: %d\n", guess);

        if (guess == -1) {
            printf("Client has exited.\n");
            break;
        } else if (guess > random_number) {
            send(client_socket, "Lower", 5, 0);
        } else if (guess < random_number) {
            send(client_socket, "Higher", 6, 0);
        } else {
            send(client_socket, "Correct", 7, 0);
            break;
        }
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}
