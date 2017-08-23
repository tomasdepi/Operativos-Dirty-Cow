/*
 * memoria.h
 *
 *  Created on: 5/4/2017
 *      Author: utnso
 */

#ifndef FUNCIONESMEMORIA_H_
#define FUNCIONESMEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <commons/config.h>
#include <commons/sockets.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/interface.h>
#include <commons/cosas.h>
#include <commons/structUtiles.h>
#include <pthread.h>
#include "peticiones.h"

#define configuracionMemoria "../confMemoria.init"
#define MAX_LEN_PUERTO 6
#define IP "192.168.3.3"
#define BACKLOG 10
#define MAX_LEN_IP 20
//Defines para escribir menos
#define cant_frames config->marcos
#define frame_size config->marcos_Size
#define cache_entradas config->entradas_Cache
#define max_entradas config->cache_x_Proceso

#define tabla_pag ((t_entrada_tabla*)memoria)

//Define temporal
#define stack_size 10

typedef struct{
        char* puerto;
        char* ip_memoria;
        int marcos;
        int marcos_Size;
        int entradas_Cache;
        int cache_x_Proceso;
        char* reemplazo_cache;
        int retardo_Memoria;
        bool retardoActivado;
        bool imprimirRetardo;
}t_config_memoria;

typedef struct{
	int pid;
	int pag;
}t_entrada_tabla;

typedef struct{
	int pid;
	int pag;
	char* content;
	unsigned long int time_used; //Cual fue la ultima vez que se utilizo
}t_entrada_cache;

void testHash();
void inicializarGlobales();
void esperar();

t_config_memoria* levantarConfiguracionMemoria(char* archivo);
void crearConfig(int argc, char* argv[]);
void destruirConfiguracionMemoria(t_config_memoria* config);

//Funciones de conexionado
void esperarConexiones();
void esperarConexionKernel();

/* Esta funcion hace la creacion de la memoria y todas las estructuras
 * administrativas necesarias para que el sistema arranque
 */
void inicializarMemoria();

//Funciones administracion memoria
int framesLibres();
int buscarFrame(int pid, int pag);
int reservarFrames(int pid, int cantPag);
int liberarFrame(int pid, int nroPag);
int escribir(int pid, int pag, int offset, char* contenido, int size); //Devuelve codigos error
int leer(int pid, int pag, int offset, int size, char* resultado); //Devuelve codigos error

bool pedidoIncorrecto(t_pedido_memoria*);

//Funciones cache
void increaseOpCount(); //Suma uno al opCount
/* Cuantas entradas tiene el pid */
int cantEntradas(int pid);
/* Busca la entrada con pid y pag. Si no existe retorna -1*/
int buscarEntrada(int pid, int pag);
/* Esta funcion aplica el LRU y me dice que entrada debo reemplazar
 * en caso de que esten todas ocupadas. Necesita el pid para no pasarse
 * del límite de entradas por proceso*/
int entradaAReemplazar(int pid);
int reemplazoLocal(int pid);
int reemplazoGlobal();
/* Busca la entrada que coincida con pid y pag, y devuelve el puntero contenido de la entrada
 * Devuelve 0 en caso de que exista la entrada, -1 en caso contrario*/
int leerCache(int pid, int pag, char** contenido);
/* Se llena una entrada de la cache con los valores pasados por parametro.
 * Si ya existe la entrada, se usa esa misma. Sino, reemplaza una usando LRU.
 * El puntero frame apunta al comienzo del frame referenciado por pid y pag*/
void actualizarEntradaCache(int pid, int pag, char* frame);

void requestHandlerKernel(int fd);
void requestHandlerCpu(int fd);
void enviarTamanioPagina(int fd);

//Pedidos de Kernel
int iniciarPrograma(int fd, t_pedido_iniciar* pedido);
int finalizarPrograma(t_pedido_finalizar *pid);
int asignarPaginas(int fd, t_pedido_asignar* pedido);
int liberarPagina(int fd, t_pedido_liberar* pedido);

//Pedidos cpu
int solicitudBytes(int fd, t_pedido_memoria* pedido);
int grabarBytes(int fd, char* paquete);

//Respuestas status
void enviarRespuesta(int fd, int respuesta);

//Funciones de interfaz
void levantarInterfaz();
void retardo(char* comando, char* param);
void dump(char* comando, char* param);
void dumpAll();
void dumpCache();
void dumpTable();
void dumpMemory(int pid);
void flush(char* comando, char* param);
void size(char* comando, char* param);
void help(char* comando, char* param);

char* getTimeStamp();

//Variables Globales
t_log* logger;
t_config_memoria* config;
int socketEscuchaConexiones;
int socketConexionKernel;
int socketConexionCpu;
bool hayCache;

char* memoria; /*Este va a ser el bloque que simula la memoria principal.
				Uso char* porque sizeof(char) = 1 y facilita la aritmetica,
				pero no tiene nada que ver con caracteres*/
t_entrada_cache* cache;

unsigned long int op_count; /*Esto vendría a ser nuestro tiempo de referencia para el algoritmo LRU.
 	 	 	 			 Cada vez que se realiza una operación en memoria, se incrementa.*/

//Mutexes
pthread_mutex_t cache_mutex;
pthread_mutex_t tablaPag_mutex;

#endif /* FUNCIONESMEMORIA_H_ */
