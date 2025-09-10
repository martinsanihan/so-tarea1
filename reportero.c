#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#define ARCHIVO_REPORTES "/tmp/reportes.txt"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <PID_DEL_SERVIDOR>\n", argv[0]);
        return 1;
    }

    pid_t pid_servidor = atoi(argv[1]);
    if (pid_servidor <= 0) {
        fprintf(stderr, "PID del servidor inválido.\n");
        return 1;
    }

    printf("📋 Solicitando reporte al servidor con PID %d...\n", pid_servidor);

    // 1. Enviar señal SIGUSR1 al servidor para que genere el reporte
    if (kill(pid_servidor, SIGUSR1) == -1) {
        perror("kill (¿es correcto el PID del servidor?)");
        return 1;
    }

    // 2. Darle tiempo al servidor para escribir el archivo
    printf("   Esperando 1 segundo a que se genere el archivo...\n");
    sleep(1);

    // 3. Abrir y leer el archivo de reportes
    FILE *f_reportes = fopen(ARCHIVO_REPORTES, "r");
    if (f_reportes == NULL) {
        perror("No se pudo abrir el archivo de reportes");
        fprintf(stderr, "Asegúrate de que el servidor esté corriendo y tenga permisos para escribir en /tmp.\n");
        return 1;
    }

    printf("🔎 Procesando reportes:\n");
    pid_t pid_reportado;
    int conteo;
    int acciones_tomadas = 0;

    // Leer el archivo línea por línea en el formato "PID CONTEO"
    while (fscanf(f_reportes, "%d %d", &pid_reportado, &conteo) == 2) {
        printf("   - PID %d tiene %d reportes.\n", pid_reportado, conteo);
        if (conteo > 10) {
            printf("   🔥 ¡ACCIÓN! Terminando proceso %d por exceso de reportes (>10).\n", pid_reportado);
            kill(pid_reportado, SIGTERM);
            acciones_tomadas++;
        }
    }

    if (acciones_tomadas == 0) {
        printf("✅ No se requirieron acciones.\n");
    }

    fclose(f_reportes);
    return 0;
}