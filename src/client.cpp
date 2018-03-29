#include "epoll-define.h"
#include <stdlib.h>

int main(int argc, char*argv[]){
	if(argc != 3){
		printf("Usage:%s ip port\n", argv[0]);
		return -1;
	}
	int clientfd = create_tcp_client(argv[1], atoi(argv[2]));
	if(clientfd == -1){
		printf("create tcp client error\n");
		return -1;
	}
	char buf[1024];
	ssize_t size = -1;
	while(1){
		printf("input:");
		memset(buf, '\0', sizeof(buf));
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1]='\0';
		if(strcmp(buf, "exit") == 0){
			close(clientfd);
			break;
		}
		size = send_data(clientfd, buf, strlen(buf));
		printf("send data size = %d send data = %s\n", size, buf);
	}
	printf("client disconnect\n");
	return 0;
}
