
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

int get_semid(key_t semkey){
    int val = semget(semkey, LENGTH_OF_SEMAPHORES, 0777 | IPC_CREAT);
    if (val == -1){
        perror("semget failed");
        exit(EXIT_FAILURE);
    }
    return val;
}

int get_shmid(key_t shmkey){
    int val = shmget(shmkey, sizeof(struct shared_variable_struct), 0777 | IPC_CREAT);
    if (val == -1){
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    return val;
}

//semaphore wait and signal wrapper functions for System-V
void semaphore_operation (int semid, int semnum, int op){
    struct sembuf buffer;
    buffer.sem_num = semnum;
    buffer.sem_op = op;
    buffer.sem_flg = 0;

    //do the actual semaphore operation
    if (semop(semid, &buffer, 1) == -1){
        if (op==-1){
            perror("semaphore_wait failed");
        }
        if (op==1){
            perror("semaphore_signal failed");
        }
        exit(EXIT_FAILURE);
    }
}

void semaphore_wait (int semid, int semnum){
    semaphore_operation(semid, semnum, -1);
}

void semaphore_signal (int semid, int semnum){
    semaphore_operation(semid, semnum, 1);
}

//deposit/withdraw functions of the Savings Account problem

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
