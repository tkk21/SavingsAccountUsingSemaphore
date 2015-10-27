
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define SEMAPHORE_KEY 0xFA2B

//Index of the semaphores in the semaphore array
#define SEMAPHORE_MUTEX 0
#define SEMAPHORE_wlist 1

#define LENGTH_OF_SEMAPHORES 2

//the union required by semctl (2)
union semun {
    int val;//value for SETVAL
    struct semid_ds *buf;//Buffer for IPC_STAT, IPC_SET
    unsigned short *array;//array for GETALL, SETALL
    struct seminfo *_buf;//Buffer for IPC_INFO
}

struct shared_variable_struct {
    int wcount; //number of waiting withdrawal cusomters on wlist
    int balance;
    LinkedList list;
}

int main (int argc, char *argv[]){
    
}
