#include "networking.h"
#include <stdlib.h>
#include <string.h>


int socket_init(void)
{
#ifdef WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
	return 0;
#endif
}

int socket_cleanup(void)
{
#ifdef WIN32
	return WSACleanup();
#else
	return 0;
#endif
}
int socket_close(SOCKET sock)
{

	int status = 0;

#ifdef WIN32
	status = shutdown(sock, SD_BOTH);
	if (status == 0) { status = closesocket(sock); }
#else
	status = shutdown(sock, SHUT_RDWR);
	if (status == 0) { status = close(sock); }
#endif

	return status;

}
int socket_create_client(const char* ip, const char* port, SOCKET* outSocket)
{
	struct addrinfo *ptr = NULL, hints;
	int res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	res = getaddrinfo(ip, port, &hints, &ptr);
	if (res)
	{
		return 1;
	}
	*outSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	
	if (*outSocket == INVALID_SOCKET)
	{
		*outSocket = INVALID_SOCKET;
		freeaddrinfo(ptr);
		return 2;
	}

	res = connect(*outSocket, ptr->ai_addr, ptr->ai_addrlen);
	if (res)
	{
		freeaddrinfo(ptr);
		return 3;
	}
	freeaddrinfo(ptr);
	return 0;
}