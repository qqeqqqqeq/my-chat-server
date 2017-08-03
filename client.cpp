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

int comm_fd;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void *read(void* ptr)
{
	int nfds,n=0;
    fd_set fds;
    char buffer[256];
    int retval;
    while (1)
    {
		FD_ZERO(&fds);
		FD_SET(comm_fd,&fds);
		nfds=comm_fd+1;

		retval = select(nfds,&fds,NULL,NULL,NULL);
		if( -1 == retval )
		{
			printf("select error\n");
			break;
		}

		else if( 0 == retval )
		{
			printf("Timeout \n");
		}
		else
		{
			bzero(buffer, sizeof(buffer));
			n = read(comm_fd, buffer, sizeof(buffer));
			if (n < 0)
				error("ERROR reading from socket");
			else if (n>0)
			{
				printf("Receive a message :%s \n",buffer);
			}
		}
    }

    return 0;
}


void *write(void* ptr)
{

	int n=0;
	char buffer[256];

	while(1)
	{
		printf("Please enter the message: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		n = write(comm_fd,buffer,strlen(buffer));
		if (n < 0)
			error("ERROR writing to socket");
	}
}



void check_port(int a,char *b[])
{
    if (a < 3)
    {
       fprintf(stderr,"usage %s host name port\n", b[0]);
       exit(0);
    }
}

void check_fd(int a)
{
    if (a < 0)
        error("ERROR opening socket");
}



int main(int argc, char *argv[])
{
    int sockfd, portno,ret;

    pthread_t pid[2];
    struct sockaddr_in serv_addr;
    struct hostent *server;

    check_port(argc,argv);
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    check_fd(sockfd);


    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }


    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,
    		(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,
    		sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    comm_fd=sockfd;
    ret=pthread_create(&pid[1],NULL,read,NULL);
    if(ret!=0)
    	printf("Create read thread error!\n");

    ret=pthread_create(&pid[2],NULL,write,NULL);
    if(ret!=0)
    	printf("Create write thread error!\n");

    while(1)
    {

    }
    return 0;
}


