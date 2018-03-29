#ifndef _PUBLIC_FUNCTION_H_
#define _PUBLIC_FUNCTION_H_

#include <sys/time.h>

#define START_TIME { \
	struct timeval start; \
	struct timeval end; \
	gettimeofday(&start, NULL);


#define END_TIME \
	gettimeofday(&end, NULL); \
	printf("cost time=%lds\t%ldus\n", (end.tv_sec - start.tv_sec), (end.tv_usec - start.tv_usec)); \
}


#endif
