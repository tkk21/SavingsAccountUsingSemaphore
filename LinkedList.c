#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"

void AddEndOfList (LinkedList *l, int val){
    LinkedListNode *next = (LinkedListNode *)malloc(sizeof(LinkedListNode));
    next->val = val;

    LinkedListNode *ptr = l->head;
    if (!ptr){
        l->head = next;
        return;
    }
    while(ptr->next){
        ptr = ptr->next;
    }
    ptr->next = next;
}

void DeleteFirstRequest (LinkedList *l){
    LinkedListNode *next = l->head->next;
    free(l->head);
    l->head = next;
}

int FirstRequestAmount (LinkedList *l){
    return l->head->val;
}

void printContentOfLL (LinkedList *l){
    LinkedListNode *ptr = l->head;
    printf("content of LL is: ");
    while (ptr){
        printf("%d, ", ptr->val);
        ptr = ptr->next;
    }
    printf("\n");
}
