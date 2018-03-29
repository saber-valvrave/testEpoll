#include "epoll-define.h"
#include "pthread_handle.h"
#include <sys/epoll.h>
#include "static_list.h"
#include "public_function.h"

#define PTHREAD_MAX 10
#define MAXEVENTS 5

//#define TEST 1


int epoll_handle = 0;


typedef struct{
	unsigned int index_busy;
	unsigned int index_ilde;
	StaticList clientfds[MAXEVENTS];
	pthread_mutex_t mutex;
}ClientFdArray;

int recycle_client(StaticList *client, int epoll_handle);

void* handle_read(void *arg){
	StaticList *info = (StaticList*)arg;
	struct epoll_event event;
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	ssize_t size = recv_data(info->value.fd, buf, sizeof(buf));
	if(size < 0){
		printf("pthreadId:%d recv data error\n",info->value.fd);
	}else if(size == 0){
		printf("pthreadId:%d client disconnect\n",info->value.fd);
		close(info->value.fd);
		info->value.ok = 0;
	}else{
		printf("pthreadId:%ld  clientfd=%d recv data size =%d recv data=%s\n", pthread_self(), info->value.fd, size, buf);
		//每次接收客户端数据，需改读事件为写事件，在写事件结束后修改为读事件
		event.data.ptr = arg;
		event.events = 0|EPOLLOUT;
		if(epoll_ctl(epoll_handle, EPOLL_CTL_MOD, info->value.fd, &event) != 0){
			perror(strerror(errno));
			printf("mod event to write error\n");
		}
	}
	return NULL;
}

void* handle_write(void *arg){
	StaticList *info = (StaticList*)arg;
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	printf("server recv:");
	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "clientid=%d server recv data\n", info->value.fd);//发送内容暂时固定
	buf[strlen(buf)-1]='\0';
	if(strcmp(buf, "exit") == 0){
		printf("pthreadId:%d server disconnect\n",info->value.fd);
		close(info->value.fd);
		info->value.ok = 0;
		return NULL;
	}
	ssize_t size = send_data(info->value.fd, buf, strlen(buf));
	if(size < 0){
		printf("pthreadId:%d server recv data error\n",info->value.fd);
	}else if(size == 0){
		printf("pthreadId:%d server not recv data\n",info->value.fd);
	}else{
		printf("pthreadId:%ld  clientfd=%d server recv data size =%d recv data=[%s]\n", pthread_self(), info->value.fd, size, buf);
	}
	struct epoll_event event;
	event.data.ptr = arg;
	event.events = 0|EPOLLIN;
	if(epoll_ctl(epoll_handle, EPOLL_CTL_MOD, info->value.fd, &event) != 0){
		perror(strerror(errno));
		printf("mod event to read error\n");
		printf("pthreadId:%d server disconnect\n",info->value.fd);
		close(info->value.fd);
		info->value.ok = 0;
	}
	return NULL;
}

