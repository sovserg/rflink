#ifndef _server_h
#define _server_h

extern int clientSocket; // if of client socket

int socket_send(const char* buf, const int size);

// start TCP server
int TCPserver(int port);

#endif
