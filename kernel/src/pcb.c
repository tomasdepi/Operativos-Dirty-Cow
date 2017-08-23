#include "pcb.h"

/*
 * Crea la estructura PCB y lo introduce en la cola de New
 * Envia a la memoria para que le reserve espacio para el codigo
 * Graba el codigo en Memoria y devuelve el PCB con todas las referencias
 */
t_pcb* crearPCB(char* codigo, int pid, int fd){

	t_pcb* pcb=malloc(sizeof(t_pcb));
	t_list * lista=list_create();
	t_metadata_program* metadata=metadata_desde_literal(codigo);
	llenarLista(&lista,metadata->instrucciones_serializado, metadata->instrucciones_size);
	pcb->indiceCodigo=lista;

	pcb->pid = pid;
	pcb->stackPointer = 0;
	pcb->cantPaginasCodigo = (strlen(codigo)) / pagina_size;
	if(strlen(codigo)%pagina_size != 0) pcb->cantPaginasCodigo++;
	pcb->exitCode = 0;
	pcb->programCounter = metadata->instruccion_inicio;
	pcb->codigo = metadata->instrucciones_size;
	pcb->tamanioEtiquetas = metadata->etiquetas_size;
	pcb->consolaFd=fd;

	if(metadata->etiquetas_size){
		pcb->etiquetas = malloc(metadata->etiquetas_size);
		memcpy(pcb->etiquetas, metadata->etiquetas, metadata->etiquetas_size);
	}else{
		pcb->etiquetas = NULL;
	}
	pcb->indiceStack=list_create();

/*
	//Reservo el espacio para el codigo y almaceno el codigo
	u_int32_t direccion_segmento = guardarEnMemoria(socketConexionMemoria,strlen(codigo),pcb->pid);
	pcb->indice_codigo = direccion_segmento;

	if(direccion_segmento == 0){
		pcb->pid = 0; // Hubo un error al querer reservar espacio de memoria
		return pcb;
	}
	//Reservo el espacio para el Stack del programa.
	u_int32_t direccion_stack = reservarMemoria(socketConexionMemoria,strlen(config->stack_Size),pcb->pid);
	pcb->stackPointer = direccion_stack;

	if(direccion_stack == 0){
		pcb->pid = 0;
		return pcb;
	}
*/
	metadata_destruir(metadata);
	return pcb;
}

void llenarLista(t_list** lista, t_intructions * indiceCodigo, int cantInstruc){
	int b = 0;
	for (b = 0; b < cantInstruc; b++) {
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		linea->offset = indiceCodigo[b].start;
		linea->size = indiceCodigo[b].offset;
		list_add(*lista, linea);
	}
	return;
}
