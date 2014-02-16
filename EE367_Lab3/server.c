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
#define MAXDATASIZE 1000 // data to be recieved

#define FILECHECK(c)        ((c) == '-' || (c) == '_' || (c) == '.' || ((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'z'))

void sigchld_handler(int s)
{
#ifdef DEBUGGER
    printf("DEBUG:calling sigchld_handler\n");
#endif
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
#ifdef DEBUGGER
    printf("DEBUG: calling get_in_addr\n");
#endif
	if (sa->sa_family == AF_INET) {
#ifdef DEBUGGER
        printf("DEBUG:38\n");
#endif
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
    char sendBuff[MAXDATASIZE];
    FILE * fd;
    int i;
    int fileFlag = 0;
    int c;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
#ifdef DEBUGGER
        printf("DEBUG:59");
#endif
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
#ifdef DEBUGGER
    printf("DEBUG: rv value = %d\n", rv);
#endif
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
#ifdef DEBUGGER
        printf("DEBUG:68\n");
#endif
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
#ifdef DEBUGGER
            printf("DEBUG:72");
#endif
			perror("server: socket");
			continue;
		}
#ifdef DEBUGGER
        printf("DEBUG86: sockfd = %d\n", sockfd);
#endif
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
#ifdef DEBUGGER
            printf("DEBUG:81");
#endif
			perror("setsockopt");
			exit(1);
		}
#ifdef DEBUGGER
        printf("DEBUG97: sockfd = %d\n", sockfd);
#endif
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
#ifdef DEBUGGER
            printf("DEBUG:90");
#endif
			close(sockfd);
			perror("server: bind");
			continue;
		}
#ifdef DEBUGGER
        printf("DEBUG108: sockfd = %d\n", sockfd);
#endif
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
#ifdef DEBUGGER
        printf("DEBUG:109");
#endif
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
#ifdef DEBUGGER
        printf("DEBUG:120");
#endif
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
			if (send(new_fd, "Connected to Server...\nPlease Enter Command:\n", 45, 0) == -1) {
				perror("send failed");
                exit(1);
            }
            while(1)
            {
                if ((numbytes = recv(new_fd, commandBuf, MAXDATASIZE,0)) == -1) {
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
                else if(strcmp("check",commandBuf) == 0)
                {
                    if (send(new_fd, "Command 'check' received\nPlease enter filename: ", 49, 0) == -1) {
                        perror("send failed");
                        exit(1);
                    }
                    if ((numbytes = recv(new_fd, commandBuf, MAXDATASIZE,0)) == -1) {
                        perror("recv failed");
                        exit(1);
                    }
                    for( i = 0; commandBuf[i] != '\0'; i++)
                    {
                        if( !FILECHECK(commandBuf[i]) )
                        {
                            fileFlag = 1;
                        }
                    }
                    fd = fopen(commandBuf, "r");
                    if(fd != NULL && !fileFlag)
                    {
                        if (send(new_fd, "File exists\n", 13, 0) == -1) {
                            perror("send failed");
                            exit(1);
                        }
                        fclose(fd);
                    }
                    else
                    {
                        if (send(new_fd, "File does not exist\n", 22, 0) == -1) {
                            perror("send failed");
                            exit(1);
                        }
                    }
                }
                else if(strcmp("display",commandBuf) == 0)
                {
                    if (send(new_fd, "Command 'display' received\nPlease enter filename: ", 49, 0) == -1) {
                        perror("send failed");
                        exit(1);
                    }
                    if ((numbytes = recv(new_fd, commandBuf, MAXDATASIZE,0)) == -1) {
                        perror("recv failed");
                        exit(1);
                    }
                    for( i = 0; commandBuf[i] != '\0'; i++)
                    {
                        if( !FILECHECK(commandBuf[i]) )
                        {
                            fileFlag = 1;
                        }
                    }
                    fd = fopen(commandBuf, "r");
                    if(fd != NULL && !fileFlag)
                    {
                        if(fork() == 0)
                        {
                            
                            close(1);
                            dup2(new_fd, 1);
                            
                            while(1)
                            {
                                c = fgetc(fd);
                                if(feof(fd))
                                    break;
                                putchar(c);
                            }
                            fclose(fd);
                            putchar('\0');
                            return 0;
                        }
                    }
                    else
                    {
                        if (send(new_fd, "File does not exist\n", 22, 0) == -1) {
                            perror("send failed");
                            exit(1);
                        }
                    }
                }
                else if(strcmp("download",commandBuf) == 0)
                {
                    if (send(new_fd, "Command 'download' received\nPlease enter command: ", 51, 0) == -1) {
                        perror("send failed");
                        exit(1);
                    }
                }
                else if(strcmp("quit",commandBuf) == 0)
                {
                    if (send(new_fd, "Command 'quit' received\nPlease enter command: ", 47, 0) == -1) {
                        perror("send failed");
                        exit(1);
                    }
                    printf("Closing connection with %s\n", s);
                    close(new_fd);
                    exit(0);
                }
                else
                {
                    if (send(new_fd, "Unknown Command received\nPlease enter command: ", 47, 0) == -1) {
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

