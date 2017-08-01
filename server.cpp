#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <semaphore.h>

int thread_fd[2]={-1,-1},thread_ck[2]= {0,0};
sem_t sem_id[2];


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void creat_sem()
{
    for(int i=1;i<3;i++)
    {
    	int res = sem_init(&sem_id[i], 0, 0);
		if(res == -1)
		{
			perror("semaphore initialization failed\n");
			exit(EXIT_FAILURE);
		}
    }

}

void *thread_1(void* )
{
	int fd=-1,n;
	char buffer[256];
	while(1)
	{
		sem_wait(&sem_id[1]);
		thread_ck[1]=1;
		if(thread_fd[1]!=-1)
		{

		fd=thread_fd[1];
		bzero(buffer,256);
		n=read(fd,buffer,256);

		if (n==0)
		{
			close(fd);
			printf("Client %d is off line\n",fd-4);
			thread_fd[1]=-1;
			thread_ck[1]=0;
			continue;
		}

		printf("Message from : %d\n",fd-4);
		printf("Here is the message: %s\n",buffer);
		write(fd,"Message received",17);
		thread_ck[1]=0;
		}
	}
	return 0;
}

void *thread_2(void* )
{
	int fd=-1,n;
	char buffer[256];
	while(1)
	{
		sem_wait(&sem_id[2]);
		thread_ck[2]=1;
		if(thread_fd[2]!=-1)
		{

		fd=thread_fd[2];
		bzero(buffer,256);
		n=read(fd,buffer,256);

		if (n==0)
		{
			close(fd);
			printf("Client %d is off line\n",fd-4);
			thread_fd[2]=-1;
			continue;
		}

		printf("Message from : %d\n",fd-4);
		printf("Here is the message: %s\n",buffer);
		write(fd,"Message received",17);
		thread_ck[2]=0;
		}
	}
	return 0;
}

void creat_thread(pthread_t pid[1])
{
	int ret;
    ret=pthread_create(&pid[0],NULL,thread_1,NULL);
    if(ret!=0)
    {
        printf("Create thread 1 error!\n");
    }

    ret=pthread_create(&pid[1],NULL,thread_2,NULL);
    if(ret!=0)
    {
        printf("Create thread 2 error!\n");
    }

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


int main(int argc, char *argv[])
{

	int portno,epfd,nfds,new_fd, listenfd;
	struct epoll_event ev,events[20];
	struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    pthread_t pid[1];


    check_port(argc);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    	error("ERROR opening socket");

    creat_sem();

    creat_thread(pid);


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

    while(1)
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
    		    int conn_fd;
    			if((conn_fd=events[i].data.fd)<0)
    				continue ;
    			for(int a=1;a<3;a++)
    			{
    				if(thread_ck[a]==0)
    				{
    					thread_fd[a]=conn_fd;
    					sem_post(&sem_id[a]);
    					sleep(0.1);
    					break;
    				}
    			}
    		}
    	}
    }
    return 0;
}




