#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <limits.h>

#define MAX_CLIENTES INT_MAX
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

void CrearFifo(){
const char *fifo_path = "/tmp/mi_fifo";
    mkfifo(fifo_path, 0666);
    printf("FIFO creada en %s\n", fifo_path);
}

void LeerFifoMensajes(){
     const char *fifo_path = "/tmp/mi_fifo";
    int fd = open(fifo_path, O_RDONLY);

    char buffer[128];
    int n = read(fd, buffer, sizeof(buffer));
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    close(fd);
}

int main(){
    CrearFifo();
    while (1)
    {
        LeerFifoMensajes();
    }
    
}