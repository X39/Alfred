#pragma once


#ifdef _WIN32
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib,"WS2_32")
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#define INVALID_SOCKET -1
typedef int SOCKET;
#endif

//https://stackoverflow.com/questions/28027937/cross-platform-sockets

int socket_init(void);
int socket_cleanup(void);
int socket_close(SOCKET);
int socket_create_client(const char*, const char*, SOCKET*);