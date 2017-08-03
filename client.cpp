/*
 * client.cpp
 *
 *  Created on: 2017-7-25
 *      Author: shen
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>

int g_connected_socketfd;

void *ReadSocket(void* ptr) {
	int nfds,n = 0;
    fd_set fds;
    char buffer[256];
    int retval;
    while (1) {
		FD_ZERO(&fds);
		FD_SET(g_connected_socketfd,&fds);
		nfds = g_connected_socketfd+1;
		retval = select(nfds,&fds,NULL,NULL,NULL);
		if (-1 == retval) {
			printf("select error\n");
			break;
		}

		else if ( 0 == retval ) {
			printf("Timeout \n");
		}

		else {
			bzero(buffer, sizeof(buffer));
			n = read(g_connected_socketfd, buffer, sizeof(buffer));
			if (n < 0) {
				printf("ERROR reading from socket\n");
				exit(EXIT_FAILURE);
			}

			else if (n > 0) {
				printf("Receive a message :%s \n",buffer);
			}
		}
    }
    return 0;
}


void *WriteSoctet(void* ptr) {
	int n = 0;
	char buffer[256];
	while (1) {
		printf("Please enter the message: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		n = write(g_connected_socketfd,buffer,strlen(buffer));
		if (n < 0) {
			printf("ERROR writing to socket");
			exit(EXIT_FAILURE);
		}
	}
}



void CheckPort(int portno,char *b[]) {
    if (portno < 3) {
       fprintf(stderr,"usage %s host name port\n", b[0]);
       exit(EXIT_FAILURE);
    }
}

void CheckFd(int socketfd) {
    if (socketfd < 0) {
        printf("ERROR opening socket");
        exit(EXIT_FAILURE);
    }
}



int main(int argc, char *argv[]) {
    int sockfd, portno,ret;
    pthread_t pid[2];
    struct sockaddr_in serv_addr;
    struct hostent *server;

    CheckPort(argc,argv);
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CheckFd(sockfd);
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,
    		(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Error connecting\n");
        exit(EXIT_FAILURE);
    }
    g_connected_socketfd = sockfd;
    ret = pthread_create(&pid[1],NULL,ReadSocket,NULL);
    if (ret != 0) {
    	printf("Create read thread error!\n");
    	exit(EXIT_FAILURE);
    }

    ret = pthread_create(&pid[2],NULL,WriteSoctet,NULL);
    if (ret != 0) {
    	printf("Create write thread error!\n");
    	exit(EXIT_FAILURE);
    }

    while (1) {}
    return 0;
}


