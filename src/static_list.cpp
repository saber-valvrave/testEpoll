//
//  static_list.c
//  static_list
//
//  Created by 魏乐 on 2018/3/22.
//  Copyright © 2018年 valvrave. All rights reserved.
//

#include <stdio.h>
#include "static_list.h"

void opterorEqual(LIST_TYPE* desc, LIST_TYPE *src){
    //*desc = *src;
}

void printfList(const StaticList *const list, const unsigned int num){
    for(int i=0; i<num; i++){
        printf("[%d---[%d]]\t",list[i].index, list[i].value.fd);
    }
    printf("\n");
}

void printIndexList(INDEX_TYPE index, const StaticList *const list, const unsigned int num){
    for(int i=index; i!=num; i=list[i].index){
        printf("[%d---[%d]]\t",list[i].index, list[i].value.fd);
    }
    printf("\n");
}

int apply_element(INDEX_TYPE* busy, INDEX_TYPE* ilde, StaticList* list, const unsigned int tail){
    if(*ilde == tail) return -1;
    INDEX_TYPE temp = *busy;
    *busy = *ilde;
    *ilde = (list+(*ilde))->index;
    (list+(*busy))->index = temp;
    return 0;
}

void recycle_element(INDEX_TYPE* busy, INDEX_TYPE* ilde, StaticList* list,int (*recycle_handle)(StaticList *client, int handle), int handle, const unsigned int tail){
    INDEX_TYPE temp = 0;
    INDEX_TYPE prev = *busy;
    for(int i = *busy; i!=tail;){
        if(i!=*busy && (list+prev)->index != i){
            prev = (list+prev)->index;
        }

        if(recycle_handle(list+i, handle) == 0){
            i=(list+i)->index;
            continue;
        }
        if(i==*busy){
           *busy = (list+i)->index;
            prev = *busy;
        }else{
            (list+prev)->index = (list+i)->index;
        }
        temp = *ilde;
        *ilde = i;
        i=(list+i)->index;
        (list+*ilde)->index = temp;
    }
}
