#ifndef _EPOLL_DEFINE_H
#define _EPOLL_DEFINE_H

#ifdef __cplusplus
//extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef __cpulusplus
//}
#endif


int create_tcp_server(){
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd == -1){
		perror(strerror(errno));
		return socketfd;
	}
	struct sockaddr_in my_addr;
	memset(&my_addr, '\0', sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(8828);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret = bind(socketfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr));

	if(-1 == ret){
		perror(strerror(errno));
		close(socketfd);
		return ret;
	}

	ret = listen(socketfd, 5);

	if(-1 == ret){
		perror(strerror(errno));
		close(socketfd);
		return ret;
	}
	return socketfd;
}

int accep_client_connect(const int socketfd){
	struct sockaddr_in client_addr;
	socklen_t addrlen = 0;
	memset(&client_addr, '\0', sizeof(struct sockaddr_in));
	int clientfd = accept(socketfd, (struct sockaddr*)&client_addr, &addrlen);
	if(clientfd == -1){
		perror(strerror(errno));
	}
	return clientfd;
}

int create_tcp_client(const char *server_ip, const int port){
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd == -1){
		perror(strerror(errno));
		return socketfd;
	}
	struct sockaddr_in my_addr;
	memset(&my_addr, '\0', sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = inet_addr(server_ip);

	int ret = connect(socketfd, (const struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	if(ret == -1){
		perror(strerror(errno));
		close(socketfd);
		return ret;
	}
	return socketfd;
}

ssize_t recv_data(int sockfd, char *buf, const size_t count){
	ssize_t read_size = 0;
	char *ptr = buf;
	size_t ptr_len = sizeof(buf);
	while(1){
		read_size = read(sockfd, (void*)ptr, ptr_len);
		if(-1 == read_size){
			perror(strerror(errno));
			return read_size;
		}
		if(0==read_size && 0 == strlen(buf)){
			break;
		}
		if(read_size < ptr_len){//隐患，若是读取的长度为当前可读取的最大长度，由于不知道是否读取结束，会继续执行read调用。此时输入内容正好是最大长度，调用read的时候由于没有数据可读，造成阻塞，直到再次有可读数据
			break;
		}
		ptr += read_size;
	}
	return strlen(buf);
}

ssize_t send_data(int sockfd, const char* buf, const size_t count){
	if(count == 0)	return count;
	ssize_t write_size = write(sockfd, (void *)buf, count);
	if(-1 == write_size){
		perror(strerror(errno));
	}
	return write_size;
}

#endif
