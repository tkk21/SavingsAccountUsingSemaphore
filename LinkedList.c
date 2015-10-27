#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"

void AddEndOfList (LinkedList *l, int val){
    LinkedListNode *next = (LinkedListNode *)malloc(sizeof(LinkedListNode));
    next->val = val;

    LinkedListNode *ptr = l->head;

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

int main(){
    LinkedListNode *root;
    LinkedList *list;
    root = malloc(sizeof(LinkedListNode));
    list = malloc(sizeof(LinkedList));

    root->val = 0;
    list->head = root;

    printContentOfLL(list);

    AddEndOfList(list, 1);
    printContentOfLL(list);
    printf("first request amount: %d", FirstRequestAmount(list));

    DeleteFirstRequest(list);
    printContentOfLL(list);
    


    return 0;
}
