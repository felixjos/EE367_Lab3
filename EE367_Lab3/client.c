/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1000 // max number of bytes we can get at once

#define DEBUGGER 1
#define VALIDCMD(c) ((c) == 'a' || (c) == 'e' || (c) == 'i' || (c) == 'o' || (c) == 'u')
#define TERMCODE == "termcode"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void get_address(int *argc, char *addr)
{
    fprintf(stderr,"Please enter IP Address of Server: ");
    scanf("%s", addr);
    fprintf(stderr,"Connecting to Server Address: %s\n",addr);
    *argc = 2;
}

void get_command(char *command)
{
    scanf("%s", command);
}


int main()
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
    int argc;
    char address[100];
    char *addr = &address[0];
    char command[MAXDATASIZE];

    get_address(&argc, addr);
    
    fprintf(stderr, "Checking Argc value = %d\n", argc);
	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(&addr[0], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';
	printf("client: received '%s'\n",buf);

    while(1)
    {
        get_command(&command);
     
    if(strcmp(command,"help") == 0)
        printf("list: list files at the server\ncheck <file name>: checks if there is a file <file name>\ndisplay <file name>: Displays the contents of <file name>\ndownload <file name>: Download <file name>\nquit: Close the connection to the server\nPlease Enter Command:\n");
    else if (strcmp(command,"\n") != 0)
    {
        
        
        // Send Command
    if (send(sockfd, &command, MAXDATASIZE-1, 0) == -1) {
	    perror("send failed");
	}
    
        // Recieve reply
        numbytes = MAXDATASIZE-1;
        printf("numbytes: %d \n", numbytes);
        
        while(numbytes >= MAXDATASIZE-1)
        {
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            
            printf("while: numbytes: %d \n", numbytes);
            buf[numbytes] = '\0';
        }
        
        // download command
    if(strcmp(buf,"filedownload") == 0)
    {
        FILE *fd;
        fd = fopen(command, "w");
        
        numbytes = MAXDATASIZE;
        printf("numbytes: %d \n", numbytes);
        
        while(numbytes >= MAXDATASIZE-1)
        {
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
            
        printf("while: numbytes: %d \n", numbytes);
        buf[numbytes] = '\0';
        fprintf(fd, "%s", buf);
        }

        fclose(fd);
    }
    else
    {
            

	buf[numbytes] = '\0';
	printf("client: received '%s'\n",buf);
    
    
    if(strcmp(command,"quit") == 0)
	{
        close(sockfd);
        return 0;
    }
    }
        
    }
        
    }
    
	
}

