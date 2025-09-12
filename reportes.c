#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>

#define FIFO_REPORT "/tmp/fifo_reportes"
#define BUFFER_SIZE 256

struct Reporte {
    pid_t pid;
    int count;
};

struct Reporte *reportes = NULL;
int num_reportes = 0;



int main(int argc, char *argv[]) {
    printf("Proceso de reportes corriendo...\n");
    fflush(stdout);

    fd_set read_fd;

    int fd_report = open(FIFO_REPORT, O_RDONLY | O_NONBLOCK);
    if (fd_report == -1) {
        perror("abrir FIFO");
        exit(1);
    }

    while(1) {
        FD_ZERO(&read_fd);
        FD_SET(fd_report, &read_fd);

        int actividad = select(fd_report + 1, &read_fd, NULL, NULL, NULL);
        if (actividad < 0) {
            perror("select()");
            continue;
        }

        if (FD_ISSET(fd_report, &read_fd)) {
            char reporte[10];
            ssize_t bytes = read(fd_report, reporte, sizeof(reporte) - 1);

            if (bytes > 0) {
                reporte[bytes] = '\0';
                pid_t pid_reportado;
                bool encontrado = false;
                sscanf(reporte, "%d", &pid_reportado);
                for (int i = 0; i < num_reportes; i++) {
                    if (pid_reportado == reportes[i].pid) {
                        encontrado = true;
                        reportes[i].count++;
                        if (reportes[i].count >= 10) {
                            kill(pid_reportado, SIGKILL);
                        }
                        break;
                    }
                }

                if (!encontrado) {
                    struct Reporte *aux = realloc(reportes, (num_reportes + 1) * sizeof(struct Reporte));
                    if (aux == NULL) {
                        perror("realloc");
                    } else {
                        reportes = aux;

                        reportes[num_reportes].pid = pid_reportado;
                        reportes[num_reportes].count = 1;
                        num_reportes++;
                    }
                }
                printf("%s\n", reporte);
                fflush(stdout);
            }
        }
    }
    return 0;
}