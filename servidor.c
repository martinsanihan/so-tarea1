#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>

#define FIFO_PUBLICO "/tmp/chat_fifo_publico"
#define MAX_BUFFER 256

struct Cliente {
    pid_t pid;
    int fd_a_servidor; // Pipe privado para leer lo que el cliente envía
    int fd_a_cliente;  // Pipe privado para escribirle al cliente
};

struct Reporte {
    pid_t pid;
    int count;
};

struct Cliente *clientes = NULL;
int num_clientes = 0;

struct Reporte *reportes = NULL;
int num_reportes = 0;

// --- Manejador de Señal para Reportes ---
void maneja_señal_reportes(int sig) {
    FILE *archivo_reportes = fopen("/tmp/reportes.txt", "w");
    if (archivo_reportes == NULL) {
        perror("fopen");
        return;
    }
    for (int i = 0; i < num_reportes; i++) {
        fprintf(archivo_reportes, "%d %d\n", reportes[i].pid, reportes[i].count);
    }
    fclose(archivo_reportes);
    write(STDOUT_FILENO, "Reportes generados en /tmp/reportes.txt\n", 39);
}

int main() {
    signal(SIGUSR1, maneja_señal_reportes);

    printf("Central PID: %d\n", getpid());
    printf("   Esperando clientes...\n");

    umask(0);
    mkfifo(FIFO_PUBLICO, 0666);
    int fd_publico = open(FIFO_PUBLICO, O_RDONLY | O_NONBLOCK);
    if (fd_publico == -1) {
        perror("open fifo publico");
        exit(1);
    }

    fd_set read_fds;
    int max_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(fd_publico, &read_fds);
        max_fd = fd_publico;

        for (int i = 0; i < num_clientes; i++) {
            FD_SET(clientes[i].fd_a_servidor, &read_fds);
            if (clientes[i].fd_a_servidor > max_fd) {
                max_fd = clientes[i].fd_a_servidor;
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            continue;
        }

        // 1. Revisar si hay una nueva conexión
        if (FD_ISSET(fd_publico, &read_fds)) {
            pid_t nuevo_pid;
            if (read(fd_publico, &nuevo_pid, sizeof(pid_t)) > 0) {
                // *** CAMBIO: Usar realloc para añadir espacio para el nuevo cliente ***
                struct Cliente *clientes_nuevos = realloc(clientes, (num_clientes + 1) * sizeof(struct Cliente));
                if (clientes_nuevos == NULL) {
                    perror("realloc para nuevo cliente");
                    // Opcional: informar al cliente que no se puede conectar
                } else {
                    clientes = clientes_nuevos;
                    
                    char fifo_a_servidor[50], fifo_a_cliente[50];
                    sprintf(fifo_a_servidor, "/tmp/fifo_a_serv_%d", nuevo_pid);
                    sprintf(fifo_a_cliente, "/tmp/fifo_a_cli_%d", nuevo_pid);
                    mkfifo(fifo_a_servidor, 0666);
                    mkfifo(fifo_a_cliente, 0666);

                    clientes[num_clientes].pid = nuevo_pid;
                    clientes[num_clientes].fd_a_servidor = open(fifo_a_servidor, O_RDONLY | O_NONBLOCK);
                    clientes[num_clientes].fd_a_cliente = open(fifo_a_cliente, O_WRONLY);
                    num_clientes++;
                    
                    printf("➕ Cliente %d conectado. Total: %d\n", nuevo_pid, num_clientes);
                }
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

                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    char mensaje_formateado[MAX_BUFFER + 20];
                    
                    if (strncmp(buffer, "reportar ", 9) == 0) {
                        pid_t pid_a_reportar = atoi(buffer + 9);
                        printf("🚨 Reporte recibido para el PID %d de parte de %d\n", pid_a_reportar, clientes[i].pid);
                        
                        int encontrado = 0;
                        for(int j=0; j < num_reportes; j++) {
                            if (reportes[j].pid == pid_a_reportar) {
                                reportes[j].count++;
                                encontrado = 1;
                                break;
                            }
                        }
                        if (!encontrado) {
                            // *** CAMBIO: Usar realloc para añadir un nuevo reporte ***
                            struct Reporte *reportes_nuevos = realloc(reportes, (num_reportes + 1) * sizeof(struct Reporte));
                             if (reportes_nuevos == NULL) {
                                perror("realloc para nuevo reporte");
                            } else {
                                reportes = reportes_nuevos;
                                reportes[num_reportes].pid = pid_a_reportar;
                                reportes[num_reportes].count = 1;
                                num_reportes++;
                            }
                        }
                    } else {
                        sprintf(mensaje_formateado, "[%d]: %s", clientes[i].pid, buffer);
                        printf(" BROADCAST: %s", mensaje_formateado);
                        
                        FILE* log_file = fopen("chat_log.txt", "a");
                        if (log_file) {
                            fputs(mensaje_formateado, log_file);
                            fclose(log_file);
                        }

                        for (int j = 0; j < num_clientes; j++) {
                            if (i != j) {
                                write(clientes[j].fd_a_cliente, mensaje_formateado, strlen(mensaje_formateado));
                            }
                        }
                    }
                } else if (bytes_read == 0) { // Cliente desconectado
                    printf("➖ Cliente %d desconectado.\n", clientes[i].pid);
                    
                    close(clientes[i].fd_a_servidor);
                    close(clientes[i].fd_a_cliente);
                    char fifo_a_servidor[50], fifo_a_cliente[50];
                    sprintf(fifo_a_servidor, "/tmp/fifo_a_serv_%d", clientes[i].pid);
                    sprintf(fifo_a_cliente, "/tmp/fifo_a_cli_%d", clientes[i].pid);
                    unlink(fifo_a_servidor);
                    unlink(fifo_a_cliente);

                    // *** CAMBIO: Eliminar cliente y encoger el array ***
                    // Mueve el último cliente a la posición del que se va (si no es el mismo)
                    if (i != num_clientes - 1) {
                        clientes[i] = clientes[num_clientes - 1];
                    }
                    num_clientes--;
                    // Encoge el array de clientes
                    if (num_clientes == 0) {
                        free(clientes);
                        clientes = NULL;
                    } else {
                        struct Cliente *clientes_nuevos = realloc(clientes, num_clientes * sizeof(struct Cliente));
                        if (clientes_nuevos == NULL) {
                             perror("realloc al eliminar cliente");
                        } else {
                             clientes = clientes_nuevos;
                        }
                    }
                    i--; // Ajustar índice del bucle
                    printf("   Clientes restantes: %d\n", num_clientes);
                }
            }
        }
    }

    // Limpieza final (este código es difícil de alcanzar en un servidor infinito)
    close(fd_publico);
    unlink(FIFO_PUBLICO);
    free(clientes);
    free(reportes);
    return 0;
}