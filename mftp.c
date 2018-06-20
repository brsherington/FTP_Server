/*
Ben Sherington
CS360 - Final: mftp.c
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mftp.h"

#define MY_PORT_NUMBER 49999
#define BUFFER 512
struct sockaddr_in servAddr;
struct hostent* hostEntry;
struct in_addr **pptr;
int socketfd;

char* getCommand(int socketfd){
  char* commandBuffer = malloc(COMMAND);
  register int i;
  char check;
  read(socketfd, &check, 1);
  for(i = 0; check != '\n'; i++){
      commandBuffer[i] = check;
      read(socketfd, &check, 1);
  }
  commandBuffer[i] = '\0';
  return commandBuffer;
}

int makeConnection(const char* server, int portNumber){
    struct sockaddr_in serverAddr;
    struct hostent* hostEntry;
    struct in_addr **pptr;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
      fprintf(stderr, "Error! Socket was unable to be opened,\n");
      return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNumber);

    hostEntry = gethostbyname(server);
    if(!hostEntry){
      fprintf(stderr, "Error! Host name could not be found!\n");
      return -1;
    }
    pptr = (struct in_addr **) hostEntry->h_addr_list;
    memcpy(&serverAddr.sin_addr, *pptr, sizeof(struct in_addr));

    if(connect(socketfd,(struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0){
      fprintf(stderr, "Error! Connection failed!\n");
      return -1;
    }

    return socketfd;
}

void ls(){
    int firstFork = 1;
    if((firstFork = fork())){
      wait(&firstFork);
    }
    else{
      int fd[2];
      pipe(fd);
      int secondFork = 1;
      int read = fd[0];
      int write = fd[1];
      if((secondFork = fork())){
        close(write);
        close(0);
        dup(read);
        close(read);
        execlp("more", "more", "-20", NULL);
      }else{
        close(read);
        close(1);
        dup(write);
        close(write);
        execlp("ls", "ls", "-l", NULL);
      }
    }
}

void rls(int fd){
    int firstFork = 1;
    if((firstFork= fork())){
      wait(&firstFork);
    }
    else{
      close(0);
      dup(fd);
      execlp("more","more", "-20", NULL);
    }
}

void cd(char* path){
    if(chdir(path) < 0){
      fprintf(stderr, "CD command failed. Not a vaild directory.\n");
    }else{
      chdir(path);
    }
}

void EXIT(){
    printf("Exiting\n");
    exit(0);
}

void get(int datafd, char* path){
    int fd;
    int readInput;
    char buffer[COMMAND];

      if((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0){
        fprintf(stderr, "Error Opening %s\n", path);
        return;
      }

    while((readInput = read(datafd, buffer, COMMAND))){
      write(fd,buffer, readInput);
    }

    close(fd);
    return;
}

int put(int datafd, int socketfd, char* word, char* path){
  int fd;
  int openRead;
  char* buffer;
  char* newBuffer;
    if((fd=open(path, O_RDONLY, 0600)) < 0){
      fprintf(stderr, "Error Opening %s\n", path);
      return -1;
    }
    write(socketfd, word, strlen(word));
    buffer = getCommand(socketfd);
    if(buffer[0] == 'E'){
      newBuffer = strcat(buffer+1, "\0");
      printf("%s\n", newBuffer);
      return -1;
    }
    else{
      while((openRead = read(fd, buffer, COMMAND)) > 0){
        write(datafd, buffer, openRead);
      }
    }

  close(fd);
  return 0;
}


int main(int argc, char const *argv[]){
    char* buffer;
    char* clientCommand = NULL;
    char serverCommand[COMMAND];
    char *path;
    char input[COMMAND];
    char *delimiters = " \n\t\v\f\r";
    int datafd = 0;

    if(!argv[1]){
      fprintf(stderr, "No Hostname detected. Please provide a Hostname.\n");
      return -1;
    }
    const char* server = argv[1];
    int socketfd = makeConnection(server, MY_PORT_NUMBER);

    int firstCommand = 0;
    while (1) {
      prompt:
        if(firstCommand != 0){
          printf("Enter a command: \n");
        }

        if(firstCommand == 0){
          printf("Enter a command: \n");
          firstCommand++;
        }

        fgets(input, COMMAND, stdin);
        printf("\n");

        if(input[0] == EOF){
          break;
        }

        if(input[0] == '\n'){
          goto prompt;
        }

        printf("%s", input);
        clientCommand = strtok(input, delimiters);

        //EXIT call
        if(!strcmp(clientCommand, "exit")){
            write(socketfd, "Q\n", 2);
            buffer = getCommand(socketfd);
            if(buffer[0] == 'A'){
              printf("Disconnecting\n");
              return 0;
            }
        }

        //CD call
        else if(strcmp(clientCommand, "cd") == 0){
          path = strtok(NULL, delimiters);
          cd(path);
          fflush(stdout);
        }
        // LS call
        else if(strcmp(clientCommand, "ls") == 0){
          ls();
          fflush(stdout);
        }
        //RCD call
        else if(strcmp(clientCommand, "rcd")==0){
          fflush(stdout);
          path = strtok(NULL, delimiters);
          if(!path){
            fprintf(stderr, "Error: Path does not exist\n");
            goto prompt;
          }
          strcpy(serverCommand, "C");
          strcat(serverCommand, path);
          strcat(serverCommand, "\n");
          printf("%s", serverCommand);
          write(socketfd, serverCommand, strlen(serverCommand));
          buffer = getCommand(socketfd);
          printf("%s\n", buffer);
          fflush(stdout);

          if(buffer[0] == 'E'){
            printf("%s\n", strcat(buffer+1, "\n"));
          }
        }
        //RLS Command
        else if(strcmp(clientCommand, "rls")==0){
          write(socketfd, "D\n", 2);
          buffer = getCommand(socketfd);
          fflush(stdout);
          assert(buffer);

          if(buffer[0] == 'E'){
            printf("%s\n", strcat(buffer+1, "\n"));
          }

          else{
            datafd = makeConnection(server, atoi(buffer+1));

            if(datafd < 0){
              return -1;
            }

            write(socketfd, "L\n", 2);
            buffer = getCommand(socketfd);
            assert(buffer);
            if(buffer[0] == 'A'){
              assert(datafd);
              rls(datafd);
            }
            else{
              fprintf(stderr, "Error: %s\n", strcat(buffer+1, "\n"));
            }
          }
        }
      //Get Command
      else if(strcmp(clientCommand, "get")==0){
        write(socketfd, "D\n",2);
        buffer = getCommand(socketfd);
        fflush(stdout);

        if(buffer[0] == 'E'){
          printf("%s\n", strcat(buffer+1, "\n"));
        }
        else{
          datafd = makeConnection(argv[1],atoi(buffer+1));
          path = strtok(NULL, delimiters);

            if(!path){
              fprintf(stderr, "Bad pathname\n");
              goto prompt;
            }

          strcpy(serverCommand, "G");
          strcat(serverCommand, path);
          strcat(serverCommand, "\n");
          printf("%s", serverCommand);
          write(socketfd, serverCommand, strlen(serverCommand));
          buffer = getCommand(socketfd);
          if(buffer[0] == 'A'){
            assert(datafd);
            assert(path);
            get(datafd, path);
            close(datafd);
          }
          else{
            fprintf(stderr, "Error: %s\n", strcat(buffer+1, "\n"));
          }
        }
      }
      //SHOW command
      else if(strcmp(clientCommand, "show")==0){
        fflush(stdout);
        write(socketfd, "D\n",2);
        buffer = getCommand(socketfd);

        if(buffer[0] == 'E'){
          printf("%s\n", strcat(buffer+1, "\n"));
        }

        else{
          datafd = makeConnection(argv[1],atoi(buffer+1));
          path = strtok(NULL, delimiters);

            if(!path){
              fprintf(stderr, "Bad pathname\n");
              goto prompt;
            }

          strcpy(serverCommand, "G");
          strcat(serverCommand, path);
          strcat(serverCommand, "\n");
          printf("%s", serverCommand);
          write(socketfd, serverCommand, strlen(serverCommand));
          buffer = getCommand(socketfd);

          if(buffer[0] == 'A'){
            get(datafd, path);
            rls(datafd);
          }
          else{
            fprintf(stderr, "Error: %s\n", strtok(buffer+1, "\n"));
          }
        }
      }
      //PUT command
      else if(strcmp(clientCommand, "put") == 0){
        printf("%s\n",buffer+1);
        fflush(stdout);
        write(socketfd, "D\n",2);
        printf("%s\n",buffer+1);
        fflush(stdout);
        buffer = getCommand(socketfd);

        if(buffer[0] == 'E'){
          printf("%s\n", strcat(buffer+1, "\n"));
        }

        else{
          datafd = makeConnection(server, atoi(buffer+1));
          path = strtok(NULL, delimiters);
            if(!path){
              fprintf(stderr, "Bad pathname\n");
              goto prompt;
            }

          strcpy(serverCommand, "P");
          strcat(serverCommand, path);
          strcat(serverCommand, "\n");
          put(datafd, socketfd, serverCommand, path);
          close(datafd);
        }
      }

      else{
        fprintf(stderr, "Please enter a valid command!\n");
      }

  }
  free(buffer);

 return 0;
}
