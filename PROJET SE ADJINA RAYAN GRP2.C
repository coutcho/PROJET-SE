#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define X_BUSES 5
#define Y_BUSES 4
#define TOTAL_BUSES (X_BUSES + Y_BUSES)
#define TRIPS 10

typedef enum {
    X_TO_Y = 0,
    Y_TO_X = 1
} Direction;

pthread_mutex_t mutex;
sem_t tunnel_access[2];

int buses_in_tunnel = 0;
int waiting_buses[2] = {0, 0};
Direction current_direction;
int is_tunnel_empty = 1;

typedef struct {
    int id;
    char home_city;
    Direction initial_direction;
} BusInfo;

const char* direction_to_string(Direction dir) {
    return (dir == X_TO_Y) ? "X -> Y" : "Y -> X";
}

void travel(void) {
    int delay = (rand() % 500) + 1000;
    usleep(delay * 1000);
}

void enter_tunnel(int bus_id, char home_city, Direction direction, int trip_number) {
    pthread_mutex_lock(&mutex);
    
    char from_city = (direction == X_TO_Y) ? 'X' : 'Y';
    char to_city = (direction == X_TO_Y) ? 'Y' : 'X';
    
    if (is_tunnel_empty) {
        current_direction = direction;
        is_tunnel_empty = 0;
    }
    
    if (!is_tunnel_empty && current_direction != direction) {
        waiting_buses[direction]++;
        pthread_mutex_unlock(&mutex);
        sem_wait(&tunnel_access[direction]);
        pthread_mutex_lock(&mutex);
    }
    
    buses_in_tunnel++;
    printf("Bus %d de la ville %c: %c -> %c (Trajet %d)\n", 
           bus_id, home_city, from_city, to_city, trip_number);
    
    pthread_mutex_unlock(&mutex);
}

void exit_tunnel(int bus_id, char home_city, Direction direction) {
    pthread_mutex_lock(&mutex);
    
    buses_in_tunnel--;
    
    if (buses_in_tunnel == 0) {
        is_tunnel_empty = 1;
        
        Direction opposite = (direction + 1) % 2;
        
        if (waiting_buses[opposite] > 0) {
            current_direction = opposite;
            waiting_buses[opposite]--;
            sem_post(&tunnel_access[opposite]);
        } 
        else if (waiting_buses[direction] > 0) {
            waiting_buses[direction]--;
            sem_post(&tunnel_access[direction]);
        }
    }
    
    pthread_mutex_unlock(&mutex);
}

void* bus_function(void* arg) {
    BusInfo* info = (BusInfo*)arg;
    int bus_id = info->id;
    char home_city = info->home_city;
    Direction start_direction = info->initial_direction;
    
    for (int trip = 1; trip <= TRIPS; trip++) {
        enter_tunnel(bus_id, home_city, start_direction, trip);
        travel();
        exit_tunnel(bus_id, home_city, start_direction);
        
        usleep(500000);
        
        Direction return_direction = (start_direction + 1) % 2;
        enter_tunnel(bus_id, home_city, return_direction, trip);
        travel();
        exit_tunnel(bus_id, home_city, return_direction);
        
        usleep(500000);
    }
    
    printf("Bus %d de la ville %c a terminé tous ses %d allers-retours\n", 
           bus_id, home_city, TRIPS);
    
    free(info);
    return NULL;
}

int main() {
    srand(time(NULL));
    
    pthread_mutex_init(&mutex, NULL);
    sem_init(&tunnel_access[X_TO_Y], 0, 0);
    sem_init(&tunnel_access[Y_TO_X], 0, 0);
    
    pthread_t threads[TOTAL_BUSES];
    int thread_count = 0;
    
    printf("Démarrage de la simulation avec %d bus de la ville X et %d bus de la ville Y\n", X_BUSES, Y_BUSES);
    printf("Chaque bus effectuera %d allers-retours\n\n", TRIPS);
    
    for (int i = 1; i <= X_BUSES; i++) {
        BusInfo* info = (BusInfo*)malloc(sizeof(BusInfo));
        info->id = i;
        info->home_city = 'X';
        info->initial_direction = X_TO_Y;
        
        pthread_create(&threads[thread_count++], NULL, bus_function, (void*)info);
        usleep(50000);
    }
    
    for (int i = 1; i <= Y_BUSES; i++) {
        BusInfo* info = (BusInfo*)malloc(sizeof(BusInfo));
        info->id = i;
        info->home_city = 'Y';
        info->initial_direction = Y_TO_X;
        
        pthread_create(&threads[thread_count++], NULL, bus_function, (void*)info);
        usleep(50000);
    }
    
    for (int i = 0; i < TOTAL_BUSES; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex);
    sem_destroy(&tunnel_access[X_TO_Y]);
    sem_destroy(&tunnel_access[Y_TO_X]);
    
    printf("\nSimulation terminée. Tous les bus ont effectué leurs trajets.\n");
    return 0;
}