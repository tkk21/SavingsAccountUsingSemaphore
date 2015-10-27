
typedef struct LinkedListNode{
    int val;
    struct LinkedListNode *next;
}LinkedListNode;

typedef struct LinkedList{
    LinkedListNode *head;
}LinkedList;

void AddEndOfList(LinkedList *l, int val);
void DeleteFirstRequest(LinkedList *l);
int FirstRequestAmount(LinkedList *l);
