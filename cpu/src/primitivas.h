/*
 * primitivas.h
 *
 *  Created on: 17/4/2017
 *      Author: utnso
 */

#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

#include <parser/parser.h>
#include <ctype.h>
#include <commons/structUtiles.h>
#include "funcionesCpu.h"
#define TAMANIO_VARIABLE 4

extern bool huboStackOver;
extern bool finPrograma;
t_pcb* pcb;

void setPCB(t_pcb* pcb);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_puntero definirVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void finalizar(void);
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
void retornar(t_valor_variable retorno);
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo direccion);
void cerrar(t_descriptor_archivo descriptor_archivo);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);
void liberarMemoria(t_puntero puntero);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
t_puntero reservar(t_valor_variable espacio);
void signalAnsisop(t_nombre_semaforo identificador_semaforo);
void wait(t_nombre_semaforo identificador_semaforo);
bool esArgumento(t_nombre_variable identificador_variable);

#endif /* PRIMITIVAS_H_ */
