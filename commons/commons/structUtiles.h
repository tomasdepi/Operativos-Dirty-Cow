/*
 * StructsUtiles.h
 *
 *  Created on: 11/4/2017
 *      Author: utnso
 */

#ifndef STRUCTSUTILES_H_
#define STRUCTSUTILES_H_

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "collections/list.h"
#include "collections/queue.h"
#include "config.h"
#include "log.h"
#include "collections/dictionary.h"

typedef struct{
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}__attribute__((__packed__)) t_argumento;

typedef struct{
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}__attribute__((__packed__)) t_posicion;

typedef struct{
	uint32_t pid;
	uint32_t pag;
	uint32_t offset;
	uint32_t size;
}__attribute__((__packed__)) t_pedido_bytes;

typedef struct{
	char id;
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
}__attribute__((__packed__)) t_var;

typedef struct{
	t_list* argumentos;
	t_list* variables;
	int32_t direcretorno;
	t_posicion* retVar;
}__attribute__((__packed__)) t_entrada_stack;

//t_dictionary* etiquetas;

typedef struct indiceCodigo{
	uint32_t offset;
	uint32_t size;
}__attribute__((__packed__)) t_indice_codigo;

typedef struct{
	uint32_t pid;  //Identificador único del Programa en el sistema
	uint32_t programCounter; //Número de la próxima instrucción a ejecutar
	uint32_t cantPaginasCodigo;
	t_list* indiceCodigo;
	t_list* indiceStack;
	int32_t exitCode;
	uint32_t consolaFd;
	char* etiquetas;
	uint32_t stackPointer; // el ultimo offset
	uint32_t tamanioEtiquetas;
	uint32_t codigo; // cant de instrucciones ----- podria sacarse, ya qye es indiceCodigo->elements_count
}__attribute__((__packed__)) t_pcb;

//Auxiliares
typedef struct {
	uint32_t tamanioBuffer;
	char *buffer;
}__attribute__((__packed__)) t_buffer_tamanio;

typedef struct {
	uint32_t tamanioStack;
	void * stack;
}__attribute__((__packed__)) t_tamanio_stack;

typedef struct{
	uint32_t pid;
	uint32_t pedido;
}__attribute__((__packed__))t_reserva;

typedef struct{
	int32_t descriptor;
	uint32_t cursor;
}__attribute__((__packed__))t_descriptor;

typedef struct{
	uint32_t pid;
	int32_t descriptor;
	uint32_t informacion;
	uint32_t size;
}__attribute__((__packed__))t_lectura;

typedef struct{
	uint16_t pid;
	int32_t descriptor;
	size_t size;
}__attribute__((__packed__))t_escritura;

typedef struct{
	uint16_t pid;
	int32_t descriptor;
	size_t posicion;
}__attribute__((__packed__))t_cursor;

typedef struct {
	int pid;
	char* info;
}__attribute__((__packed__)) t_imprimir;

void freePCB(t_pcb* pcb);
t_entrada_stack* crearPosicionStack(void);
void insertarNuevoStack(t_pcb* pcb);
void eliminarUltimaPosicionStack(t_pcb* pcb);
void destruirPosicionStack(t_entrada_stack* stack);
t_var* crearVariableStack(char id, uint32_t pagina, uint32_t offset, uint32_t size);
void destruirVariableStack(t_var* var);
t_argumento* crearArgumentoStack(uint32_t pagina, uint32_t offset, uint32_t size);
void destruirArgumentoStack(t_argumento* arg);
void agregarVariable(t_entrada_stack* stack, t_var* variable);
void agregarArgumento(t_entrada_stack* stack, t_argumento* argumento);
char* ansisop_a_string(char* path);

#endif /* STRUCTSUTILES_H_ */
