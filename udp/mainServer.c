// udp_server_getaddrinfo.c

#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h> //for rand
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
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
	#include <time.h>
	int OSInit( void ) {}
	int OSCleanup( void ) {}
#endif

int initialization();
void execution( int internet_socket );
void cleanup( int internet_socket );

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

	execution( internet_socket );

	////////////
	//Clean up//
	////////////

	cleanup( internet_socket );
	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "8888", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

void execution( int internet_socket )
{
	//Step 2.0 - Voorbereiding game
	srand( time( NULL ) );
	int random_number = rand() % 100 + 1;
	int best_guess = -9999;
	int best_diff = 9999;
	int timeout = 8000;

	printf("Generated number: %d\n", random_number);

	struct sockaddr_storage client_address;
	socklen_t client_address_length = sizeof client_address;
	char buffer[2000];

	while( 1 )
	{
		//Step 2.1 - Wacht op data met timeout
		fd_set readfds;
		FD_ZERO( &readfds );
		FD_SET( internet_socket, &readfds );

		struct timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = ( timeout % 1000 ) * 1000;

		printf("Waiting for guesses...\n");

		int activity = select( 0, &readfds, NULL, NULL, &tv );

		if( activity > 0 )
		{
			int recv_size = recvfrom( internet_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &client_address, &client_address_length );
			if( recv_size == -1 )
			{
				perror( "recvfrom" );
				continue;
			}

			buffer[recv_size] = '\0';
			int guess = atoi( buffer );

			char client_ip[NI_MAXHOST];
			getnameinfo((struct sockaddr *) &client_address, client_address_length, client_ip, sizeof(client_ip), NULL, 0, NI_NUMERICHOST);

			printf("Received guess: %d from %s\n", guess, client_ip);

			int diff = abs( guess - random_number );
			if( diff < best_diff )
			{
				best_diff = diff;
				best_guess = guess;
			}

			timeout = 8000;
		}
		else
		{
			//Step 2.2 - Eerste timeout
			printf("Timeout event.\n");

			if( best_diff == 0 )
			{
				sendto( internet_socket, "You won !", 9, 0, (struct sockaddr *) &client_address, client_address_length );
			}
			else
			{
				sendto( internet_socket, "You won ?", 9, 0, (struct sockaddr *) &client_address, client_address_length );
			}

			//Step 2.3 - Wacht op mogelijke late guess
			timeout = 16000;
			best_guess = -9999;
			best_diff = 9999;

			FD_ZERO( &readfds );
			FD_SET( internet_socket, &readfds );
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = ( timeout % 1000 ) * 1000;

			activity = select( 0, &readfds, NULL, NULL, &tv );

			if( activity <= 0 )
			{
				printf("Second timeout, server restart...\n");
				random_number = rand() % 100 + 1;
				printf("Generated new number: %d\n", random_number );
				timeout = 8000;
			}
			else
			{
				recvfrom( internet_socket, buffer, sizeof(buffer), 0, (struct sockaddr *) &client_address, &client_address_length );
				sendto( internet_socket, "You lost !", 10, 0, (struct sockaddr *) &client_address, client_address_length );

				random_number = rand() % 100 + 1;
				printf("Generated new number: %d\n", random_number );
				timeout = 8000;
			}
		}
	}
}

void cleanup( int internet_socket )
{
	//Step 3.1
	close( internet_socket );
}
