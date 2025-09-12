#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>

#define FIFO_PATH "/tmp/fifo_publico"
#define BUFFER_SIZE 256

struct Cliente {
    pid_t pid;
    int fd_a_central;
    int fd_a_cliente;
};

struct Cliente *clientes = NULL;
int num_clientes = 0;

int main() {
    printf("[Central:%d]: Esperando Conexiones\n", getpid());

    // Conjunto descriptores FIFO y variable buffer
    fd_set read_fds;
    char buffer[BUFFER_SIZE];

    // Crear FIFO con permisos de lectura y escritura
    umask(0);
    mkfifo(FIFO_PATH, 0666);

    //Abrir FIFO en modo de lectura
    int fd_publico = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd_publico == -1) {
        perror("abrir FIFO");
        exit(1);
    }

    while (1) {
        // Vaciar y setear fd publico al conjunto de descriptores
        FD_ZERO(&read_fds);
        FD_SET(fd_publico, &read_fds);
        int fd_max = fd_publico;

        // Añadir descriptores de clientes al conjunto
        for (int i = 0; i < num_clientes; i++) {
            FD_SET(clientes[i].fd_a_central, &read_fds);
            if (clientes[i].fd_a_central > fd_max) {
                fd_max = clientes[i].fd_a_central;
            }
        }

        // Revisa si hay actividad en alguno de los pipes
        int actividad = select(fd_max + 1, &read_fds, NULL, NULL, NULL);
        if (actividad < 0) {
            perror("select()");
            continue;
        }

        // Revisar nuevas conexiones
        if (FD_ISSET(fd_publico, &read_fds)) {
            pid_t nuevo_pid;
            if (read(fd_publico, &nuevo_pid, sizeof(pid_t)) > 0) {
                struct Cliente *aux = realloc(clientes, (num_clientes + 1) * sizeof(struct Cliente));
                if (aux == NULL) {
                    perror("realloc");
                } else {
                    clientes = aux;

                    char fifo_a_central[50], fifo_a_cliente[50];
                    sprintf(fifo_a_central, "/tmp/fifo_a_central_%d", nuevo_pid);
                    sprintf(fifo_a_cliente, "/tmp/fifo_a_cliente_%d", nuevo_pid);

                    clientes[num_clientes].pid = nuevo_pid;
                    clientes[num_clientes].fd_a_central = open(fifo_a_central, O_RDONLY | O_NONBLOCK);
                    clientes[num_clientes].fd_a_cliente = open(fifo_a_cliente, O_WRONLY);
                    
                    num_clientes++;
                    printf("Nueva conexión: PID %d (%d Participantes)\n", nuevo_pid, num_clientes);
                }

                close(fd_publico);
                fd_publico = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
            }
        }

        for (int i = 0; i < num_clientes; i++) {
            if (FD_ISSET(clientes[i].fd_a_central, &read_fds)) {
                char buffer[BUFFER_SIZE];
                ssize_t bytes = read(clientes[i].fd_a_central, buffer, sizeof(buffer) - 1);

                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    char mensaje[BUFFER_SIZE + 20];

                    sprintf(mensaje, "[%d]: %s", clientes[i].pid, buffer);
                    printf("[Central:%d]: [%d]: %s", getpid(), clientes[i].pid, buffer);

                    FILE* log_file = fopen("/tmp/chat_log.txt", "a");
                    if (log_file) {
                        fputs(mensaje, log_file);
                        fclose(log_file);
                    }

                    for (int j = 0; j < num_clientes; j++) {
                        if(i != j) {
                            write(clientes[j].fd_a_cliente, mensaje, strlen(mensaje));
                        }
                    }
                } else if (bytes == 0) {
                    printf("Desconexión: PID %d\n", clientes[i].pid);

                    close(clientes[i].fd_a_central);
                    close(clientes[i].fd_a_cliente);
                    char fifo_a_central[50], fifo_a_cliente[50];
                    sprintf(fifo_a_central, "/tmp/fifo_a_central_%d", clientes[i].pid);
                    sprintf(fifo_a_cliente, "/tmp/fifo_a_cliente_%d", clientes[i].pid);
                    unlink(fifo_a_central);
                    unlink(fifo_a_cliente);

                    if (i != num_clientes - 1) {
                        clientes[i] = clientes[num_clientes - 1];
                    }
                    num_clientes--;

                    if (num_clientes == 0) {
                        free(clientes);
                        clientes = NULL;
                    } else {
                        struct Cliente *aux = realloc(clientes, num_clientes * sizeof(struct Cliente));
                        if (aux == NULL) {
                            perror("realloc");
                        } else {
                            clientes = aux;
                        }
                    }
                    i--;
                    printf("    %d Participantes\n", num_clientes);
                }
            }
        }
    }

    return 0;
}