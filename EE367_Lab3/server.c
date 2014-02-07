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
#define MAXDATASIZE 2 // data to be recieved
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
#ifdef DEBUGGER
        printf("DEBUG-105:entering while loop\n");
#endif
		sin_size = sizeof their_addr;
#ifdef DEBUGGER
        printf("DEBUG: Accept...\n");
#endif
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
#ifdef DEBUGGER
        printf("DEBUG86: newfd = %d\n", new_fd);
#endif
		if (new_fd == -1) {
#ifdef DEBUGGER
            printf("DEBUG:new_fd = -1");
#endif
			perror("accept failed");
			continue;
		}
#ifdef DEBUGGER
        printf("DEBUG: inet_ntop...\n");
#endif
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
#ifdef DEBUGGER
            printf("DEBUG:fork = 0\n");
#endif
#ifdef DEBUGGER
            printf("DEBUG:closeing socket sockfd\n");
#endif
			close(sockfd); // child doesn't need the listener
#ifdef DEBUGGER
            printf("DEBUG86: sending msg to new_fd = %d\n", new_fd);
#endif
			if (send(new_fd, "Connected to Server...\nPlease Enter Command", 45, 0) == -1)
				perror("send failed");
            while(1)
            {
            if ((numbytes = recv(new_fd, commandBuf, MAXDATASIZE,0)) == -1) {
				perror("recv failed");
                exit(1);
            }
            commandBuf[numbytes] = '\0';
            printf("server: received command '%c' \n", commandBuf[0]);
            switch(commandBuf[0])
            {
                case 'a': {
                    if (send(new_fd, "A received\n", 10, 0) == -1)
                    perror("send failed");
                    break;
                }
                case 'e': {
                    if (send(new_fd, "Exiting Server\n", 10, 0) == -1)
                        perror("send failed");
                    printf("Closing Connection with %s", s);
                    close(new_fd);
                    exit(0);
                    break;
                }
                case 'i': {
                    if (send(new_fd, "I received\n", 10, 0) == -1)
                        perror("send failed");
                    break;
                }
                case 'o': {
                    if (send(new_fd, "O received\n", 10, 0) == -1)
                        perror("send failed");
                    break;
                }
                case 'u': {
                    if (send(new_fd, "U received\n", 10, 0) == -1)
                        perror("send failed");
                    break;
                }
                default: {
                    if (send(new_fd, "Unknown Command\n", 15, 0) == -1)
                        perror("send failed");
                    break;
                }
            }
            }
		}
#ifdef DEBUGGER
        printf("DEBUG179: Parent waiting to close new_fd\n");
#endif
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

