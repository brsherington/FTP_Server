/*
Ben Sherington
CS360 - Final: mftpserve.c
*/

#include "mftp.h"
#define portSize 16
#define writeSize 13
#define MY_PORT_NUMBER 49999

struct in_addr **pptr;

char* readBuffer(char word, char* buffer, int connectfd){
		register int i = 0;

		while(word != '\n'){
			buffer[i] = word;
			i++;
			read(connectfd, &word, 1);
		}

		buffer[i] = '\0';

		assert(buffer);
		return buffer;
}

int detectPort(int portNumber){
	int fix = 1;
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	const socklen_t optimal = sizeof(1);
	int errorFix = setsockopt(listenfd,SOL_SOCKET, SO_REUSEADDR, (void*) &fix, optimal);

	return listenfd;
}

struct sockaddr_in makeSocket(int portNumber){
		struct sockaddr_in servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_port	= htons(portNumber);
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		return servAddr;
}

char* selectPort(int datafd, struct sockaddr_in servAddr){
		if(bind(datafd,(struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
				perror("bind");
				exit(-1);
		}

		char* portNum = malloc(portSize);
		struct sockaddr_in portSocket;
		unsigned int socketSize = sizeof(portSocket);

		int test = getsockname(datafd, (struct sockaddr *) &portSocket, &socketSize);
		assert(test != -1);
		int port = ntohs(portSocket.sin_port);
		sprintf(portNum, "%d", port);

		return portNum;
}
//RCD command
int rcd(char* path){
		strtok(path, "\n");
		if(chdir(path) == -1){
			fprintf(stderr, "Error: Path does not exist.\n");
			return -1;
		}
		printf("Huzzah!\n");
		printf("Current directory has changed.\n");
		return 0;
}
void rls(int datafd){
		int forkCheck = fork();

		if(!forkCheck){
			close(1);
			dup(datafd);

			if(execlp("ls","ls","-l",NULL)){
				fprintf(stderr, "Error\n");
				exit(-1);
			}
		}
		printf("Huzzah!\n");
		printf("Current directory on server has been read.\n");
		wait(&forkCheck);
		return;
}
//GET command
void get(int datafd, int connectfd, char* path){
		int fd;
		int bytes;
		char commandBuffer[COMMAND];
		if((fd=open(path, O_RDONLY,0600)) < 0){
			write(connectfd, "EError!\n",20);
			return;
		}
		write(connectfd, "A\n", 2);

		while((bytes = read(fd,commandBuffer, COMMAND)) > 0){
			write(datafd, commandBuffer, bytes);
		}

		close(fd);
		return;
}

int put(int datafd, int connectfd, char* path){
	int fd;
	int bytes;
	char commandBuffer[COMMAND];

	if((fd=open(path, O_WRONLY|O_CREAT|O_EXCL,0600)) < 0){
		write(connectfd, "EError!",20);
		return -1;
	}

	write(connectfd, "A\n", 2);

	while((bytes = read(fd,commandBuffer, COMMAND)) > 0){
		write(datafd, commandBuffer, bytes);
	}

	close(fd);
	return 0;
}

//creates data connection
int dataConnection(int listenfd){
		struct sockaddr_in clientAddr;
		unsigned int sockLength = sizeof(struct sockaddr_in);
		int connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &sockLength);
		return connectfd;
}

int main(int argc, char** argv){

		char charBuffer;
		char buffer[COMMAND];
		char reply[COMMAND];
		char* portGrab;

		int port = 0;
		int listendatafd = 0;
		int datafd = 0;


		if(argv[1]){
			port = atoi(argv[1]);
		}

		if(!(argv[1])){
			port = MY_PORT_NUMBER;
		}
		int listenfd= detectPort(port);

		struct sockaddr_in server = makeSocket(port);
		struct sockaddr_in data;

		if(bind(listenfd,(struct sockaddr *) &server, sizeof(server)) < 0){
				perror("bind");
				exit(1);
		}

		listen(listenfd, 4);
		socklen_t socketLength = sizeof(struct sockaddr_in);

		struct sockaddr_in client;
		struct hostent* hostEntry;
		char* hostName;

		while (1) {
			int connectfd = accept(listenfd, (struct sockaddr *) &client, &socketLength);

			if (connectfd < 0){
				fprintf(stderr, "Error! Unable to connect.\n");
				return -1;
			}

			int forkCheck = fork();

			if (!forkCheck){
					hostEntry = gethostbyaddr(&(client.sin_addr), sizeof(struct in_addr), AF_INET);
					hostName = hostEntry->h_name;

					while(1){

						if (!read(connectfd, &charBuffer, 1)){
							printf("Error! The Connection was dropped.\n");
							return 0;
						}

						readBuffer(charBuffer, buffer, connectfd);

						//EXIT call
						if(buffer[0] == 'Q'){
							printf("Disconnecting from %s.\n",  hostName);
							write(connectfd, "A\n", 2);
							exit(0);
						}
						//GET call
						if(buffer[0] == 'G'){
							if(listendatafd == -1){
								printf("Error\n Data connection could not be established.\n");
							}
							else {
								get(datafd, connectfd, buffer+1);
								close(datafd);
								close(listendatafd);
								datafd = 0;
								listendatafd = datafd;
							}
						}
						//Create Data Connection
						if(buffer[0] == 'D'){
							listendatafd = detectPort(port);
							data = makeSocket(0);
							portGrab = selectPort(listendatafd, data);
							strcpy(reply, "A");
							strcat(reply, portGrab);
							strcat(reply, "\n");
							write(connectfd, reply, (int) strlen(reply));
							listen(listendatafd, 1);

							datafd = dataConnection(listendatafd);
						}

						//RLS call
						if(buffer[0] == 'L'){
							if(!listendatafd){
								printf("Error!\n Data connection could not be established.\n");
							}
							else {
								rls(datafd);
								write(connectfd, "A\n", 2);
								close(datafd);
								close(listendatafd);
								datafd = 0;
								listendatafd = datafd;
							}
						}

						//RCD call
						if(buffer[0] == 'C'){
							printf("%s", buffer);
							if ((rcd(buffer+1) != -1) ){
								write(connectfd, "A\n", 2);
							}
							else{
								write(connectfd, "ERCD failed.\n", writeSize);
							}
						}
				//PUT call
						if(buffer[0] == 'P'){
							if(!listendatafd){
								printf("Error\n Data connection could not be established.\n");
							}
							else {
								put(datafd, connectfd, buffer+1);
								close(datafd);
								close(listendatafd);
								datafd = 0;
								listendatafd = datafd;
							}
						}

					}
				}
				close(connectfd);
			}
			free(portGrab);
			return 0;
}
