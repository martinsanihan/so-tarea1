#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FIFO_PUBLICO "/tmp/chat_fifo_publico"
#define MAX_BUFFER 256

// Variable global para almacenar el PID del proceso lector
pid_t pid_lector;

// Manejador para salir limpiamente
void manejador_sigint(int sig) {
    printf("\nPara salir, escribe '/salir'.\n> ");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, manejador_sigint);
    pid_t mi_pid = getpid();

    printf("👋 ¡Bienvenido al Chat! Tu ID es: %d\n", mi_pid);
    printf("   Comandos disponibles: /clonar, /salir, reportar <PID>\n\n");

    // 1. Crear los FIFOs privados ANTES de contactar al servidor
    char fifo_a_servidor[50], fifo_a_cliente[50];
    sprintf(fifo_a_servidor, "/tmp/fifo_a_serv_%d", mi_pid);
    sprintf(fifo_a_cliente, "/tmp/fifo_a_cli_%d", mi_pid);
    umask(0);
    mkfifo(fifo_a_servidor, 0666);
    mkfifo(fifo_a_cliente, 0666);
    
    // 2. Conectarse al FIFO público del servidor para anunciarse
    int fd_publico = open(FIFO_PUBLICO, O_WRONLY);
    if (fd_publico == -1) {
        perror("No se pudo abrir el FIFO publico (¿está el servidor corriendo?)");
        exit(1);
    }
    write(fd_publico, &mi_pid, sizeof(pid_t));
    close(fd_publico);

    // 3. Abrir los FIFOs privados para la comunicación
    int fd_write = open(fifo_a_servidor, O_WRONLY);
    // Este open es bloqueante, espera a que el servidor abra el otro extremo
    int fd_read = open(fifo_a_cliente, O_RDONLY);
    
    // 4. Bifurcar para manejar lectura y escritura simultáneamente
    pid_lector = fork();

    if (pid_lector == -1) {
        perror("fork");
        exit(1);
    }

    if (pid_lector == 0) { // --- Proceso Hijo (Lector) ---
        char buffer_read[MAX_BUFFER];
        ssize_t bytes_read;
        while ((bytes_read = read(fd_read, buffer_read, sizeof(buffer_read) - 1)) > 0) {
            buffer_read[bytes_read] = '\0';
            printf("\r%s> ", buffer_read); // \r para volver al inicio de la línea
            fflush(stdout);
        }
        printf("\nEl servidor se ha desconectado.\n");
        close(fd_read);
        exit(0);
    } else { // --- Proceso Padre (Escritor) ---
        char buffer_write[MAX_BUFFER];
        printf("> ");
        fflush(stdout);

        while (fgets(buffer_write, sizeof(buffer_write), stdin)) {
            // Comando para salir
            if (strcmp(buffer_write, "/salir\n") == 0) {
                break;
            }
            // Comando para clonar
            else if (strcmp(buffer_write, "/clonar\n") == 0) {
                pid_t pid_clon = fork();
                if (pid_clon == 0) { // El nuevo proceso (clon)
                    // Reemplaza su imagen con una nueva instancia de cliente
                    printf("Clonando proceso... abriendo nueva terminal.\n");
                    // Usamos execlp para que busque el ejecutable en el PATH
                    // Esto asume que tienes un terminal como 'xterm' o 'gnome-terminal'
                    execlp("xterm", "xterm", "-e", argv[0], NULL);
                    perror("execlp fallo"); // Esto solo se ejecuta si execlp falla
                    exit(1);
                }
            }
            // Mensaje normal
            else {
                write(fd_write, buffer_write, strlen(buffer_write));
            }
            printf("> ");
            fflush(stdout);
        }
        // Limpieza al salir
        printf("\nDesconectando...\n");
        kill(pid_lector, SIGKILL); // Matar al proceso hijo lector
        wait(NULL); // Esperar a que el hijo termine
        close(fd_write);
        close(fd_read);
        unlink(fifo_a_servidor);
        unlink(fifo_a_cliente);
    }

    return 0;
}