/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 100 // data to be recieved

#define FILECHECK(c)        ((c) == '-' || (c) == '_' || (c) == '.' || ((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'z'))

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
    int numbytes;
    char commandBuf[MAXDATASIZE];
    char fileName[MAXDATASIZE];
    FILE * fd;
    int i, j;
    int fileFlag = 0;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept failed");
			continue;
		}
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if (send(new_fd, "\nConnected to Server...\nPlease Enter Command:\n", 47, 0) == -1) {
				perror("send failed");
                exit(1);
            }
            while(1)
            {
                if ((numbytes = recv(new_fd, commandBuf, MAXDATASIZE-1,0)) == -1) {
                    perror("recv failed");
                    exit(1);
                }
                commandBuf[numbytes] = '\0';
                printf("server: received command '%s' \n", commandBuf);
            
                if(strcmp("list",commandBuf) == 0)
                {
                    if(fork() == 0)
                    {
                        close(1);
                        
                        dup2(new_fd, 1);
                        
                        execl("/bin/ls", "ls", NULL);
                        printf("ls command failed\n");
                    }
                }
                /************* Check ************/
                else if(strncmp("check",commandBuf, 4) == 0)
                {
                    for( i = 6; commandBuf[i] != '\0'; i++)
                    {
                        fileName[i-6] = commandBuf[i];
                        printf("FileName: %s", fileName);
                        if( !FILECHECK(commandBuf[i]) )
                        {
                            fileFlag = 1;
                        }
                    }
                    
                    fd = fopen(fileName, "r");
                    if(fd != NULL && !fileFlag)
                    {
                        if (send(new_fd, "\nFile exists\nPlease Enter Command: ", 37, 0) == -1) {
                            perror("send failed");
                            exit(1);
                        }
                        fclose(fd);
                    }
                    else
                    {
                        if (send(new_fd, "\nFile does not exist\nPlease Enter Command: ", 46, 0) == -1) {
                            perror("send failed");
                            exit(1);
                        }
                    }
                }
                /************* Get ************/
                else if((strncmp("display",commandBuf, 6) == 0) || strncmp("download",commandBuf, 7) == 0)
                {
                    for ( i = 0 ; commandBuf[i] != ' ' ; i++)
                    {
                    }
                    i++;
                    j = i;
                    
                    for(;commandBuf[i] != '\0'; i++)
                    {
                        fileName[i-j] = commandBuf[i];
                        if( !FILECHECK(fileName[i-j]) )
                        {
                            fileFlag = 1;
                        }
                    }
                    fd = fopen(fileName, "r");
                    if(fd != NULL && !fileFlag)
                    {
                        fclose(fd);
                        if(fork() == 0)
                        {
                            close(1);
                            dup2(new_fd, 1);
                            
                            execl("/bin/cat", "cat", fileName, NULL);
                            printf("ls command failed\n");
                        }
                    }
                    else
                    {
                        if (send(new_fd, "\nFile does not exist\n", 24, 0) == -1) {
                            perror("send failed");
                            exit(1);
                        }
                    }
                }
                else if(strcmp("quit",commandBuf) == 0)
                {
                    if (send(new_fd, "\nCommand 'quit' received\nPlease enter command: ", 49, 0) == -1) {
                        perror("send failed");
                        exit(1);
                    }
                    printf("Closing connection with %s\n", s);
                    close(new_fd);
                    exit(0);
                }
                else
                {
                    if (send(new_fd, "\nUnknown Command received\nPlease enter command: ", 49, 0) == -1) {
                        perror("send failed");
                        exit(1);
                    }
                }
            }
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

