#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>

#define COMMAND 4096

#ifndef MFTP_H
#define MFTP_H

//client
int makeConnection(const char* server, int portNumber);
void ls();
void rls(int fd);
void EXIT();

//server
struct sockaddr_in makeSocket(int listenfd);
void rls(int datafd);
int rcd(char* path);
int dataConnection(int listenfd);

#endif
