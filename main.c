#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// total count of santa's crew
#define REN_TOTAL 9
#define ELF_TOTAL 10

// minimal numbers that will make santa wake up
#define REN_WAIT_MIN REN_TOTAL
#define ELF_WAIT_MIN 3

// randomised sleep generation boundaries
#define WAIT_TIME_MIN 1
#define WAIT_TIME_MAX 5

// aliases to make using mutexes easier
#define lock(sem) pthread_mutex_lock(&sem)
#define unlock(sem) pthread_mutex_unlock(&sem)

// aliases to make using conditional variables easier
#define wait pthread_cond_wait
#define signal pthread_cond_signal
#define broadcast pthread_cond_broadcast

// more readable type names for pthread constructs
typedef pthread_cond_t Cond;
typedef pthread_mutex_t Mutex;
typedef pthread_t Thread;

// WAITING ROOMS
Mutex access_reindeer = PTHREAD_MUTEX_INITIALIZER;
int waiting_reindeer = 0;

Mutex access_elves = PTHREAD_MUTEX_INITIALIZER;
int waiting_elves = 0;

// pairs of mutexes and conditional variables
// that control sleep/waiting of the participating threads
Mutex wait_access_reindeer = PTHREAD_MUTEX_INITIALIZER;
Cond wait_control_reindeer = PTHREAD_COND_INITIALIZER;

Mutex wait_access_elves = PTHREAD_MUTEX_INITIALIZER;
Cond wait_control_elves = PTHREAD_COND_INITIALIZER;

Mutex sleep_access_santa = PTHREAD_MUTEX_INITIALIZER;
Cond sleep_control_santa = PTHREAD_COND_INITIALIZER;

/**
 * Make a thread sleep for a randomised time.
 * Boundaries of the randomisation can be set in defines.
 * @param id identifier of the caller, used to log output/debugging info.
 */
void randomised_sleep(int id) {
    int time = rand() % (WAIT_TIME_MAX - WAIT_TIME_MIN + 1) + WAIT_TIME_MIN;
    printf("[%3d] sleeps for %u sec\n", id, time);
    sleep(time);
}

/**
 * Santa sleeps until he's called by his reindeer or elves (inclusive or).
 * When he wakes up he attends to his stuff prioritising the reindeer.
 */
_Noreturn void *santa(void *arg) {
    while (1) {
        printf("! mikołaj zasypia\n");
        lock(sleep_access_santa);
        // wait until reindeer or elves wake santa up
        wait(&sleep_control_santa, &sleep_access_santa);
        unlock(sleep_access_santa);
        printf("! mikołaj obudzony\n");

        // attend to the reindeer first
        // even if they are not the original wake-up-ers
        if (waiting_reindeer == REN_WAIT_MIN) {
            printf("< Obsługa reniferów\n");
            waiting_reindeer = 0;
            // let the attended reindeer scatter
            broadcast(&wait_control_reindeer);
            printf("> Zwolnienie reniferów\n");
        }

        // attend to the elves
        if (waiting_elves >= ELF_WAIT_MIN) {
            // prevent more elves from joining the waiting crew
            // they will have to wait fot a new group to assemble before being served by the santa
            lock(access_elves);
            printf("< Obsługa elfów\n");
            waiting_elves = 0;
            // let the currently attended elves scatter
            broadcast(&wait_control_elves);
            printf("> Zwolnienie elfów\n");
            unlock(access_elves);
        }
    }
}

/**
 * All the reindeer must gather before waking up santa.
 * After some time of being occupied by reindeer matters they enter their waiting room
 * and wait until the last one wakes up santa to attend to them.
 * @param id identifier of the reindeer
 */
_Noreturn void *reindeer(const int *id) {
    while (1) {
        randomised_sleep(*id);
        lock(access_reindeer);
        waiting_reindeer++;
        printf("\t[%3d] waits for santa: %u total\n", *id, waiting_reindeer);

        // if this reindeer is the last one required to wake up santa it will wake him up,
        // but wait with all the others to be released by santa nonetheless
        if (waiting_reindeer == REN_WAIT_MIN) {
            signal(&sleep_control_santa);
            unlock(sleep_access_santa);
            printf("!\t[%3d] waking up santa\n", *id);
        }
        unlock(access_reindeer);

        lock(wait_access_reindeer);

        printf("\t[%3d] goes to sleep\n", *id);
        wait(&wait_control_reindeer, &wait_access_reindeer);
        unlock(wait_access_reindeer);
    }
}

/**
 * Elves work until they stumble on a problem requiring santa's attention.
 * if they do, they gather in their own waiting room till at least ELF_WAIT_MIN of them are there.
 * If an elf enters the waiting room and notices that there is enough of them (including the elf)
 * to wake up santa, he will do it.
 * Thus, it is possible that there will be more elves waiting than the minimal number.
 * @param id identifier of the elf.
 */
_Noreturn void *elf(const int *id) {
    while (1) {
        randomised_sleep(*id);
        lock(access_elves);
        waiting_elves++;
        printf("Waiting elves: %u\n", waiting_elves);

        if (waiting_elves >= ELF_WAIT_MIN) {
            signal(&sleep_control_santa);
            unlock(sleep_access_santa);
            printf("!\t%d\twaking santa\n", *id);
        }
        unlock(access_elves);

        lock(wait_access_elves);
        printf("\t%d\tgo to sleep\n", *id);
        wait(&wait_control_elves, &wait_access_elves);
        unlock(wait_access_elves);
    }
}

int main(void) {
    srand(getpid());

    // initialising required threads
    Thread santa_thread;
    Thread elf_thread[ELF_TOTAL];
    Thread reindeer_thread[REN_TOTAL];

    // creating identifiers for the reindeer_thread
    int ren_ids[REN_TOTAL];
    for (int r = 0; r < REN_TOTAL; r++) ren_ids[r] = r + 100;

    // creating identifiers for the elf_thread
    int elf_ids[ELF_TOTAL];
    for (int e = 0; e < ELF_TOTAL; e++) elf_ids[e] = e;

    // spawning all threads
    pthread_create(&santa_thread, NULL, santa, NULL);
    for (int r = 0; r < REN_TOTAL; r++) pthread_create(&reindeer_thread[r], NULL, reindeer, &ren_ids[r]);
    for (int e = 0; e < ELF_TOTAL; e++) pthread_create(&elf_thread[e], NULL, elf, &elf_ids[e]);

    // waiting for the threads to finish
    pthread_join(santa_thread, NULL);
    for (int r = 0; r < REN_TOTAL; r++) pthread_join(reindeer_thread[r], NULL);
    for (int e = 0; e < ELF_TOTAL; e++) pthread_join(elf_thread[e], NULL);

    return 0;
}