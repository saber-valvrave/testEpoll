#include "epoll-define.h"
#include <stdlib.h>
#include <sys/epoll.h>

#define MAXNUM 5


int main(int argc, char*argv[]){
	if(argc != 3){
		printf("Usage:%s ip port\n", argv[0]);
		return -1;
	}
	int epoll_handle = epoll_create(MAXNUM);
	if(epoll_handle == -1){
		perror(strerror(errno));
		return -1;
	}

	int clientfd = create_tcp_client(argv[1], atoi(argv[2]));
	if(clientfd == -1){
		printf("create tcp client error\n");
		return -1;
	}

	struct epoll_event event;
	event.events = 0|EPOLLOUT;
	event.data.fd = clientfd;

	printf("put socketfd to event\n");
	if(epoll_ctl(epoll_handle, EPOLL_CTL_ADD, clientfd, &event) != 0){
		perror(strerror(errno));
		printf("add event error\n");
		return -1;
	}

	struct epoll_event events[MAXNUM];
	char buf[1024];
	ssize_t size = -1;

	while(1){
		int count = epoll_wait(epoll_handle, events, MAXNUM, 2000);
		if(count == -1){
			perror(strerror(errno));
			printf("wait events error\n");
			continue;
		}
		printf("epoll_wait count=%d\n", count);
		for(int i =0; i<count; i++){
			if(events[i].events & EPOLLIN){
				printf("read ready....\n");
				size = recv_data(clientfd, buf, sizeof(buf));
				if(size < 0){
					printf("recv data error\n");
				}else if(size == 0){
					printf("server close connect\n");
					close(clientfd);
					continue;
				}else{
					printf("pclientfd=%d recv data size =%d recv data=%s\n",clientfd, size, buf);
					event.events = (event.events ^ EPOLLIN) | EPOLLOUT;
					if(epoll_ctl(epoll_handle, EPOLL_CTL_MOD, clientfd, &event) != 0){
						perror(strerror(errno));
						printf("mod event to write error\n");
					}
				}
			}else if(events[i].events & EPOLLOUT){
				printf("write ready....\n");
				//if(events[i].events & EPOLLOUT)	printf("write.....\n");
				//if(events[i].events & EPOLLIN)	printf("read.....\n");
				printf("input:");
				memset(buf, '\0', sizeof(buf));
				fgets(buf, sizeof(buf), stdin);
				buf[strlen(buf)-1]='\0';
				if(strcmp(buf, "exit") == 0){
					printf("client close connect\n");
					close(clientfd);
					continue;
				}
				size = send_data(clientfd, buf, strlen(buf));
				printf("send data size = %d send data = %s\n", size, buf);
				event.events = (event.events ^ EPOLLOUT) | EPOLLIN;
				if(epoll_ctl(epoll_handle, EPOLL_CTL_MOD, clientfd, &event) != 0){
					perror(strerror(errno));
					printf("mod event to read error\n");
				}
			}
		}
	}
	printf("client disconnect\n");
	return 0;
}
