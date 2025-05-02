#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_BUSES_X 5
#define NUM_BUSES_Y 4
#define NUM_ROUND_TRIPS 10

sem_t mutex;  // Single mutex for both counters
sem_t S1, S2; // S1 = for X->Y wait; S2 = for Y->X wait
int nb_AB = 0, nb_BA = 0;

void random_sleep() {
    usleep((rand() % 500 + 1000) * 1000); // Sleep between 1s and 1.5s
}

void* bus_X(void* arg) {
    int id = *((int*)arg);
    for (int i = 1; i <= NUM_ROUND_TRIPS; i++) {
        // ---- Trajet Aller (X->Y) ----
        sem_wait(&mutex);
        if (nb_BA > 0) {
            sem_post(&mutex);
            sem_wait(&S1);  // wait for Y->X to finish
        } else {
            nb_AB++;
            sem_post(&mutex);
        }

        printf("Bus %d [X]: X->Y (Trip %d)\n", id, i);
        random_sleep();

        // ---- Sortie du tunnel (X->Y) ----
        sem_wait(&mutex);
        nb_AB--;
        if (nb_AB == 0)
            sem_post(&S2);  // Wake up waiting Y->X
        sem_post(&mutex);

        // ---- Trajet Retour (Y->X) ----
        sem_wait(&mutex);
        if (nb_AB > 0) {
            sem_post(&mutex);
            sem_wait(&S2);  // wait for X->Y to finish
        } else {
            nb_BA++;
            sem_post(&mutex);
        }

        printf("Bus %d [X]: Y->X (Trip %d)\n", id, i);
        random_sleep();

        // ---- Sortie du tunnel (Y->X) ----
        sem_wait(&mutex);
        nb_BA--;
        if (nb_BA == 0)
            sem_post(&S1);  // Wake up waiting X->Y
        sem_post(&mutex);
    }

    free(arg);
    return NULL;
}

void* bus_Y(void* arg) {
    int id = *((int*)arg);
    for (int i = 1; i <= NUM_ROUND_TRIPS; i++) {
        // ---- Trajet Aller (Y->X) ----
        sem_wait(&mutex);
        if (nb_AB > 0) {
            sem_post(&mutex);
            sem_wait(&S2);  // wait for X->Y to finish
        } else {
            nb_BA++;
            sem_post(&mutex);
        }

        printf("Bus %d [Y]: Y->X (Trip %d)\n", id, i);
        random_sleep();

        // ---- Sortie du tunnel (Y->X) ----
        sem_wait(&mutex);
        nb_BA--;
        if (nb_BA == 0)
            sem_post(&S1);  // Wake up waiting X->Y
        sem_post(&mutex);

        // ---- Trajet Retour (X->Y) ----
        sem_wait(&mutex);
        if (nb_BA > 0) {
            sem_post(&mutex);
            sem_wait(&S1);  // wait for Y->X to finish
        } else {
            nb_AB++;
            sem_post(&mutex);
        }

        printf("Bus %d [Y]: X->Y (Trip %d)\n", id, i);
        random_sleep();

        // ---- Sortie du tunnel (X->Y) ----
        sem_wait(&mutex);
        nb_AB--;
        if (nb_AB == 0)
            sem_post(&S2);  // Wake up waiting Y->X
        sem_post(&mutex);
    }

    free(arg);
    return NULL;
}

int main() {
    srand(time(NULL));

    sem_init(&mutex, 0, 1);  // shared mutex
    sem_init(&S1, 0, 0);     // for X->Y
    sem_init(&S2, 0, 0);     // for Y->X

    pthread_t threads[NUM_BUSES_X + NUM_BUSES_Y];

    // Create X buses
    for (int i = 0; i < NUM_BUSES_X; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&threads[i], NULL, bus_X, id);
    }

    // Create Y buses
    for (int i = 0; i < NUM_BUSES_Y; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&threads[NUM_BUSES_X + i], NULL, bus_Y, id);
    }

    // Join all threads
    for (int i = 0; i < NUM_BUSES_X + NUM_BUSES_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&S1);
    sem_destroy(&S2);

    return 0;
}
