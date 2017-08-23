/*
 * sockets.h
 *
 *  Created on: 4/4/2017
 *      Author: utnso
 */

#ifndef COMMONS_SOCKETS_H_
#define COMMONS_SOCKETS_H_

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include "collections/list.h"
#include "structUtiles.h"
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>

typedef struct {
	int8_t type;
	int32_t length;
}__attribute__((__packed__)) header_t;

enum enum_protocolo{// Si yo soy el kernel tengo que enviar handshake_kernel.
	PEDIDO_INFO_CONEXION = 1,
	RESPUESTA_PEDIDO_INFO_CONEXION = 2,
	HANDSHAKE_CPU = 3,
	HANDSHAKE_KERNEL = 4,
	HANDSHAKE_MEMORIA = 5,
	HANDSHAKE_FS = 6,
	OK = 7
};

//Mensajes que el kernel le envia al CPU
enum protocolo_kernel_a_cpu{

	EXEC_PCB = 10,
	EXEC_QUANTUM = 11,
	VALOR_VAR_COMPARTIDA = 12,
	WAIT_SEGUIR_EJECUCION = 13,
	WAIT_DETENER_EJECUCION = 14,
	SIGNAL_OK = 15,
	ASIG_VAR_COMPARTIDA_OK = 16,
	IMPRIMIR_TEXTO_OK = 17,
	IMPRIMIR_VARIABLE_OK = 18,
	RESERVAR_MEMORIA_OK = 19,
	LIBERAR_MEMORIA_OK = 20,
	ABRIR_ARCHIVO_OK = 21,
	BORRAR_ARCHIVO_OK = 22,
	CERRAR_ARCHIVO_OK = 23,
	ESCRIBIR_ARCHIVO_OK = 24,
	LEER_ARCHIVO_OK = 25,
	TAMANIO_STACK_PARA_CPU = 26,
	TAMANIO_PAGINAS_NUCLEO = 27,
	QUANTUM_SLEEP = 28,
	MEMORY_CORRUPTION = 29,
	ASIGNACION_OK = 30
};

//Mensajes que el CPU le envia al kernel
enum protocolo_cpu_a_kernel{
	LEER_VAR_COMPARTIDA = 33,
	ASIG_VAR_COMPARTIDA = 34,
	ESCRIBIR = 35,
	SEM_WAIT = 36,
	FIN_EJECUCION = 37,
	FIN_PROCESO = 38,
	SEM_SIGNAL = 39,
	RESERVAR_MEMORIA = 40,
	LIBERAR_MEMORIA = 41,
	ABRIR_ARCHIVO = 42,
	CERRAR_ARCHIVO = 43,
	BORRAR_ARCHIVO = 44,
	LEER_ARCHIVO = 45,
	ENVIO_PCB = 46,
	STACKOVERFLOW = 47,
	PROC_BLOCKED = 48,
	ERROR_ARCHIVO = 49,
	MOVER_CURSOR = 50,
	VERIFICAR_ASIGNAR = 51
};

//Mensajes que se le envian a la memoria.
enum protocolo_a_memoria{
	LEER_VAR = 57,
	LIBERAR_PAGINA = 58,
	ASIGNAR_PAGINAS = 59,
	SOLICITUD_BYTES = 60,
	GRABAR_BYTES = 61,
	CAMBIO_PROCESO_ACTIVO = 62,
	CREAR_SEGMENTO = 63,
	DESTRUIR_SEGMENTOS = 64,
	INICIAR_PROGRAMA = 65,
};

//Mensajes que la memoria le envia el resto de los procesos.
enum protocolo_memoria_a_cualquiera{
	OVERFLOW = 70,
	OP_OK = 71,
	QUILOMBO = 72,
	SIN_ESPACIO = 73,
	ENVIAR_TAMANIO_PAGINA = 74,
	RESPUESTA_BYTES = 75,
	MEMORY_OVERLOAD = 76,
	SEGMENTO_CREADO = 77,
};

