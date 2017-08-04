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


#define THREAD_NUMBER 5

struct ThreadInfo {
	int thr_socketfd;
	int thr_num;
	int thr_epfd;
	bool thr_check;
};
struct epoll_event ev,events[20];
struct ClientInfo {
	int cli_socketfd;
	char cli_name[10];
};

ThreadInfo g_thread_info[THREAD_NUMBER];
ClientInfo g_user_list[10];
sem_t g_sem_id[THREAD_NUMBER];

void ThreadIndoInit() {
	for(int i = 0; i < THREAD_NUMBER; i++) {
		g_thread_info[i].thr_socketfd = -1;
		g_thread_info[i].thr_num = -1;
		g_thread_info[i].thr_epfd = -1;
		g_thread_info[i].thr_check = true;
	}
}

void ClientInfoInit() {
	for(int i = 0; i < 10; i++) {
		g_user_list[i].cli_socketfd=-1;
		sprintf(g_user_list[i].cli_name," ");
	}
}

void CreateSem() {
	int res;
    for (int i = 0; i < THREAD_NUMBER; i++) {
    	res = sem_init(&g_sem_id[i], 0, 0);
		if (-1 == res) {
			printf("Semaphore initialization failed\n");
			exit(EXIT_FAILURE);
		}
    }
}

void *BoardcastThread(void* ptr) {
	pthread_detach(pthread_self());
	int fd = -1,cli_num;
	char buffer[256],full_message[256],offline_message[64],set_name_message[50];
	int thread_num = *(int *)ptr;
	int epfd = g_thread_info[thread_num].thr_epfd;

	printf("thread %d created \n",thread_num);
	while (1) {
		sem_wait (&g_sem_id[thread_num]);
		if (-1 != g_thread_info[thread_num].thr_socketfd) {
			fd = g_thread_info[thread_num].thr_socketfd;
			for (int n = 0; n < 10; n++) {
				if (g_user_list[n].cli_socketfd == fd) {
					cli_num = n;
					break;
				}
			}
			bzero(buffer,256);
			int n = read(fd,buffer,256);

			//user off line
			if (0 == n) {
				ev.data.fd = fd;
				ev.events = EPOLLIN| EPOLLONESHOT;
				epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
				g_user_list[cli_num].cli_socketfd =-1;
				close(fd);
				printf("User: %s is off line\n",g_user_list[cli_num].cli_name);
				sprintf(offline_message,"\nUser : %s off line\n",g_user_list[cli_num].cli_name);
				for (int a = 0; a < 10; a++) {
					if (-1 != g_user_list[a].cli_socketfd) {
						write(g_user_list[a].cli_socketfd,offline_message,50);
					}
				}
				g_thread_info[thread_num].thr_socketfd = -1;
				g_thread_info[thread_num].thr_check = true;
				continue;
			}

			//setting user name
			char *a = strstr(buffer,"set_name:");
			if(NULL != a) {
				strncpy(g_user_list[cli_num].cli_name,a+9,10);
				write(g_user_list[cli_num].cli_socketfd,"user id been set\n",20);
				sprintf(set_name_message,"\nSocket ID: %d set user name to %s\n",g_user_list[cli_num].cli_socketfd,g_user_list[cli_num].cli_name);
				printf("%s",set_name_message);
				for (int i = 0; i < 10; i++) {
					if (-1 != g_user_list[i].cli_socketfd && fd != g_user_list[i].cli_socketfd) {
						write(g_user_list[i].cli_socketfd,set_name_message,256);
					}
				}


				ev.data.fd = fd;
				ev.events = EPOLLIN| EPOLLONESHOT;
				epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
				continue;
			}

			//broadcast message
			ev.data.fd = fd;
			ev.events = EPOLLIN| EPOLLONESHOT;
			epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
			printf("Message from User: %s",g_user_list[cli_num].cli_name);
			printf(": %s\n",buffer);
			sprintf(full_message, "\nMessage from User: %s\n:",g_user_list[cli_num].cli_name);
			strcat(full_message, buffer);
			for (int i = 0; i < 10; i++) {
				if (-1 != g_user_list[i].cli_socketfd && fd != g_user_list[i].cli_socketfd) {
					write(g_user_list[i].cli_socketfd,full_message,256);
				}
			}
			g_thread_info[thread_num].thr_check = true;
		}
		else
			printf("Socket ID error \n");
	}
	return 0;
}

void CreateThread(pthread_t pid[],int ep_fd) {
	int ret;
    for (int a = 0; a < THREAD_NUMBER; a++) {
    	g_thread_info[a].thr_num = a;
    	g_thread_info[a].thr_epfd = ep_fd;
    	ret = pthread_create(&pid[a],NULL,BoardcastThread,&a);
    	sleep(0.05);
    	if (0 != ret)
    		printf("Create thread %d error!\n",a);
    }
}

void CheckPort(int portno) {
	 if (portno < 2) {
		 printf("Error no port provided\n");
		 exit(EXIT_FAILURE);
	 }
}

int CreateListenfd() {
	int lisfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == lisfd) {
		printf("Error opening socket\n");
		exit(EXIT_FAILURE);
	}
	return lisfd;
}


int main(int argc, char *argv[]) {
	int nfds,accepted_fd,conn_fd;
	char online_message[64];
	struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    pthread_t pid[THREAD_NUMBER];

    ThreadIndoInit();
    ClientInfoInit();
    CheckPort(argc);
    int listenfd = CreateListenfd();
    CreateSem();

 	int epfd = epoll_create(20);
 	CreateThread(pid,epfd);
 	ev.data.fd = listenfd;
 	ev.events = EPOLLIN;
 	epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    int portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    	printf("Error on binding\n");
    	exit(EXIT_FAILURE);
    }
    listen(listenfd,5);
    while (1) {
    	nfds = epoll_wait(epfd,events,20,1000);
    	for (int i = 0; i < nfds; i++) {
    		if (events[i].data.fd == listenfd) {
    			clilen = sizeof(cli_addr);
    			accepted_fd = accept(listenfd,(struct sockaddr *) &cli_addr, &clilen);
    			if (accepted_fd < 0) {
    				printf("Accept socket failed\n");
    				exit(EXIT_FAILURE);
    			}
    			ev.data.fd = accepted_fd;
    			ev.events = EPOLLIN | EPOLLONESHOT;
    			epoll_ctl(epfd,EPOLL_CTL_ADD,accepted_fd,&ev);
    			printf("accepted Socket ID: %d\n",accepted_fd);
				sprintf(online_message,"\nSuccessful connection, your ID is: %d\n",accepted_fd);
    			write(accepted_fd,online_message,64);
				sprintf(online_message,"\nUser ID: %d on line\n",accepted_fd);
    			for (int n = 0; n < 10; n++) {
    				if (-1 == g_user_list[n].cli_socketfd) {
    					g_user_list[n].cli_socketfd = accepted_fd;
    					for (int m=0; m <10; m++) {
    						if(-1 != g_user_list[m].cli_socketfd && accepted_fd != g_user_list[m].cli_socketfd) {
    							write(g_user_list[m].cli_socketfd,online_message,64);
    						}
    					}
    					break;
    				}
    			}
    		}
    		else if (events[i].data.fd > 0) {
    			conn_fd = events[i].data.fd;
    			for (int n = 0; n < THREAD_NUMBER; n++) {
    				if (g_thread_info[n].thr_check == true) {
    					g_thread_info[n].thr_socketfd = conn_fd;
    					sem_post(&g_sem_id[n]);
    					g_thread_info[n].thr_check = false;
    					break;
    				}
    			}
    		}
    	}
    }
    return 0;
}




