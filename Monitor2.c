#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define BUFFER_SIZE 256

// Fonction pour mesurer le temps d'exécution en millisecondes
double get_execution_time(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
}

// Fonction de surveillance de la mémoire
void monitor_memory(int pipe_fd) {
    clock_t start = clock();

    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long totalMemory = memInfo.totalram / (1024 * 1024);
    long freeMemory = memInfo.freeram / (1024 * 1024);

    clock_t end = clock();
    double execution_time = get_execution_time(start, end);

    // Formatage des données en chaîne de caractères
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Mémoire totale: %ld MB, Mémoire libre: %ld MB, Temps: %.2f ms\n", totalMemory, freeMemory, execution_time);

    // Écriture dans le pipe
    write(pipe_fd, buffer, strlen(buffer) + 1);
    close(pipe_fd);
}

// Fonction de surveillance du disque
void monitor_disk(int pipe_fd) {
    clock_t start = clock();

    struct statvfs diskInfo;
    statvfs("/", &diskInfo);
    unsigned long long totalDisk = (diskInfo.f_blocks * diskInfo.f_frsize) / (1024 * 1024);
    unsigned long long freeDisk = (diskInfo.f_bfree * diskInfo.f_frsize) / (1024 * 1024);

    clock_t end = clock();
    double execution_time = get_execution_time(start, end);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Disque total: %llu MB, Disque libre: %llu MB, Temps: %.2f ms\n", totalDisk, freeDisk, execution_time);

    write(pipe_fd, buffer, strlen(buffer) + 1);
    close(pipe_fd);
}

// Fonction de surveillance de l'utilisation réseau
void monitor_network(int pipe_fd) {
    clock_t start = clock();

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

    clock_t end = clock();
    double execution_time = get_execution_time(start, end);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Données reçues: %lu bytes, Données envoyées: %lu bytes, Temps: %.2f ms\n", rx_bytes, tx_bytes, execution_time);

    write(pipe_fd, buffer, strlen(buffer) + 1);
    close(pipe_fd);
}

int main() {
    int memory_pipe[2], disk_pipe[2], network_pipe[2];
    pipe(memory_pipe); // Crée un pipe pour la mémoire
    pipe(disk_pipe);   // Crée un pipe pour le disque
    pipe(network_pipe); //à Crée un pipe pour le réseau

    pid_t memory_pid = fork();
    if (memory_pid == 0) {
        close(memory_pipe[0]); // Ferme le côté lecture
        monitor_memory(memory_pipe[1]);
        exit(0);
    }

    pid_t disk_pid = fork();
    if (disk_pid == 0) {
        close(disk_pipe[0]); // Ferme le côté lecture
        monitor_disk(disk_pipe[1]);
        exit(0);
    }

    pid_t network_pid = fork();
    if (network_pid == 0) {
        close(network_pipe[0]); // Ferme le côté lecture
        monitor_network(network_pipe[1]);
        exit(0);
    }

    // Processus parent : lit les données envoyées par chaque processus fils
    close(memory_pipe[1]); // Ferme le côté écriture des pipes
    close(disk_pipe[1]);
    close(network_pipe[1]);

    char buffer[BUFFER_SIZE];
    while (1) {
        printf("---- Surveillance des ressources ----\n");

        // Lecture des données du processus de surveillance de la mémoire
        read(memory_pipe[0], buffer, BUFFER_SIZE);
        printf("%s", buffer);

        // Lecture des données du processus de surveillance du disque
        read(disk_pipe[0], buffer, BUFFER_SIZE);
        printf("%s", buffer);

        // Lecture des données du processus de surveillance du réseau
        read(network_pipe[0], buffer, BUFFER_SIZE);
        printf("%s", buffer);

        printf("-------------------------------------\n\n");

        sleep(2); // Pause avant la prochaine itération
    }

    close(memory_pipe[0]);
    close(disk_pipe[0]);
    close(network_pipe[0]);

    // Attendre la fin des processus fils
    waitpid(memory_pid, NULL, 0);
    waitpid(disk_pid, NULL, 0);
    waitpid(network_pid, NULL, 0);

    return 0;
}
