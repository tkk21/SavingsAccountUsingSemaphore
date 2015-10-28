#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "LinkedList.h"

#define SEMAPHORE_KEY 0xFA2B

//Index of the semaphores in the semaphore array
#define SEMAPHORE_MUTEX 0
#define SEMAPHORE_WLIST 1

#define LENGTH_OF_SEMAPHORES 2

//the union required by semctl (2)
union semun {
    int val;//value for SETVAL
    struct semid_ds *buf;//Buffer for IPC_STAT, IPC_SET
    unsigned short *array;//array for GETALL, SETALL
    struct seminfo *_buf;//Buffer for IPC_INFO
};

struct shared_variable_struct {
    int wcount; //number of waiting withdrawal customers on wlist
    int balance;
    LinkedList *list;
};

typedef enum {DEPOSIT, WITHDRAW} transaction_type;

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

void print_statement (struct shared_variable_struct * shared_variables){
    printf("Current Balance is: %d\n", shared_variables->balance);
    printf("[PID: %d]: Number of Waiting Withdrawals: %d\n", getpid(), shared_variables->wcount);
}

//deposit/withdraw functions of the Savings Account problem

void deposit(int deposit){
    printf("[PID: %d]: Depositing %d\n", getpid(), deposit);
    
    int semid = get_semid((key_t)SEMAPHORE_KEY);
    int shmid = get_shmid((key_t)SEMAPHORE_KEY);
    struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);

    //Implementing Deposit using semaphore
    printf("[PID: %d, Deposit]: Waiting on Mutex\n", getpid());
    semaphore_wait(semid, SEMAPHORE_MUTEX);
    printf("[PID: %d, Deposit]: Passed Mutex\n", getpid());
    
    printf("[PID: %d, Deposit]: (Before Deposit) ", getpid());
    print_statement(shared_variables);
    shared_variables->balance = shared_variables->balance + deposit;
    printf("[PID: %d, Deposit]: (After Deposit) ", getpid());
    print_statement(shared_variables);
    
    if (shared_variables->wcount == 0){ // no withdrawal requests at this time
        semaphore_signal(semid, SEMAPHORE_MUTEX);
    }
    else if (FirstRequestAmount(shared_variables->list) > shared_variables->balance){ // Not enough balance for 0st waiting withdrawal
        semaphore_signal(semid, SEMAPHORE_MUTEX);
    }
    else {
        semaphore_signal(semid, SEMAPHORE_WLIST);//enough to withdraw, signal the waiting withdrawal
    }
    //done
    exit(EXIT_SUCCESS);
}

void withdraw(int withdraw){
    printf("[PID: %d]: Withdrawing %d\n", getpid(), withdraw);
    int semid = get_semid((key_t)SEMAPHORE_KEY);
    int shmid = get_shmid((key_t)SEMAPHORE_KEY);
    struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);

    //Implementing Withdraw using semaphore
    printf("[PID: %d, Withdraw]: Waiting on Mutex\n", getpid());
    semaphore_wait(semid, SEMAPHORE_MUTEX);
    printf("[PID: %d, Withdraw]: Passed Mutex\n", getpid());
    
    if (shared_variables->wcount == 0 && shared_variables->balance>=withdraw){
        printf("[PID: %d, Withdraw]: (Before Withdrawal) ", getpid());
        print_statement(shared_variables);
        shared_variables->balance = shared_variables->balance - withdraw;
        printf("[PID: %d, Withdraw]: (After Withdrawal) ", getpid());
        print_statement(shared_variables);
 
        semaphore_signal(semid, SEMAPHORE_MUTEX);
    }
    else {
        shared_variables->wcount = shared_variables->wcount + 1;
        AddEndOfList(shared_variables->list, withdraw);
        semaphore_signal(semid, SEMAPHORE_MUTEX);
        printf("[PID: %d, Withdraw]: Waiting on wlist\n", getpid());
        semaphore_wait(semid, SEMAPHORE_WLIST); //start waiting for a deposit
        printf("[PID: %d, Withdraw]: Passed wlist\n", getpid());

        sleep(2); //sleep 2s to slow down

        printf("[PID: %d, Withdraw]: (Before Withdrawal) ", getpid());
        print_statement(shared_variables);
        shared_variables->balance = shared_variables->balance - FirstRequestAmount(shared_variables->list);
        
        DeleteFirstRequest(shared_variables->list);
        shared_variables->wcount = shared_variables->wcount - 1;
        printf("[PID: %d, Withdraw]: (After Withdrawal) ", getpid());
        print_statement(shared_variables);
        

        if (shared_variables->wcount > 0 && FirstRequestAmount(shared_variables->list) < shared_variables->balance){
            semaphore_signal(semid, SEMAPHORE_WLIST);
        }
        else{
            semaphore_signal(semid, SEMAPHORE_MUTEX);//this signal() is paired with deposit's wait(mutex)
        }
    }
    exit(EXIT_SUCCESS);
}

void customer_fork (transaction_type type, int amount){
    pid_t child_pid;
    child_pid = fork();
    if (child_pid == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0){
        switch(type){
        
            case DEPOSIT:
                deposit(amount);
                break;
            case WITHDRAW:
                withdraw(amount);
                break;
            default:
                printf("!Invalid Transaction Type!\n");
                exit(EXIT_FAILURE);
        }
    }
    else {
        return; //parent
    }
}

//this function starts up the customer processes in a preset manner
//as allowed in the instructions
//the preset sequence is
//     w50, w30, w20, d5, d35, d60, d100, w250, d275, d55, w700, w80, w100, d300, w100
//expected sequence of balance is
//500, 450, 420, 400, 405, 440, 500, 600, 350, 625, 680, -20, -100, -200, 100, 0
int test (){
    time_t t;
    srand( (unsigned) time(&t));
    transaction_type transaction_list[15] = {WITHDRAW, WITHDRAW, WITHDRAW, DEPOSIT, DEPOSIT, DEPOSIT, DEPOSIT, WITHDRAW, DEPOSIT, DEPOSIT, WITHDRAW, WITHDRAW, WITHDRAW, DEPOSIT, WITHDRAW};
    int amount_list[15] = {50, 30, 20, 5, 35, 60, 100, 250, 275, 55, 700, 80, 100, 300, 100};
    int i; 
    for (i = 0; i<15; i++){
        customer_fork(transaction_list[i], amount_list[i]);
        sleep(rand()%2+ 1);//sleep between 1-3 s
    }

    //customer_fork(WITHDRAW, 200);

    return 15;
}

int test1 (){
    
    transaction_type transaction_list[5] = {DEPOSIT, DEPOSIT, DEPOSIT, DEPOSIT, DEPOSIT};
    int amount_list[5] = {100, 100, 100, 100, 100};
    int i;
    for (i=0;i<5;i++){
        customer_fork(transaction_list[i], amount_list[i]);
        sleep(1);
    }
    return 5;
}

int main (int argc, char *argv[]){
    printf("Program started with the pid %d\n", getpid());

    union semun semaphore_values;

    //setting up the initial semaphore values
    unsigned short semaphore_init_values[LENGTH_OF_SEMAPHORES];
    semaphore_init_values[SEMAPHORE_MUTEX] = 1;
    semaphore_init_values[SEMAPHORE_WLIST] = 0;
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
    shared_variables->list = malloc(sizeof(LinkedList));
    shared_variables->list->head = NULL;
    
    int processes_fired;
    processes_fired = test();
    
    int i;
    for (i = 0; i<processes_fired; i++){
        wait(NULL);
    }
    
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
