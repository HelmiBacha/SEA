#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <time.h>
#include <semaphore.h>
#include <string.h>

#define BUFFER_SIZE 256
#define QUEUE_SIZE 10  // Taille de la file

typedef struct {
    char messages[QUEUE_SIZE][BUFFER_SIZE];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    sem_t items;
    sem_t space;
} Queue;

// Initialisation de la file
void init_queue(Queue* queue) {
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->items, 0, 0);
    sem_init(&queue->space, 0, QUEUE_SIZE);
}

// Enfilement d'un message
void enqueue(Queue* queue, const char* message) {
    sem_wait(&queue->space);               // Attendre l'espace disponible
    pthread_mutex_lock(&queue->mutex);      // Verrouiller l'accès

    strncpy(queue->messages[queue->rear], message, BUFFER_SIZE - 1);
    queue->messages[queue->rear][BUFFER_SIZE - 1] = '\0';
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    queue->count++;

    pthread_mutex_unlock(&queue->mutex);    // Déverrouiller l'accès
    sem_post(&queue->items);                // Signaler un nouvel élément
}

// Défilement d'un message
void dequeue(Queue* queue, char* buffer) {
    sem_wait(&queue->items);                // Attendre un élément disponible
    pthread_mutex_lock(&queue->mutex);      // Verrouiller l'accès

    strncpy(buffer, queue->messages[queue->front], BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    queue->front = (queue->front + 1) % QUEUE_SIZE;
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);    // Déverrouiller l'accès
    sem_post(&queue->space);                // Signaler un espace libre
}

// Fonction pour mesurer le temps d'exécution en millisecondes
double get_execution_time(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
}

// Producteur de surveillance de la mémoire
void* monitor_memory(void* arg) {
    Queue* queue = (Queue*)arg;
    while (1) {
    printf("------------------------------------------\n");
        clock_t start = clock();
        
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        long totalMemory = memInfo.totalram / (1024 * 1024);
        long freeMemory = memInfo.freeram / (1024 * 1024);

        clock_t end = clock();
        double execution_time = get_execution_time(start, end);

        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Mémoire totale: %ld MB, Mémoire libre: %ld MB, Temps: %.2f ms", 
                 totalMemory, freeMemory, execution_time);
        
        enqueue(queue, message);
        sleep(2);  // Pause avant la prochaine vérification
    }
    return NULL;
}

// Producteur de surveillance du disque
void* monitor_disk(void* arg) {
    Queue* queue = (Queue*)arg;
    while (1) {
        clock_t start = clock();

        struct statvfs diskInfo;
        statvfs("/", &diskInfo);
        unsigned long long totalDisk = (diskInfo.f_blocks * diskInfo.f_frsize) / (1024 * 1024);
        unsigned long long freeDisk = (diskInfo.f_bfree * diskInfo.f_frsize) / (1024 * 1024);

        clock_t end = clock();
        double execution_time = get_execution_time(start, end);

        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Disque total: %llu MB, Disque libre: %llu MB, Temps: %.2f ms", 
                 totalDisk, freeDisk, execution_time);

        enqueue(queue, message);
        sleep(2);
    }
    return NULL;
}

// Producteur de surveillance du réseau
void* monitor_network(void* arg) {
    Queue* queue = (Queue*)arg;
    while (1) {
        clock_t start = clock();

        FILE *file = fopen("/sys/class/net/ens33/statistics/rx_bytes", "r");
        if (file == NULL) {
            perror("Erreur lors de la lecture du réseau (rx_bytes)");
            return NULL;
        }
        unsigned long rx_bytes;
        fscanf(file, "%lu", &rx_bytes);
        fclose(file);

        file = fopen("/sys/class/net/ens33/statistics/tx_bytes", "r");
        if (file == NULL) {
            perror("Erreur lors de la lecture du réseau (tx_bytes)");
            return NULL;
        }
        unsigned long tx_bytes;
        fscanf(file, "%lu", &tx_bytes);
        fclose(file);

        clock_t end = clock();
        double execution_time = get_execution_time(start, end);

        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Données reçues: %lu bytes, Données envoyées: %lu bytes, Temps: %.2f ms", 
                 rx_bytes, tx_bytes, execution_time);

        enqueue(queue, message);
        sleep(2);
    }
    return NULL;
}

// Consommateur : affiche les messages
void* consumer(void* arg) {
    Queue* queue = (Queue*)arg;
    char message[BUFFER_SIZE];
    while (1) {
        dequeue(queue, message);
        printf("%s\n", message);
    }
    return NULL;
}

int main() {
    Queue queue;
    init_queue(&queue);

    // Création des threads producteurs et consommateur
    pthread_t memory_thread, disk_thread, network_thread, consumer_thread;

    pthread_create(&memory_thread, NULL, monitor_memory, (void*)&queue);
    pthread_create(&disk_thread, NULL, monitor_disk, (void*)&queue);
    pthread_create(&network_thread, NULL, monitor_network, (void*)&queue);
    pthread_create(&consumer_thread, NULL, consumer, (void*)&queue);

    // Attente des threads (infinie ici)
    pthread_join(memory_thread, NULL);
    pthread_join(disk_thread, NULL);
    pthread_join(network_thread, NULL);
    pthread_join(consumer_thread, NULL);

    return 0;
}
