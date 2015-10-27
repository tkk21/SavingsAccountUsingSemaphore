
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
    printf("Program started with the pid %d\n", getpid());

    union semun semaphore_values;

    //setting up the initial semaphore values
    unsigned short semaphore_init_values[LENGTH_OF_SEMAPHORES];
    semaphore_init_values[SEMAPHORE_MUTEX] = 1;
    semaphore_init_values[SEMAPHORE_wlist] = 0;
    semaphore_values.array = semaphore_init_values;

    //syscall to make semaphore
    int semid = get_semid((key_t)SEMAPHORE_KEY);
    if (semctl (semid, SEMAPHORE_MUTEX, SETALL, semaphore_values) == -1){
        perror("semctl failed");
        exit(EXIT_FAILURE);
    }

    //Shared memory 
    int shmid = get_shmid((key_t)SEMAPHORE_KEY);
    struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);

    //initial values of the shared memory variables
    shared_variables->wcount = 0;
    shared_variables->balance = 500;
    shared_variables->list = NULL;






    //everything is done
    printf("Done\n");

    //cleanup
    if(shmdt(shared_variables) == -1){
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    if (shmctl(shmid, IPC_RMID, NULL) < 0){
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, SEMAPHORE_MUTEX, IPC_RMID, semaphore_values) == -1){
        perror("semctl failed");
        exit(EXIT_FAILURE);
    }
    

}
