/*
 * peticiones.h
 *
 *  Created on: 19/4/2017
 *      Author: utnso
 */

#ifndef PETICIONES_H_
#define PETICIONES_H_

typedef struct{
	uint32_t pid;
	uint32_t pag;
	uint32_t offset;
	uint32_t size;
}__attribute__((__packed__))t_pedido_memoria;

typedef struct{
	uint32_t pid;
	uint32_t cant_pag;
}__attribute__((__packed__))t_pedido_iniciar;

typedef struct{
	uint32_t pid;
	uint32_t cant_pag;
}__attribute__((__packed__))t_pedido_asignar;

typedef struct{
	uint32_t pid;
	uint32_t nroPag;
}__attribute__((__packed__))t_pedido_liberar;

typedef uint32_t t_pedido_finalizar;

#endif /* PETICIONES_H_ */
