# Chat Comuniatirio Entre Procesos en Sistemas UNIX

Este proyecto tiene como objetivo construir un chat en donde puedan conversar distintos procesos (clientes), comunicandose con un servidor (proceso central) que mediante pipes bidireccionales (FIFOs) recibirá y reenviará los mensajes para que todos puedan verlos. Además de la funcionalidad de conversar, se tiene un sistema de reportes para poder matar a cualquier proceso que esté teniendo mala conducta, si a un cliente lo reportan 10 veces será eliminado.

## Compilación y Ejecución

En el proyecto hay 5 archivos, 3 códigos en el lenguaje C, 1 Makefile y este README.md. El archivo más importante para la ejecución de este programa es Makefile, este se encargará de compilar los tres códigos, ejecutarlos y abrir un número de clientes determinados por el usuario.

```
make
make run NUM=*num_clientes*
```
En donde *num_clientes* se refiere a la cantidad deseada por el usuario, cabe mencionar que este proyecto asume que en el entorno que se esté ejecutando, se tendrá instalada la terminal **xterm**, ya que esta se puede encontrar instalada por defecto en muchas distribuciones de linux, que es el sistema operativo en donde se realizó este proyecto.

Dentro de Makefile también se encuentra el parámetro **clean** que usando el comando `make clean` eliminará los ejecutables de los programas, además como los archivos basura que se encuentren almacenados por los named pipes.

Una vez ejecutados los comandos `make` y `make run NUM=*num_clientes*` se abrirán 2 terminales, una para el proceso central y otra para el proceso de reportes, además se abriran *num_clientes* cantidad de terminales para clientes distintos, los cuales son totalmente independiente entre ellos. En este momento se puede escribir en las terminales de clientes y estos mensajes aparecerán en el proceso central y les llegarán a los demás clientes. Los clientes pueden desconectarse cuando lo deseen y el proceso central avisará a los demás que hubo una desconexión, y si se quiere abrir un nuevo cliente, se puede en una nueva terminal ejecutando `./cliente` en el directorio del proyecto, el proceso central avisará de una nueva conexión.

![ejecución inicial](<images/Pasted image.png>)

## Funcionamiento

### 1. Conversación

El proceso central se encarga de recibir a los clientes mediante un pipe fifo público, el cual será exclusivamente para saber si llegó un cliente nuevo al chat, en cuanto esto pasa, el nuevo cliente se agrega a una lista dinámica, se crean y se abren los pipes privados para la conversación entre cliente y proceso central. El cliente envía mensajes mediante el pipe privado de escritura y el proceso central los recibe mediante el pipe privado de lectura, aquí identifica qué tipo de mensaje es, ya que puede ser un mensaje normal o un reporte. Si el mensaje era uno normal, se reenviará a todos los clientes, si era uno del formato "reportar PID" lo enviará por pipes exclusivos para reportes hacía el proceso reportero, este es el que procesará el PID entregado y decidirá que hacer con el cliente.

![mensajes normales](<images/Pasted image (2).png>)

Además de sólo imprimir y enviar los mensajes a los demás clientes, el proceso central lleva un registro de la totalidad del chat en un archivo "chat_log.txt" que se crea en el mismo directorio del proyecto.

### 2. Clonar Clientes
Uno de los dos comandos disponibles es "/clonar" que básicamente crea un hijo que ejecuta el mismo código, como cualquier cliente, por lo tanto, este también participa de la conversación con todas las funcionalidades.

![alt text](<images/Screencast from 2025-09-12 17-32-34.gif>)

### 3. Reportes
Cada vez que se recibe un reporte el proceso reportero imprime el pid del cliente reportado, como medida de seguridad, sólo se pueden reportar PIDs que estén en la lista de clientes conectados al proceso central, para no poder matar a procesos que no tengan nada que ver con el chat. Al identificar que un usuario tiene 10 reportes, el proceso encargado de eso automáticamente mata al proceso.

![reportes](<images/Pasted image (3).png>)

