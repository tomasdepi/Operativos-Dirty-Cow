/*
 * kernel.h
 *
 *  Created on: 4/4/2017
 *      Author: utnso
 */

#ifndef FUNCIONESKERNEL_H_
#define FUNCIONESKERNEL_H_


#define IP "192.168.3.3"
#define BACKLOG 10
#define configuracionKernel "../confKernel.init"
#define MAX_LEN_PUERTO 6
#define MAX_LEN_IP 20

#include <stdio.h>
#include <stdlib.h>
#include <commons/sockets.h>
#include <string.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/interface.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/cosas.h>
#include <stdbool.h>
#include <pthread.h>
//#include "plp.h"
//#include "pcp.h"
#include "pcb.h"
#include "operaciones.h"
#include <sys/inotify.h>

typedef struct {
	char* ip_kernel;
	char* puerto_PROG;
	char* puerto_CPU;
	char* ip_Memoria;
	char* puerto_Memoria;
	char* ip_FS;
	char* puerto_FS;
	int quantum;
	int quantum_Sleep;
	char* algoritmo;
	int grado_MultiProg;
	t_dictionary* semaforos;
	t_dictionary* variablesGlobales;
	int stack_Size;
}t_config_kernel;

typedef struct{
	uint32_t pid;
	uint32_t cant_pag;
}__attribute__((__packed__)) t_pedido_iniciar;

typedef struct{
	int socket; //funciona como id del cpu
	t_pcb* pcb;
	bool disponible;
}cpu_t;

typedef struct{
	int socketConsola;
	char* codigo;
	int pid;
}proceso_en_espera_t;

typedef struct{
	uint32_t pid;
	uint8_t estado;
	uint32_t socketConsola;
	uint32_t cantRafagas;
	uint32_t cantSyscalls;
	uint32_t cantOpPrivi;
	uint32_t cantAlocar;
	uint32_t cantLiberar;
	uint32_t cantBytesAlocar;
	uint32_t cantBytesLiberar;
	bool matarSiguienteRafaga;
	uint32_t exitCode;
	uint32_t cantPagReservar;
	uint32_t cantPagLiberar;
}info_estadistica_t;

typedef struct{
	char* archivo;
	int vecesAbierto;
	int ubicacion;
}entrada_tabla_globlal_archivo;

typedef struct{
	int proceso; //id del proceso
	t_list* archivos;
}entrada_tabla_archivo_proceso;

typedef struct{
	int fd;
	char* flags;
	int globalFD;
	int cursor;
}t_archivo;

typedef struct{
	uint32_t pid;
	uint32_t pag;
	uint32_t size;
	uint32_t cant;
}__attribute__((__packed__))reserva_memoria;

typedef struct{
	uint32_t used;
	uint32_t size;
}__attribute__((__packed__))meta_bloque;

typedef struct{
	uint32_t pid;
	uint32_t pagBase;
	uint32_t cant;
}__attribute__((__packed__))pedido_mem;

typedef struct{
	uint32_t used;
	uint32_t size;
	uint32_t pos;
}__attribute__((__packed__))t_bloque;

typedef struct{
	uint32_t pid;
	t_list* list;
}t_entrada_datos;

//para comunicacion con fs
typedef struct{
	char* path;
	int offset;
	int size;
} pedido_obtener_datos;

typedef struct{
	char* path;
	int offset;
	int size;
	char* buffer;
}pedido_guardar_datos;

enum enum_estado{
	NEW = 1,
	READY = 2,
	FINISH = 3,
	EXEC = 4,
	BLOQ = 5
};

void inicializarColas(void);
void inicializaciones(void);
void crearConfig(int argc, char* argv[]);
t_config_kernel* levantarConfiguracionKernel(char* archivo_conf);
bool verificarConfig(t_config*);
void destruirConfiguracionKernel(t_config_kernel* config);
t_dictionary* crearDiccionarioConValue(char** array, char** valores);
t_dictionary* crearDiccionario(char** array);
void modificarValorDiccionario(t_dictionary* dic, char* key, void* data);
int semaforoSignal(t_dictionary* dic, char* key);
int semaforoWait(t_dictionary* dic, char* key);
int leerVariableGlobal(t_dictionary* dic, char* key);
void escribirVariableGlobal(t_dictionary* dic, char* key, int nuevoValor);

void establecerConexiones(void);
int conexionConFileSystem(void);
int conexionConMemoria(void);
void trabajarConexionCPU(void);
void enviarTamanioStack(int fd);
void enviarQuantumSleep(int fd);

void conectarConServidores(void);
void escucharConexiones(void);
void aceptarNuevaConexion(int socketEscucha, fd_set* set);
info_estadistica_t* buscarInformacionPorSocketConsola(int socketConsola);


//Mensajes con consola
void trabajarConexionConsola(void);
void procesarMensajeConsola(int consola_fd, int mensaje, char* package);
proceso_en_espera_t* crearProcesoEnEspera(int consola_fd, char* package);
int asignarPid(void);