int main(){

	ClientFdArray array;
	array.index_busy = MAXEVENTS;
	array.index_ilde = 0;
	for(int i=0; i<MAXEVENTS; i++){
		array.clientfds[i].value.fd = -1;
		array.clientfds[i].value.ok = 0;
		array.clientfds[i].index = i+1;
	}
	pthread_mutex_init(&array.mutex, NULL);

	//int epoll_handle = epoll_create(MAXEVENTS);
	epoll_handle = epoll_create(MAXEVENTS);//为模拟测试方便，暂时使用全局变量
	if(epoll_handle == -1){
		perror(strerror(errno));
		return -1;
	}

	if(0 != create_pthread_pool(PTHREAD_MAX)){
		printf("create pthread pool error\n");
		return -1;
	}

	int socketfd = create_tcp_server();
	if(socketfd == -1){
		printf("create tcp server error\n");
		recycle_pthread_pool();
		return -1;
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = socketfd;

	printf("put socketfd to event\n");
	if(epoll_ctl(epoll_handle, EPOLL_CTL_ADD, socketfd, &event) != 0){
		perror(strerror(errno));
		printf("add event error\n");
		recycle_pthread_pool();
		return -1;
	}

	int ret = -1;
	int ready = -1;
	int clientfd = -1;
	struct epoll_event events[MAXEVENTS];

	while(1){
		//START_TIME
		ready = epoll_wait(epoll_handle, events, MAXEVENTS, 2000);
		//END_TIME
		if(ready == -1){
			perror(strerror(errno));
			printf("wait events error\n");
			continue;
		}
		printf("epoll_wait ready=%d  fd=%d\n", ready, ready > 0 ? events[0].data.fd : -1);
		for(int i=0; i<ready; i++){
			if(events[i].data.fd == socketfd){
				printf("start accept...\n");
				clientfd = accep_client_connect(events[i].data.fd);
				if(clientfd == -1){
					printf("accept client connect error\n");
					continue;
				}

				//printf("accept sucess: socketfd=%d  clientfd = %d\n", socketfd, clientfd);
				//pthread_mutex_lock(&array.mutex);
				//printf("apply_element ....\n");printfList(array.clientfds, MAXEVENTS);
				if(apply_element(&array.index_busy, &array.index_ilde, array.clientfds, MAXEVENTS) != 0){
					recycle_element(&array.index_busy, &array.index_ilde, array.clientfds,recycle_client, epoll_handle, MAXEVENTS);
					if(apply_element(&array.index_busy, &array.index_ilde, array.clientfds, MAXEVENTS) != 0){
						close(clientfd);
						printf("client is full\n");
						continue;
					}
				}
			    //printf("ilde:%d  ",array.index_ilde);printIndexList(array.index_ilde, array.clientfds, MAXEVENTS);
			   // printf("busy:%d  ",array.index_busy);printIndexList(array.index_busy, array.clientfds, MAXEVENTS);
				event.events = 0|EPOLLIN;
				array.clientfds[array.index_busy].value.fd = clientfd;
				array.clientfds[array.index_busy].value.ok = 1;
				event.data.ptr = (void*)(array.clientfds+array.index_busy);

				//pthread_mutex_unlock(&array.mutex);
				if(epoll_ctl(epoll_handle, EPOLL_CTL_ADD, clientfd, &event) != 0){
					perror(strerror(errno));
					printf("add event error\n");
					close(clientfd);	clientfd = -1;
					((StaticList*)event.data.ptr)->value.ok = -1;
					continue;
				}

#ifdef TEST
				if(event.events & EPOLLIN){
					printf("epoll---read\n");
				}else{
					printf("epoll---unknow\n");
				}
				event.events |= EPOLLOUT;
				if(epoll_ctl(epoll_handle, EPOLL_CTL_ADD, clientfd, &event) != 0){//再次添加导致失败：提示文件已存在
					perror(strerror(errno));
					printf("add event error\n");
					close(clientfd);	clientfd = -1;
					((StaticList*)event.data.ptr)->value.ok = -1;
					//continue;
				}
				if(event.events & EPOLLOUT){
					printf("epoll---write\n");
				}else{
					printf("epoll---unknow\n");
				}
#endif

			}else{
				if(events[i].events & EPOLLIN){
					printf("recv data...\n");
					ret = start_thread(handle_read, events[i].data.ptr);
					if (ret == -1) {
						close(events[i].data.fd);
						((StaticList*) events[i].data.ptr)->value.ok = 0;
						printf("server is busy\n");
					}
				}
				if(events[i].events & EPOLLOUT){
					printf("server recv data...\n");
					ret = start_thread(handle_write, events[i].data.ptr);
					if (ret == -1) {
						close(events[i].data.fd);
						((StaticList*) events[i].data.ptr)->value.ok = 0;
						printf("server is busy\n");
					}
				}
			}
		}
		//printf("recycle cleint\n");
		recycle_element(&array.index_busy, &array.index_ilde, array.clientfds,recycle_client, epoll_handle, MAXEVENTS);
	   //printf("ilde:%d  ",array.index_ilde);printIndexList(array.index_ilde, array.clientfds, MAXEVENTS);
	   //printf("busy:%d  ",array.index_busy);printIndexList(array.index_busy, array.clientfds, MAXEVENTS);
		//sleep(1);
		usleep(100);

	}

}


int recycle_client(StaticList *client, int epoll_handle){
	if (client->value.ok == 0) {
		printf("recycle_client = %d\n", client->value.fd);
		client->value.fd = -1;
		return 1;
	}
	if (client->value.ok == -1) {
		printf("%d\n", client->value.fd);
		client->value.fd = -1;
		client->value.ok = 0;
		return 1;
	}
	return 0;
}
