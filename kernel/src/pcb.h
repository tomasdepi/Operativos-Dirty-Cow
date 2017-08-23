/*
 * pcb.h
 *
 *  Created on: 12/4/2017
 *      Author: utnso
 */

#ifndef PCB_H_
#define PCB_H_

#include <commons/structUtiles.h>
#include "funcionesKernel.h"
#include <parser/metadata_program.h>
#include <sys/stat.h>
#include <parser/parser.h>

//Prototipos de creacion y manejo de pcb
t_pcb* crearPCB(char* codigo, int id, int fd);
void llenarLista(t_list** lista, t_intructions * indiceCodigo, int cantInstruc);

#endif /* PCB_H_ */
