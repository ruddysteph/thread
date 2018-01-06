/*

La valeur maximale de NUM_EATING permettant d'éviter l'interblocage est le nombre de mangeur -1.

*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>    // usleep
#include <semaphore.h> // pour les semaphores
#include <fcntl.h>     // pour les flags O_CREAT, O_EXCL, ..

// compile avec
// gcc -Wall phylo.c -lpthread

#define NUM_DINERS 3
#define EAT_TIMES 20
#define NUM_EATING 1

/* Macros pour faire référence facilement aux fourchettes droite et gauche */
#define LEFT(philNum) (philNum)
#define RIGHT(philNum) ((philNum + 1) % NUM_DINERS)

typedef struct p {
    int num;
    sem_t * left_fork;
    sem_t * right_fork;
    sem_t * numEating;
} Philo;

void forkWait(sem_t * fork) {
    sem_wait(fork);
}

void forkPost(sem_t * fork) {
    sem_post(fork);
}

void* philo_thread(void* arg) {
    Philo p = *(Philo*) arg;

    int i;
    for(i = 0; i < EAT_TIMES; i++) {
        forkWait(p.numEating);
        printf("phylo %d attend la fourchette %d\n", p.num, RIGHT(p.num));
        forkWait(p.right_fork);
        printf("phylo %d attend la fourchette %d\n", p.num, LEFT(p.num));
        forkWait(p.left_fork);
        printf("phylo %d mange\n", p.num);
        usleep(10000);
        forkPost(p.left_fork);
        printf("phylo %d relâche la fourchette %d\n", p.num, LEFT(p.num));
        forkPost(p.right_fork);
        printf("phylo %d relâche la fourchette %d\n", p.num, RIGHT(p.num));
        forkPost(p.numEating);
    }
    return NULL;
}

int main() {
    int i;
    // semaphores représentant les fourchettes
    sem_t * forks[NUM_DINERS];
    char fork_names[NUM_DINERS][10];
    int error = 0;
    for (i = 0; i < NUM_DINERS; i++) {
        snprintf(fork_names[i], 10, "fork%d", i);
        forks[i] = sem_open(fork_names[i], O_CREAT | O_EXCL, 755, 1);
        if (forks[i] == SEM_FAILED) error++; // une erreur s'est produite
    }
    // semaphore pour le nombre de "mangeurs" simultanés
    sem_t * numEating = sem_open("numEating", O_CREAT | O_EXCL, 755,  NUM_EATING);
    if (numEating == SEM_FAILED) error++;  // une erreur s'est produite

    // en cas d'erreur dans la création d'un sémaphore, on les supprime tous
    if (error) {
        for (i = 0; i < NUM_DINERS; i++)
            sem_unlink(fork_names[i]);
        sem_unlink("numEating");
        printf("semaphores existants éliminés !\n");
        exit(EXIT_FAILURE); // et on sort
    }
    // lancer les threads philosophes
    pthread_t philo_threads[NUM_DINERS];
    Philo philo_infos[NUM_DINERS];
    for (i = 0; i < NUM_DINERS; i++) {
        philo_infos[i].num = i;
        philo_infos[i].left_fork = forks[LEFT(i)];
        philo_infos[i].right_fork = forks[RIGHT(i)];
        philo_infos[i].numEating = numEating;
        pthread_create(&philo_threads[i], NULL, philo_thread, &philo_infos[i]);
    }

    // attendre les threads
    for (i = 0; i < NUM_DINERS; i++) {
        pthread_join(philo_threads[i], NULL);
    }
    // détruire les semaphores
    sem_close(numEating);
    sem_unlink("numEating");
    for (i = 0; i < NUM_DINERS; i++) {
        sem_close(forks[i]);
        sem_unlink(fork_names[i]);
    }
    return EXIT_SUCCESS;
}