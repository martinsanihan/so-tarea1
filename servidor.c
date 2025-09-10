#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>

#define MAX_CLIENTES 10
#define FIFO_PUBLICO "/tmp/chat_fifo_publico"
#define MAX_BUFFER 256

// --- Estructuras de Datos ---
struct Cliente {
    pid_t pid;
    int fd_a_servidor; // Pipe para leer lo que el cliente envía
    int fd_a_cliente;  // Pipe para escribirle al cliente
};

struct Reporte {
    pid_t pid;
    int count;
};

// --- Variables Globales ---
struct Cliente clientes[MAX_CLIENTES];
int num_clientes = 0;

struct Reporte reportes[MAX_CLIENTES];
int num_reportes = 0;

// --- Manejador de Señal para Reportes ---
void manejador_sigusr1(int sig) {
    FILE *archivo_reportes = fopen("/tmp/reportes.txt", "w");
    if (archivo_reportes == NULL) {
        perror("fopen en manejador");
        return;
    }
    for (int i = 0; i < num_reportes; i++) {
        fprintf(archivo_reportes, "%d %d\n", reportes[i].pid, reportes[i].count);
    }
    fclose(archivo_reportes);
    // Escribir en la consola del servidor para confirmar que la señal fue recibida
    write(STDOUT_FILENO, "Reporte generado en /tmp/reportes.txt\n", 39);
}

// --- Función Principal ---
int main() {
    // Inicialización
    memset(clientes, 0, sizeof(clientes));
    memset(reportes, 0, sizeof(reportes));
    signal(SIGUSR1, manejador_sigusr1);

    printf("✅ Servidor iniciado. PID: %d\n", getpid());
    printf("   Esperando clientes en %s\n", FIFO_PUBLICO);

    // Crear FIFO público
    umask(0);
    mkfifo(FIFO_PUBLICO, 0666);
    int fd_publico = open(FIFO_PUBLICO, O_RDONLY | O_NONBLOCK);
    if (fd_publico == -1) {
        perror("open fifo publico");
        exit(1);
    }

    fd_set read_fds;
    int max_fd;

    // Bucle principal del servidor
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(fd_publico, &read_fds);
        max_fd = fd_publico;

        // Añadir los FDs de los clientes al set
        for (int i = 0; i < num_clientes; i++) {
            FD_SET(clientes[i].fd_a_servidor, &read_fds);
            if (clientes[i].fd_a_servidor > max_fd) {
                max_fd = clientes[i].fd_a_servidor;
            }
        }

        // Esperar actividad en alguno de los FDs
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            continue;
        }

        // 1. Revisar si hay una nueva conexión en el FIFO público
        if (FD_ISSET(fd_publico, &read_fds)) {
            pid_t nuevo_pid;
            if (read(fd_publico, &nuevo_pid, sizeof(pid_t)) > 0 && num_clientes < MAX_CLIENTES) {
                // Crear pipes privados para el nuevo cliente
                char fifo_a_servidor[50], fifo_a_cliente[50];
                sprintf(fifo_a_servidor, "/tmp/fifo_a_serv_%d", nuevo_pid);
                sprintf(fifo_a_cliente, "/tmp/fifo_a_cli_%d", nuevo_pid);
                mkfifo(fifo_a_servidor, 0666);
                mkfifo(fifo_a_cliente, 0666);

                // Añadir cliente a la lista
                clientes[num_clientes].pid = nuevo_pid;
                clientes[num_clientes].fd_a_servidor = open(fifo_a_servidor, O_RDONLY | O_NONBLOCK);
                clientes[num_clientes].fd_a_cliente = open(fifo_a_cliente, O_WRONLY);
                num_clientes++;
                
                printf("➕ Cliente %d conectado.\n", nuevo_pid);

                // Reabrir el FIFO público para aceptar más clientes
                close(fd_publico);
                fd_publico = open(FIFO_PUBLICO, O_RDONLY | O_NONBLOCK);
            }
        }

        // 2. Revisar si hay mensajes de clientes existentes
        for (int i = 0; i < num_clientes; i++) {
            if (FD_ISSET(clientes[i].fd_a_servidor, &read_fds)) {
                char buffer[MAX_BUFFER];
                ssize_t bytes_read = read(clientes[i].fd_a_servidor, buffer, sizeof(buffer) - 1);

                if (bytes_read > 0) { // Mensaje recibido
                    buffer[bytes_read] = '\0';
                    char mensaje_formateado[MAX_BUFFER + 20];
                    
                    // Lógica de reportes
                    if (strncmp(buffer, "reportar ", 9) == 0) {
                        pid_t pid_a_reportar = atoi(buffer + 9);
                        printf("🚨 Reporte recibido para el PID %d de parte de %d\n", pid_a_reportar, clientes[i].pid);
                        
                        // Buscar si ya existe un reporte para este PID
                        int encontrado = 0;
                        for(int j=0; j < num_reportes; j++) {
                            if (reportes[j].pid == pid_a_reportar) {
                                reportes[j].count++;
                                encontrado = 1;
                                break;
                            }
                        }
                        // Si no existe, crearlo
                        if (!encontrado && num_reportes < MAX_CLIENTES) {
                            reportes[num_reportes].pid = pid_a_reportar;
                            reportes[num_reportes].count = 1;
                            num_reportes++;
                        }
                    } else { // Mensaje normal
                        sprintf(mensaje_formateado, "[%d]: %s", clientes[i].pid, buffer);
                        printf(" BROADCAST: %s", mensaje_formateado);
                        
                        // Guardar en log
                        FILE* log_file = fopen("chat_log.txt", "a");
                        if (log_file) {
                            fputs(mensaje_formateado, log_file);
                            fclose(log_file);
                        }

                        // Reenviar a todos los demás clientes
                        for (int j = 0; j < num_clientes; j++) {
                            if (i != j) {
                                write(clientes[j].fd_a_cliente, mensaje_formateado, strlen(mensaje_formateado));
                            }
                        }
                    }
                } else if (bytes_read == 0) { // Cliente desconectado
                    printf("➖ Cliente %d desconectado.\n", clientes[i].pid);
                    
                    // Cerrar y eliminar pipes
                    close(clientes[i].fd_a_servidor);
                    close(clientes[i].fd_a_cliente);
                    char fifo_a_servidor[50], fifo_a_cliente[50];
                    sprintf(fifo_a_servidor, "/tmp/fifo_a_serv_%d", clientes[i].pid);
                    sprintf(fifo_a_cliente, "/tmp/fifo_a_cli_%d", clientes[i].pid);
                    unlink(fifo_a_servidor);
                    unlink(fifo_a_cliente);

                    // Eliminar de la lista de clientes
                    for (int j = i; j < num_clientes - 1; j++) {
                        clientes[j] = clientes[j + 1];
                    }
                    num_clientes--;
                    i--; // Ajustar índice del bucle
                }
            }
        }
    }

    close(fd_publico);
    unlink(FIFO_PUBLICO);
    return 0;
}