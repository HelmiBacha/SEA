#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <time.h>

#define BUFFER_SIZE 256

pthread_mutex_t print_mutex;  // Mutex pour synchroniser l'affichage

// Fonction pour mesurer le temps d'exécution en millisecondes
double get_execution_time(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
}

// Fonction de surveillance de la mémoire
void* monitor_memory(void* arg) {
    while (1) {
    	printf("-------------------------------------------------------------------------\n");
        clock_t start = clock();

        struct sysinfo memInfo;
        sysinfo(&memInfo);
        long totalMemory = memInfo.totalram / (1024 * 1024);
        long freeMemory = memInfo.freeram / (1024 * 1024);

        clock_t end = clock();
        double execution_time = get_execution_time(start, end);

        pthread_mutex_lock(&print_mutex);
        printf("Mémoire totale: %ld MB, Mémoire libre: %ld MB, Temps: %.2f ms\n", totalMemory, freeMemory, execution_time);
        pthread_mutex_unlock(&print_mutex);

        sleep(2);  // Pause avant la prochaine vérification
        
    }
    return NULL;
}

// Fonction de surveillance du disque
void* monitor_disk(void* arg) {
    while (1) {
        clock_t start = clock();

        struct statvfs diskInfo;
        statvfs("/", &diskInfo);
        unsigned long long totalDisk = (diskInfo.f_blocks * diskInfo.f_frsize) / (1024 * 1024);
        unsigned long long freeDisk = (diskInfo.f_bfree * diskInfo.f_frsize) / (1024 * 1024);

        clock_t end = clock();
        double execution_time = get_execution_time(start, end);

        pthread_mutex_lock(&print_mutex);
        printf("Disque total: %llu MB, Disque libre: %llu MB, Temps: %.2f ms\n", totalDisk, freeDisk, execution_time);
        pthread_mutex_unlock(&print_mutex);

        sleep(2);
    }
    return NULL;
}

// Fonction de surveillance de l'utilisation réseau
void* monitor_network(void* arg) {
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

        pthread_mutex_lock(&print_mutex);
        printf("Données reçues: %lu bytes, Données envoyées: %lu bytes, Temps: %.2f ms\n", rx_bytes, tx_bytes, execution_time);
        pthread_mutex_unlock(&print_mutex);
	
        sleep(2);
        
    }
    return NULL;
}

int main() {
	
    // Initialisation du mutex
    pthread_mutex_init(&print_mutex, NULL);

    // Création des threads
    pthread_t memory_thread, disk_thread, network_thread;

    pthread_create(&memory_thread, NULL, monitor_memory, NULL);
    pthread_create(&disk_thread, NULL, monitor_disk, NULL);
    pthread_create(&network_thread, NULL, monitor_network, NULL);

    // Attente de la fin des threads (infinie ici)
    pthread_join(memory_thread, NULL);
    pthread_join(disk_thread, NULL);
    pthread_join(network_thread, NULL);

    // Destruction du mutex
    pthread_mutex_destroy(&print_mutex);
    

    return 0;
}
