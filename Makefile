CC = gcc
CFLAGS = -Wall -std=c99

all: central cliente reportes

central: central.c
		$(CC) $(CFLAGS) -o central central.c

cliente: cliente.c
		$(CC) $(CFLAGS) -o cliente cliente.c

reportes: reportes.c
		$(CC) $(CFLAGS) -o reportes reportes.c

clean:
		rm -rf cliente central reportes chat_log.txt *.o
		rm -rf /tmp/fifo*

run: all
		@echo "Iniciando proceso central y $(NUM) clientes..."
		xterm -e "./central" &
		@for i in `seq 1 $(NUM)`; do \
				xterm -e "./cliente" & \
				sleep 0.5; \
		done
		xterm -e "./reportes"