//Funciones de interfaz
void levantarInterfaz(void);
void listProcesses(char* comando, char* param);
void processInfo(char* comando, char* param);
void getTablaArchivos(char* comando, char* param);
void gradoMultiprogramacion(char* comando, char* param);
void killProcess(char*,char*);
void stopPlanification(char*,char*);
void showHelp(char* comando, char* param);

//Funciones de manejo de CPUs
void agregarNuevaCPU(t_list* lista, int socketCPU);
void liberarCPU(cpu_t* cpu);
void eliminarCPU(t_list* lista, int socketCPU);
void actualizarReferenciaPCB(int id, t_pcb* pcb);
cpu_t* obtenerCpuLibre(void);
cpu_t *obtener_cpu_por_socket_asociado(int socket_asociado);
void desocupar_cpu(int socket_asociado);

//Planificacion
void lanzarHilosPlanificacion(void);
pthread_t hiloPLP;
pthread_t hiloPCP;

//Planificar Corto Plazo
void planificarCortoPlazo(void);
void enviarPcbCPU(t_pcb* pcb, int socketCPU);

//Planificacion Largo Plazo
void planificarLargoPlazo(void);
void alertarConsolaProcesoAceptado(int* pid, int socketConsola);
void envioCodigoMemoria(char* codigo, int fd, int cant_pag);

//Palnificacion Mediano Plazo
void crearColasBloqueados(char** semaforos);
void desbloquearProceso(char* semaforo);
void bloquearProceso(char* semaforo, t_pcb* pcb);

//estadisticas (para consola del kernel)
void crearInfoEstadistica(int pid, uint32_t socketConsola);
info_estadistica_t* buscarInformacion(int pid);
void estadisticaAumentarRafaga(int pid);
void estadisticaAumentarSyscall(int pid);
void estadisticaAumentarOpPriviligiada(int pid);
void estadisticaAumentarAlocar(int pid);
void estadisticaAumentarLiberar(int pid);
void estadisticaAlocarBytes(int pid, int cant);
void estadisticaLiberarBytes(int pid, int cant);
void estadisticaCambiarEstado(int pid, uint8_t nuevoEstado);
void aumentarEstadisticaPorSocketAsociado(int socket, void(*estadistica)(int pid));
void eliminarEstadistica(int pid);
void finalizacion_proceso(void* paquete_from_cpu, int socket_cpu_asociado);
void verificarProcesosEnCpuCaida(int socketCPU);
void verificarProcesosConsolaCaida(uint32_t socketConsola);
void quitarDeMemoriaDinamica(int pid);

//Variables Globales
t_config_kernel* config;
int socketConexionFS;
int socketConexionMemoria;
int max_pid;
int cantProcesosSistema;
int pagina_size;
t_log* logger;

//Sacar Comentarios siguientes.
//Semaforo Limita la cantidad de procesos que pueden accesar a un sector critico.
//mutex solo permite que uno a la vez entre.
//Semaforos
sem_t sem_cola_ready;
sem_t semaforo_cantidiad_procesos_en_ready;
sem_t sem_cola_new;
sem_t mutex_cola_ready;
sem_t mutex_cola_new;
sem_t mutex_cola_exec;
sem_t sem_multi;
sem_t semCPUs_disponibles;
sem_t mutex_lista_CPUs;
sem_t mutex_dinamico;
sem_t mutex_datos;
sem_t mutex_fs;

pthread_mutex_t mutex_memoria_fd;

//Listas
t_list* listaCPUs;
t_list* listadoEstadistico;
bool planificacionActivada;
pthread_mutex_t lockPlanificacion;
pthread_cond_t lockCondicionPlanificacion;

//Colas procesos
t_queue *colaNew, *colaReady, *colaFinished, *colaExec, *colaBloqueados;

//Diccionarios
t_dictionary* bloqueos;

//para manejo de archivos
int max_archivo_fd;
t_list* globalFileTable;
t_list* processFileTable;
int getArchivoFdMax(void);
void crearEntradaArchivoProceso(int proceso);
int agregarArchivo_aProceso(int proceso, char* file, char* permisos);
int eliminarFd(int fd, int proceso);
void imprimirTablaGlobal(void);
char* buscarPathDeArchivo(int globalFD);
t_archivo* buscarArchivo(int pid, int fd);
bool existeArchivo(char* path);
bool archivoPuedeSerBorrado(int globalFD);


fd_set master;
fd_set setConsolas;
fd_set setCPUs;
int socketEscuchaCPUs;
int socketEscuchaConsolas;
int max_fd;
int valor[3];
t_list* mem_dinamica;
t_list* bloques;

//Cosas del inotify
void inicializarInotify();
void cambiarConfig();
char* inotify_path;
int inotify_fd;
int create_wd;
int delete_wd;
int modify_wd;

int16_t grabarBytes(int socket, t_pedido_bytes* pedido, void*buf);
int16_t solicitudBytes(int socket, t_pedido_bytes* pedido, char**buf);

#endif /* FUNCIONESKERNEL_H_ */
