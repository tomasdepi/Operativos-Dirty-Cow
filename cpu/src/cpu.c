/*
 ============================================================================
 Name        : cpu.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "funcionesCpu.h"

int main(int argc, char** argv) {

	crearLog();
	crearConfig(argc,argv);
	// Conecta y obtiene tamanio de pagina (memoria) y de stack (kernel).
	if(conexionConKernel() == -1 || conexionConMemoria() == -1){
		finalizarCPU();
	}

	/*
	 * Manejo de la interrupcion SigusR1
	 */
	signal(SIGUSR1, revisarSigusR1);
	for(;;){
		requestHandlerKernel();
	}

	return EXIT_SUCCESS;
}
