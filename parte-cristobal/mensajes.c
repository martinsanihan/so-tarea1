#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static const char *fifo_path = "/tmp/mi_fifo";

static void MandarMensajeConPid(const char *mensaje, size_t nbytes, pid_t pid) {
    int fd = open(fifo_path, O_WRONLY);

    if (dprintf(fd, "[PID %ld] %.*s", (long)pid, (int)nbytes, mensaje) < 0) {
        perror("dprintf");
        close(fd);
        return;
    }
    if (nbytes == 0 || mensaje[nbytes - 1] != '\n') {
        if (write(fd, "\n", 1) < 0) perror("write \\n");
    }
    close(fd);
}

static int AbrirNuevaXterm(void) {
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len < 0) { perror("readlink /proc/self/exe"); return -1; }
    exe_path[len] = '\0';

    pid_t t = fork();
    if (t < 0) { perror("fork"); return -1; }
    if (t == 0) {
        execlp("xterm", "xterm", "-T", "mensajes", "-e", exe_path, (char*)NULL);
        perror("execlp xterm");
        _exit(127);
    }
    return 0;
}

int main(void) {
    char *linea = NULL;
    size_t cap = 0;
    ssize_t nread;

    printf("Escribe líneas para enviar por la FIFO (Ctrl+D para salir).\n");
    printf("Comando especial: escribe ':nuevo' para abrir otra terminal de 'mensajes'.");

    while(1) {
        printf("> ");
        fflush(stdout);

        nread = getline(&linea, &cap, stdin);
        if (nread == -1) {
            if (feof(stdin)) printf("\nFin de entrada.");
            else perror("getline");
            break;
        }

        size_t useful = (size_t)nread;
        size_t trimmed = useful;
        if (trimmed && linea[trimmed - 1] == '\n') trimmed--;

        if (trimmed == 6 && strncmp(linea, ":nuevo", 6) == 0) {
            if (AbrirNuevaXterm() == 0) {
                puts("[OK] Se abrió una nueva ventana de xterm con 'mensajes'.");
            } else {
                puts("[ERR] No se pudo abrir xterm.");
            }
            continue;
        }

        MandarMensajeConPid(linea, useful, getpid());
    }

    free(linea);
    return 0;
}
