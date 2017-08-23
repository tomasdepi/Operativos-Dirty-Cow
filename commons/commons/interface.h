/*
 * interface.h
 *
 *  Created on: 5/4/2017
 *      Author: utnso
 */

#ifndef COMMONS_INTERFACE_H_
#define COMMONS_INTERFACE_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define command_size 50
#define buffer_size command_size * 2

typedef struct{
	char comando[command_size];
	void (*funcion)(char* , char* );
}comando;

typedef struct{
	comando* comandos;
	int cantComandos;
}interface_thread_param;

void interface(interface_thread_param* param);

#endif /* COMMONS_INTERFACE_H_ */
