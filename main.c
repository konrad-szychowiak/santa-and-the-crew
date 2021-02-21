#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define COUNT_REN 9
#define COUNT_ELF 10

#define WAIT_REN_MIN 9
#define WAIT_ELF_MIN 3

#define WAIT_TIME_MIN 1
#define WAIT_TIME_MAX 5

#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock

#define wait pthread_cond_wait
#define signal pthread_cond_signal
#define broadcast pthread_cond_broadcast

typedef pthread_cond_t Cond;
typedef pthread_mutex_t Mutex;

// waiting rooms
int waiting_rens = 0;
int waiting_elves = 0;

// PILLOWS control sleep fo the participants
Mutex pillow_controller_rens = PTHREAD_MUTEX_INITIALIZER;
Cond pillow_rens = PTHREAD_COND_INITIALIZER;

Mutex pillow_controller_elves = PTHREAD_MUTEX_INITIALIZER;
Cond pillow_elves = PTHREAD_COND_INITIALIZER;

Mutex pillow_controller_santa = PTHREAD_MUTEX_INITIALIZER;
Cond pillow_santa = PTHREAD_COND_INITIALIZER;

// switches
Mutex access_rens = PTHREAD_MUTEX_INITIALIZER;
Mutex master_access_rens = PTHREAD_MUTEX_INITIALIZER;
Mutex access_elves = PTHREAD_MUTEX_INITIALIZER;
Mutex master_access_elves = PTHREAD_MUTEX_INITIALIZER;


void randomised_sleep(int id) {
    int time = rand() % (WAIT_TIME_MAX - WAIT_TIME_MIN + 1) + WAIT_TIME_MIN;
    printf("\t[%3d] sleeps for %u sec\n", id, time);
    sleep(time);
}


_Noreturn void *santa(void *arg) {
    while (1) {
        printf("!\tmikołaj zasypia\n");
        lock(&pillow_controller_santa);
        wait(&pillow_santa, &pillow_controller_santa);
        unlock(&pillow_controller_santa);
        printf("!\tmikołaj obudzony\n");

        if (waiting_rens == WAIT_REN_MIN) {
            printf("< Obsługa reniferów\n");
            sleep(5);
            waiting_rens = 0;
            broadcast(&pillow_rens);
            // unlock(&pillow_controller_rens);
            printf("> Zwolnienie reniferów\n");
        }

        if (waiting_elves >= WAIT_ELF_MIN) {
            lock(&access_elves);
            printf("< Obsługa elfów\n");
            waiting_elves = 0;
            broadcast(&pillow_elves);
            // unlock(&pillow_controller_elves);
            printf("> Zwolnienie elfów\n");
            unlock(&access_elves);
        }
    }
}


_Noreturn void *reindeer(int *id) {
    while (1) {
        randomised_sleep(*id);
        lock(&access_rens);
        waiting_rens++;
        printf("\t[%3d] waits for santa: %u total\n", *id, waiting_rens);
        if (waiting_rens == WAIT_REN_MIN) {
            signal(&pillow_santa);
            unlock(&pillow_controller_santa);
            printf("!\t[%3d] waking santa\n", *id);
        }
        unlock(&access_rens);

        lock(&pillow_controller_rens);
        printf("\t[%3d] goes to sleep\n", *id);
        wait(&pillow_rens, &pillow_controller_rens);
        unlock(&pillow_controller_rens);
    }
}


_Noreturn void *elf(int *id) {
    while (1) {
        randomised_sleep(*id);
        printf("\t[%3d] wakes up\n", *id);
        lock(&access_elves);
        waiting_elves++;
        printf("Waiting elves: %u\n", waiting_elves);

        if (waiting_elves >= WAIT_ELF_MIN) {
            signal(&pillow_santa);
            unlock(&pillow_controller_santa);
            printf("!\t%d\twaking santa\n", *id);
        }
        unlock(&access_elves);

        lock(&pillow_controller_elves);
        printf("\t%d\tgo to sleep\n", *id);
        wait(&pillow_elves, &pillow_controller_elves);
        unlock(&pillow_controller_elves);
    }
}

int main(void) {
    printf("hello, world\n");
    pthread_t snt;
    pthread_t rens[COUNT_REN];
    pthread_t elves[COUNT_ELF];

    int ren_ids[COUNT_REN];
    for (int r = 0; r < COUNT_REN; r++) ren_ids[r] = r + 100;

    int elf_ids[COUNT_ELF];
    for (int e = 0; e < COUNT_ELF; e++) elf_ids[e] = e;

    printf("initialising...\n");
    pthread_create(&snt, NULL, santa, NULL);
    for (int r = 0; r < COUNT_REN; r++) pthread_create(&rens[r], NULL, reindeer, &ren_ids[r]);
    for (int e = 0; e < COUNT_ELF; e++) pthread_create(&elves[e], NULL, elf, &elf_ids[e]);

    pthread_join(snt, NULL);
    for (int r = 0; r < COUNT_REN; r++) pthread_join(rens[r], NULL);
    for (int e = 0; e < COUNT_ELF; e++) pthread_join(elves[e], NULL);

    return 0;
}