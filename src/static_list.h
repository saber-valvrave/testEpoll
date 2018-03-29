//
//  static_list.h
//  static_list
//
//  Created by 魏乐 on 2018/3/22.
//  Copyright © 2018年 valvrave. All rights reserved.
//

#ifndef static_list_h
#define static_list_h


typedef struct{int ok; int fd;} ClientInfo; //ok=0:close 1:open -1:exception
typedef ClientInfo LIST_TYPE;
typedef unsigned int INDEX_TYPE;

typedef struct{
    INDEX_TYPE index;
    LIST_TYPE value;
}StaticList;

extern void printfList(const StaticList *const list, const unsigned int num);

extern void printIndexList(INDEX_TYPE index, const StaticList *const list, const unsigned int num);

extern void opterorEqual(LIST_TYPE* desc, LIST_TYPE *src);

extern int apply_element(INDEX_TYPE* busy, INDEX_TYPE* ilde, StaticList* list, const unsigned int tail);

extern void recycle_element(INDEX_TYPE* busy, INDEX_TYPE* ilde, StaticList* list,int (*recycle_handle)(StaticList *client, int handle), int handle, const unsigned int tail);

#endif /* static_list_h */
