#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

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
    int n;
    printf("Cuantos chat quiere crear ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++)
    {
        pid_t t=fork();
        if(t==0){
        execlp("xterm", "xterm","-T", "mensajes","-e", "/home/ubuntu/Escritorio/vs/mensajes",(char*)NULL);
        }
        
    }
    while (1)
    {
        LeerFifoMensajes();
    }
    
}