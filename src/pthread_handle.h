#ifndef _PTHREAD_HANDLE_H
#define _PTHREAD_HANDLE_H

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

typedef int pthread_pool;

typedef struct {
	unsigned int index;
	int state; //-1:over 0:init 1:run
	void *arg;
	void* (*pthread_handle)(void*);
	pthread_pool * pool_head;
}pthread_member;

typedef struct {
	unsigned int count;
	pthread_member* member;
	pthread_pool* pool;
	pthread_t *pthreadid;
	pthread_mutex_t mutex;
}pthread_manager;


extern pthread_manager* _get_pthread_manager();



void _phtread_pool_init(pthread_pool *const pool, const unsigned int count){
	for(int i = 0; i < count; i++){
		*(pool+i) = -1;
	}
}

void _pthread_member_init(pthread_member *const member, pthread_pool *const pool, const unsigned int count){
	for(int i=0; i<count; i++){
		(member+i)->index = i;
		(member+i)->state = 0;
		(member+i)->arg = NULL;
		(member+i)->pthread_handle = NULL;
		(member+i)->pool_head = pool;
	}
}
void _pthread_pool_destroy(pthread_member *const member, const unsigned int count){
	for(int i=0; i<count; i++){
		(member+i)->state = -1;
	}
}

int _pthread_manager_init(const unsigned int count){
	if(!(count>0))	return -1;
	pthread_manager* manager = _get_pthread_manager();
	if(manager->count != 0)	return 1;

	manager->pool = (pthread_pool*)malloc(sizeof(pthread_pool) * count);
	if(manager->pool == NULL) return -1;
	manager->member = (pthread_member*)malloc(sizeof(pthread_member) * count);
	if(manager->member == NULL){
		free(manager->pool);manager->pool = NULL;
		return -1;
	}
	manager->pthreadid = (pthread_t*)malloc(sizeof(pthread_t) * count);
	if(manager->pthreadid == NULL){
		free(manager->pool); manager->pool = NULL;
		free(manager->member);manager->member = NULL;
		return -1;
	}

	if(pthread_mutex_init(&manager->mutex, NULL) != 0){
		free(manager->pool); manager->pool = NULL;
		free(manager->member);manager->member = NULL;
		free(manager->pthreadid);manager->pthreadid = NULL;
		return -1;
	}

	manager->count = count;

	_phtread_pool_init(manager->pool, count);
	_pthread_member_init(manager->member, manager->pool, count);

	return 0;
}

void _pthread_manager_destroy(){
	pthread_manager* manager = _get_pthread_manager();
	if(manager->count == 0) return;
	if(manager->pool != NULL){
		free(manager->pool);manager->pool=NULL;
	}
	if(manager->member != NULL){
		free(manager->member);manager->member=NULL;
	}
	if(manager->pthreadid != NULL){
		free(manager->pthreadid);manager->pthreadid=NULL;
	}
	pthread_mutex_destroy(&manager->mutex);
}

pthread_manager* _get_pthread_manager(){
	static pthread_manager static_manager;
	if(static_manager.count != 0)	return &static_manager;
	static_manager.count = 0;
	static_manager.member = NULL;
	static_manager.pool = NULL;
	return &static_manager;
}


void _pthread_pool_join(const unsigned int count=0){
	pthread_manager *manager = _get_pthread_manager();
	int num = count;
	if(num == 0)	num = manager->count;
	for(int j = num-1; j<0; j--){
		pthread_join(*(manager->pthreadid+j), NULL);
	}
}

int _malloc_handle_pthread_from_pool(){
	pthread_manager *manager = _get_pthread_manager();
	pthread_mutex_lock(&(manager->mutex));
	for(int i=0; i<manager->count; i++){
		if(*(manager->pool+i) == -1){
			*(manager->pool+i) = i;
			pthread_mutex_unlock(&(manager->mutex));
			return i;
		}
	}
	pthread_mutex_unlock(&(manager->mutex));
	return -1;
}
int _free_handle_pthread_to_pool(const unsigned int index){
	pthread_manager *manager = _get_pthread_manager();
	pthread_mutex_lock(&(manager->mutex));
	if(*(manager->pool+index) != index){
		pthread_mutex_unlock(&(manager->mutex));
		return -1;
	}
	*(manager->pool+index) = -1;
	pthread_mutex_unlock(&(manager->mutex));
	return 0;
}

int _check_available_pthread_from_pool(){
	pthread_manager *manager = _get_pthread_manager();
	pthread_mutex_lock(&(manager->mutex));
	for(int i=0; i<manager->count; i++){
		if(*(manager->pool+i) != -1){
			pthread_mutex_unlock(&(manager->mutex));
			return i;
		}
	}
	pthread_mutex_unlock(&(manager->mutex));
	return -1;
}

void* _pthread_handle(void *arg){
	pthread_member *member = (pthread_member*)arg;
	while(1){
		if(member->state == -1)	pthread_exit((void *)0);
		usleep(100);//wait 1ms
		//sleep(0);
		if(member->pthread_handle !=NULL){
			member->pthread_handle(member->arg);
			member->pthread_handle = NULL;
			member->arg = NULL;
			_free_handle_pthread_to_pool(member->index);
		}
	}
	return NULL;
}

int create_pthread_pool(const unsigned int count){

	if(_pthread_manager_init(count) != 0){
		_pthread_manager_destroy();
		return -1;
	}
	pthread_manager *manager = _get_pthread_manager();
	for(int i=0; i<manager->count; i++){
		if(0 != pthread_create(manager->pthreadid+i, NULL, _pthread_handle, (void *)(manager->member+i))){
			_pthread_pool_destroy(manager->member, manager->count);
			_pthread_pool_join(i);
			_pthread_manager_destroy();
			return -1;
		}
	}
	return 0;
}

void recycle_pthread_pool(){
	pthread_manager *manager = _get_pthread_manager();
	while(1){
		if(_check_available_pthread_from_pool() == -1)	break;
	}
	_pthread_pool_destroy(manager->member, manager->count);
	_pthread_pool_join();
	_pthread_manager_destroy();
}



int start_thread( void*(*handle)(void*), void* arg){
	int index = _malloc_handle_pthread_from_pool();
	if(index == -1)	return index;
	pthread_manager *manager = _get_pthread_manager();
	(manager->member+index)->arg = arg;
	(manager->member+index)->pthread_handle = handle;
	return 0;
}

#endif
