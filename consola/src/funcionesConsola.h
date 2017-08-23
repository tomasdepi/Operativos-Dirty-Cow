/*
 * consola.h
 *
 *  Created on: 4/4/2017
 *      Author: utnso
 */

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <commons/string.h>
#include <commons/sockets.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/interface.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_COMMAND_SIZE 256

#define IniciarProceso "iniciarProceso"
#define configuracionConsola "../confConsola.init"

typedef struct{
	char* ip_Kernel;
	char* puerto_Kernel;
	char* program_path;
}t_config_consola;

typedef struct{
	int pid;
	int socket;
	pthread_t thread;
	struct tm* fechaInicio;
	struct tm* fechaFin;
	struct timespec start;
	struct timespec end;
	int impresiones;
}t_proceso;

typedef struct{
	char* pathAnsisop;
	int socket;

}dataHilo;

void crearConfig(int argc,char* argv[]);
t_config_consola* levantarConfiguracionConsola(char * archivo);
int crearLog();

int enviarArchivo(int kernel_fd, char* path);

void inicializacion(void);
t_proceso* crearProceso(int socketProceso, pthread_t threadPrograma, int pid);
bool esNumero(char* string);
void threadPrograma(dataHilo* data);
void terminarProceso(t_proceso* proc, int32_t exitCode);
void imprimirInformacion(t_proceso* proceso, int32_t exitCode);
void imprimirInformacionProcesoRechazado(int pid, int32_t exitCode);
void notificarProcesoRechazado(void* paquete, char* pathAnsisop);
void finalizarEjecucionProceso(bool* procesoActivo, dataHilo* data, int32_t exitCode);
void cargarFechaFin(t_proceso* proc);
char* obtenerExitCode(int32_t exitCode);
void imprimirPorPantalla(void* buffer);


//Funciones de interfaz
void levantarInterfaz();
void iniciarPrograma(char* comando, char* param);
void finalizarPrograma(char* comando, char* param);
void desconectarConsola(char* comando, char* param);
void limpiarMensajes(char* comando, char* param);
void showHelp(char* comando, char* param);

//Creo el path con la direccion del archivo de config y el nombre del archivo seguido por .ansisop
char* crearPath(char* program_name);


//Variables Globales

t_log* logger;
t_config_consola* config;
pthread_t threadInterfaz;
t_list* procesos;

#endif /* FUNCIONESCONSOLA_H_ */