//Mensajes entre Kernel y Programa
enum protocolo_kernel_programa{
	PID_PROGRAMA = 82,
	IMPRIMIR_VARIABLE_PROGRAMA = 83,
	IMPRIMIR_POR_PANTALLA = 84,
	ERROR_GENERAL = 85,
	SEGMENTATION_FAULT_PROGRAMA = 86,
	MEMORY_OVERLOAD_PROGRAMA = 87,
	FINALIZAR_EJECUCION = 88,
	FINALIZAR_EJECUCION_ERROR = 89,
	PROCESO_RECHAZADO = 90
};

enum protocolo_programa_a_kernel{
	CODIGO_PROGRAMA = 95,
	HANDSHAKE_PROGRAMA = 96,
	ENVIO_CODIGO = 97,
	FINALIZAR_PROGRAMA = 98
};

enum protocolo_kernel_a_fs{
	CREAR_ARCHIVO = 103,
	OBTENER_DATOS = 104,
	GUARDAR_DATOS = 105,
	VALIDAR_ARCHIVO = 106
	//para borrar archivo uso el que esta en cpu
};

enum protocolo_fs_a_kernel{
 	ARCHIVO_EXISTE = 110,
 	LECTURA_OK = 111,
 	ESCRITURA_OK = 112,
 	BORRADO_OK = 113,
 	MOVER_CURSOR_OK = 114,
 	ERROR = 115
 };

enum exit_code{
	FINALIZO_BIEN = 0,
	FALLA_RESERVAR_RECURSOS = -1,
	ARCHIVO_INEXISTENTE = -2,
	LEER_ARCHIVO_SIN_PERMISOS = -3,
	ESCRIBIR_ARCHIVO_SIN_PERMISOS = -4,
	ERROR_MEMORIA = -5,
	DESCONEXION_CONSOLA = -6,
	FINALIZAR_DESDE_CONSOLA = -7,
	SUPERO_TAMANIO_PAGINA = -8,
	SUPERA_LIMITE_ASIGNACION_PAGINAS = -9,
	SEMAFORO_NO_EXISTE = -10,
	GLOBAL_NO_DEFINIDA = -11,
	NULL_POINTER = -12,
	DESCONEXION_CPU = -13,
	ERROR_SIN_DEFINICION = -20,
	SEGMENTATION_FAULT = -14,
	SIN_ESPACIO_FS = -15,
	IMPOSIBLE_BORRAR_ARCHIVO = -16
};

//
// Funciones básicas para manejo de sockets.
//

int getSocket(void);
int bindSocket(int, char *, char *);
int listenSocket(int, int);
int acceptSocket(int);
int sendSocket(int socket, header_t* cabecera, void* data);
int connectSocket(int, char *, char *);
int sendallSocket(int s, void* buf, int len);
int createServer(char *, char *, int);
int createClient(char *, char *);
int recibir_paquete(int socket, void** paquete, int* tipo);
int recibir_string(int socket,void** puntero_buffer,int* tipo);
int agregar_caracter_nulo(void* stream, int tamanio);
header_t crear_cabecera(int codigo, int length);
int recvMsj(int socket, void** paquete, header_t*header);
int enviar_info(int sockfd, int codigo_operacion, int length, void* buff);
int enviar_paquete_vacio(int codigo_operacion, int socket);
int finalizarConexion(int socket);

//
// Serializadores y Deserializadores de mensajes.
//

//************* NUEVOS **************
t_list* deserializarIndiceStack(char* buffer);
t_buffer_tamanio* serializarIndiceStack(t_list* indiceStack);

t_list* deserializarIndiceCodigo(char* indice, uint32_t tam);
t_buffer_tamanio* serializarIndiceStack(t_list* indiceStack);

t_pcb* deserializar_pcb(char* package);
t_buffer_tamanio* serializar_pcb(t_pcb* pcb);
//***********************************

int sendAll(int fd, char *cosa, int size, int flags);
int recvAll(int fd, char *buffer, int size, int flags);

#endif /* COMMONS_SOCKETS_H_ */
