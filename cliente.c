#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/fifo_publico"
#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    pid_t mi_pid = getpid();

    printf("PID: %d\n", mi_pid);
    // printf("   Comandos disponibles: /clonar, /salir, reportar <PID>\n\n");

    // Crear FIFOs privados
    char fifo_a_central[50], fifo_a_cliente[50];
    sprintf(fifo_a_central, "/tmp/fifo_a_central_%d", mi_pid);
    sprintf(fifo_a_cliente, "/tmp/fifo_a_cliente_%d", mi_pid);
    umask(0);
    mkfifo(fifo_a_central, 0666);
    mkfifo(fifo_a_cliente, 0666);

    // Abre el FIFO público para establecer la conexión
    int fd_publico = open(FIFO_PATH, O_WRONLY);
    if (fd_publico == -1) {
        perror("abrir FIFO");
        exit(1);
    }
    write(fd_publico, &mi_pid, sizeof(pid_t));
    close(fd_publico);

    int fd_a_central = open(fifo_a_central, O_WRONLY);
    int fd_a_cliente = open(fifo_a_cliente, O_RDONLY);

    while(1) {
        sleep(1);
    }

    return 0;
}