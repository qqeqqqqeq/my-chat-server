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

#define THR_NUM 5

struct thread_info{
	int thr_fd;
	bool thr_ck;
};
struct th_ep{
	int num;
	int thread_epfd;
}ep;
struct epoll_event ev,events[20];


thread_info THREAD_INFO[THR_NUM]={-1,true};
sem_t sem_id[THR_NUM];
int fd_list[10]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};






void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void create_sem()
{
    for(int i=0;i<THR_NUM;i++)
    {
    	int res = sem_init(&sem_id[i], 0, 0);
		if(res == -1)
		{
			perror("semaphore initialization failed\n");
			exit(EXIT_FAILURE);
		}
    }
}

void *thread(void* ptr)
{
	pthread_detach(pthread_self());
	int fd =-1,n,thread_no,epfd;
	char buffer[256],cli_id[256],offline_message[50];

	th_ep *thr=(th_ep *)ptr;
	thread_no=thr->num;
	epfd=thr->thread_epfd;
	printf("thread %d created \n",thread_no);

	THREAD_INFO[thread_no].thr_fd=1;

	while(1)
	{
		sem_wait(&sem_id[thread_no]);
		if(THREAD_INFO[thread_no].thr_fd!=-1)
		{
			fd=THREAD_INFO[thread_no].thr_fd;
			bzero(buffer,256);
			n=read(fd,buffer,256);


			if (n==0)
			{
				ev.data.fd=fd;
				ev.events=EPOLLIN| EPOLLONESHOT;
				epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
				for (int i=0;i<10;i++)
				{
					if(fd_list[i]==fd)
					{
						fd_list[i]=-1;
						close(fd);
						printf("Socket ID: %d is off line\n",fd);
						sprintf(offline_message,"\nUser ID: %d offline\n",fd);
						for (int a=0;a<10;a++)
						{
							if(fd_list[a]!=-1)
							{
								write(fd_list[a],offline_message,50);
							}
						}
						break;
					}
				}
				THREAD_INFO[thread_no].thr_fd=-1;
				THREAD_INFO[thread_no].thr_ck=true;
				continue;
			}
			ev.data.fd=fd;
			ev.events=EPOLLIN| EPOLLONESHOT;
			epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
			printf("Message from Socket ID: %d\n",fd);
			printf("Here is the message: %s\n",buffer);
			sprintf(cli_id, "\nMessage from user ID: %d \n",fd);
			strcat(cli_id, buffer);
			for ( int i=0;i<10;i++)
			{
				if(fd_list[i]!=-1)
				{
					if(fd_list[i]!=fd)
					{

						write(fd_list[i],cli_id,256);
					}
				}
			}
			THREAD_INFO[thread_no].thr_ck=true;
		}
		else
		{
			printf("Socket ID error \n");

		}
	}
	return 0;
}

void create_thread(pthread_t pid[],int ep_fd)
{
	int ret;
    for(int a=0;a<THR_NUM;a++)
    {
    	ep.num=a;
    	ep.thread_epfd=ep_fd;
    	ret=pthread_create(&pid[a],NULL,thread,&ep);
    	sleep(0.05);
    	if(ret!=0)
    		printf("Create thread %d error!\n",a);
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


int main(int argc, char *argv[])
{

	int portno,epfd,nfds,new_fd, listenfd;

	struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    pthread_t pid[THR_NUM];

    check_port(argc);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    	error("ERROR opening socket");

    create_sem();

 	epfd=epoll_create(20);
 	create_thread(pid,epfd);
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
    	nfds=epoll_wait(epfd,events,20,1000);
    	for(int i=0;i<nfds;i++)
    	{
    		if(events[i].data.fd==listenfd)
    		{
    			clilen=sizeof(cli_addr);
    			new_fd=accept(listenfd,(struct sockaddr *) &cli_addr, &clilen);
    			if(new_fd<0)
    				error("accept fail");
    			ev.data.fd=new_fd;
    			ev.events=EPOLLIN| EPOLLONESHOT;
    			epoll_ctl(epfd,EPOLL_CTL_ADD,new_fd,&ev);
    			printf("accepted Socket ID: %d\n",new_fd);
    			for (int a=0;a<10;a++)
    			{
    				if(fd_list[a]== -1 )
    				{
    					fd_list[a]=new_fd;
    					break;
    				}
    			}
    		}
    		else if(events[i].data.fd>0)
    		{
    		    int conn_fd;
    			conn_fd=events[i].data.fd;
    			for(int a=0;a<THR_NUM;a++)
    			{
    				if(THREAD_INFO[a].thr_ck==true)
    				{
    					THREAD_INFO[a].thr_fd=conn_fd;
    					sem_post(&sem_id[a]);
    					THREAD_INFO[a].thr_ck=false;
    					break;
    				}
    			}
    		}
    	}
    }
    return 0;
}




