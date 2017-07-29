#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>


void error(const char *msg)
{
    perror(msg);
    exit(1);
}


void check_port(int portno)
{
	 if (portno < 2)
	    {
	    	fprintf(stderr,"ERROR, no port provided\n");
	    	exit(1);
	    }
}



void accept_event(epoll_event ev,epoll_event events[20],int lis_fd,int epfd )
{
	if(lis_fd<0)
	{
		error("accept fail");
	}
	ev.data.fd=lis_fd;
	ev.events=EPOLLIN;
	epoll_ctl(epfd,EPOLL_CTL_ADD,lis_fd,&ev);

}


void read_event(epoll_event ev,epoll_event events[20],int conn_fd,int i,int epfd,char buffer[256])
{
	printf("Message from : %d\n",conn_fd-4);
	printf("Here is the message: %s\n",buffer);
	ev.data.fd=conn_fd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,conn_fd,&ev);
}


void write_event( epoll_event ev,epoll_event events[20],int conn_fd,int i,int epfd)
{
	conn_fd=events[i].data.fd;
	write(conn_fd,"Message received",25);
	ev.data.fd=conn_fd;
	ev.events=EPOLLIN;
	epoll_ctl(epfd,EPOLL_CTL_MOD,conn_fd,&ev);
}


int main(int argc, char *argv[])
{

	int sockfd,portno,epfd,nfds,new_fd, listenfd,n;
	struct epoll_event ev,events[20];
	struct sockaddr_in serv_addr, cli_addr;
	char buffer[256];
    socklen_t clilen;



    check_port(argc);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
    	error("ERROR opening socket");
    }

 	epfd=epoll_create(20);
 	ev.data.fd=listenfd;
 	ev.events=EPOLLIN;
 	epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;


    if (bind(listenfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    	error("ERROR on binding");
    listen(listenfd,5);

    for(;;)
    {
    	nfds=epoll_wait(epfd,events,20,-1);
    	for(int i=0;i<nfds;i++)
    	{
    		if(events[i].data.fd==listenfd)
    		{
    			clilen=sizeof(cli_addr);
    			new_fd=accept(listenfd,(struct sockaddr *) &cli_addr, &clilen);
    			accept_event(ev,events,new_fd,epfd);
    			printf("accepted %d\n",new_fd-4);
    		}
    		else if(events[i].events==EPOLLIN)
    		{
    			bzero(buffer,256);
    			if((sockfd=events[i].data.fd)<0)
    				continue;
    			n=read(sockfd,buffer,256);
    			if (n==0)
    			{
    				close(sockfd);
    				events[i].data.fd=-1;
    				printf("Client %d is off line\n",sockfd-4);
    				continue;
    			}
    			read_event(ev,events,sockfd,i,epfd,buffer);
    		}
    		else if(events[i].events==EPOLLOUT)
    		{
    			write_event(ev,events,sockfd,i,epfd);
    		}
    	}
    }
    return 0;
}



