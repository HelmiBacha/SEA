#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <time.h>

// Fonction pour mesurer et afficher le temps d'exécution d'une tâche
double get_execution_time(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC * 1000; // Retourne le temps en millisecondes
}

// Fonction pour surveiller la mémoire
void monitor_memory() {
    clock_t start = clock(); // Début du chronométrage
    struct sysinfo memInfo;
    sysinfo(&memInfo);

    long totalMemory = memInfo.totalram / (1024 * 1024);
    long freeMemory = memInfo.freeram / (1024 * 1024);

    printf("Mémoire totale: %ld MB, Mémoire libre: %ld MB\n", totalMemory, freeMemory);

    clock_t end = clock(); // Fin du chronométrage
    printf("Temps d'exécution (mémoire): %.2f ms\n", get_execution_time(start, end));
}

// Fonction pour surveiller le disque
void monitor_disk() {
    clock_t start = clock(); // Début du chronométrage
    struct statvfs diskInfo;
    statvfs("/", &diskInfo);

    unsigned long long totalDisk = (diskInfo.f_blocks * diskInfo.f_frsize) / (1024 * 1024);
    unsigned long long freeDisk = (diskInfo.f_bfree * diskInfo.f_frsize) / (1024 * 1024);

    printf("Disque total: %llu MB, Disque libre: %llu MB\n", totalDisk, freeDisk);

    clock_t end = clock(); // Fin du chronométrage
    printf("Temps d'exécution (disque): %.2f ms\n", get_execution_time(start, end));
}

// Fonction pour surveiller l'utilisation réseau
void monitor_network() {
    clock_t start = clock(); // Début du chronométrage

    FILE *file = fopen("/sys/class/net/ens33/statistics/rx_bytes", "r");
    if (file == NULL) {
        perror("Erreur lors de la lecture du réseau (rx_bytes)");
        return;
    }
    unsigned long rx_bytes;
    fscanf(file, "%lu", &rx_bytes);
    fclose(file);
    
    file = fopen("/sys/class/net/ens33/statistics/tx_bytes", "r");
    if (file == NULL) {
        perror("Erreur lors de la lecture du réseau (tx_bytes)");
        return;
    }
    unsigned long tx_bytes;
    fscanf(file, "%lu", &tx_bytes);
    fclose(file);

    printf("Données reçues: %lu bytes, Données envoyées: %lu bytes\n", rx_bytes, tx_bytes);

    clock_t end = clock(); // Fin du chronométrage
    printf("Temps d'exécution (réseau): %.2f ms\n", get_execution_time(start, end));
}

int main() {
    while (1) {
        printf("---- Surveillance des ressources ----\n");

        // Appel des fonctions séquentiellement avec mesure de temps
        monitor_memory();
        monitor_disk();
        monitor_network();

        printf("-------------------------------------\n\n");

        // Pause de 2 secondes avant la prochaine itération
        sleep(2);
    }

    return 0;
}
