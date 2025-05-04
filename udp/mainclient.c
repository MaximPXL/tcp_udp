// udp_client_getaddrinfo.c

#ifdef _WIN32
    #define _WIN32_WINNT _WIN32_WINNT_WIN7 //select minimal legacy support, needed for inet_pton, inet_ntop
    #include <winsock2.h> //for all socket programming
    #include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
    #include <stdio.h> //for fprintf, printf
    #include <stdlib.h> //for exit
    #include <string.h> //for memset, strlen
    #include <io.h> //for _close
    #define close _close
#else
    #include <sys/socket.h> //for sockaddr, socket, recvfrom
    #include <sys/types.h> //for size_t
    #include <netdb.h> //for getaddrinfo
    #include <netinet/in.h> //for sockaddr_in
    #include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
    #include <errno.h> //for errno
    #include <stdio.h> //for fprintf, perror
    #include <unistd.h> //for close
    #include <stdlib.h> //for exit
    #include <string.h> //for memset
#endif

int main(int argc, char *argv[]) {
    //////////////////
    // Initialization
    //////////////////

    // Step 1.0 - Initialise Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }

    // Step 1.1 - Setup address info
    struct addrinfo hints;
    struct addrinfo *server_info = NULL;
    char server_ip[100];
    char server_port[] = "8888";

    printf("Enter server IP address: ");
    scanf("%s", server_ip);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(server_ip, server_port, &hints, &server_info) != 0) {
        fprintf(stderr, "getaddrinfo failed.\n");
        WSACleanup();
        exit(1);
    }

    // Step 1.2 - Create UDP socket
    int sockfd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "Could not create socket.\n");
        freeaddrinfo(server_info);
        WSACleanup();
        exit(1);
    }
    printf("Socket created.\n");

    // Set receive timeout (Step 1.3)
    DWORD timeout = 16000; // 16 seconds
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));


    /////////////
    // Execution
    /////////////

    char message[2000];
    char buffer[2000];
    int bytes_received;
    socklen_t addr_len = (socklen_t)server_info->ai_addrlen;

    while (1) {
        printf("Enter your guess: ");
        scanf("%s", message);

        // Step 2.1 - Send guess
        sendto(sockfd, message, (int)strlen(message), 0, server_info->ai_addr, server_info->ai_addrlen);

        // Step 2.2 - Receive reply
        bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, server_info->ai_addr, &addr_len);

        if (bytes_received == SOCKET_ERROR) {
            printf("You lost ?\n");
        } else {
            buffer[bytes_received] = '\0';
            printf("Server reply: %s\n", buffer);
        }
    }

    ////////////
    // Clean up
    ////////////

    // Step 3.2 - Free address info
    freeaddrinfo(server_info);

    // Step 3.1 - Close socket
    close(sockfd);

    // Step 3.0 - Cleanup Winsock
    WSACleanup();

    return 0;
}
