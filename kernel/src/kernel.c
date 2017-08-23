/*
 ============================================================================
 Name        : kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesKernel.h"

int main(int argc, char** argv){
	logger = log_create("../logKernel","kernel", 0, LOG_LEVEL_TRACE);
	crearConfig(argc,argv);
	inicializaciones();

	levantarInterfaz();

	conectarConServidores();

	lanzarHilosPlanificacion();

	escucharConexiones();

	destruirConfiguracionKernel(config);
	printf("me voy por aca \n");
	return EXIT_SUCCESS;
}

/**
 * @NAME: ConexionConServidores
 * @DESC: Conexiones con Memoria y FS.
 */
void conectarConServidores(void){
	if(conexionConMemoria() == -1){
		log_error(logger,"No se pudo establecer la conexion con la memoria.");
		exit(1);
	}
	if(conexionConFileSystem() == -1){
		log_error(logger,"No se pudo establecer la conexion con el File System.");
		exit(1);
	}
	printf("Conexion exitosa con FileSystem y Memoria\n");
}

/**
 * @NAME: EscucharConexiones
 * @DESC: Escucho conexiones CPU y ConsolaPrograma.
 */
void escucharConexiones(void){
	FD_ZERO(&master);
	FD_ZERO(&setCPUs);
	FD_ZERO(&setConsolas);

	socketEscuchaCPUs = createServer2(config->ip_kernel,config->puerto_CPU,BACKLOG);
	socketEscuchaConsolas = createServer2(config->ip_kernel,config->puerto_PROG,BACKLOG);

	max_fd = max(socketEscuchaCPUs, socketEscuchaConsolas);

	//Agrego fd de inotify
	FD_SET(inotify_fd, &master);

	FD_SET(socketEscuchaCPUs, &master);
	FD_SET(socketEscuchaConsolas, &master);
	fd_set read_fd;
	int iterador_sockets, resultadoHilo;

	while(1){
		read_fd = master;

		if(select(max_fd + 1, &read_fd, NULL, NULL, NULL) == -1){
			perror("select");
			exit(1);
		}

		if(FD_ISSET(socketEscuchaCPUs, &read_fd)){ //una cpu quiere conectarses
			aceptarNuevaConexion(socketEscuchaCPUs, &setCPUs);
		}

		if(FD_ISSET(socketEscuchaConsolas, &read_fd)){ //una consola quiere conectarse
			aceptarNuevaConexion(socketEscuchaConsolas, &setConsolas);
		}

		for(iterador_sockets = 0; iterador_sockets <= max_fd; iterador_sockets++) {

			if(iterador_sockets == inotify_fd && FD_ISSET(iterador_sockets,&read_fd))
				cambiarConfig();

			if(FD_ISSET(iterador_sockets, &setCPUs) && FD_ISSET(iterador_sockets,&read_fd)){ //una cpu realiza una operacion
				FD_CLR(iterador_sockets, &setCPUs);
				pthread_t hilo;
				resultadoHilo = pthread_create(&hilo, NULL, (void*)trabajarMensajeCPU, iterador_sockets);
				if(resultadoHilo) exit(1);
			}

			if(FD_ISSET(iterador_sockets, &setConsolas) && FD_ISSET(iterador_sockets,&read_fd)){ //una consola realiza una operacion
				FD_CLR(iterador_sockets, &setConsolas);
				pthread_t hilo;
				resultadoHilo = pthread_create(&hilo, NULL, (void*)trabajarMensajeConsola, iterador_sockets);
				if(resultadoHilo) exit(1);
			}
		}
	}
}

void aceptarNuevaConexion(int socketEscucha, fd_set* set){

	int newSocket = acceptSocket(socketEscucha);

	if(newSocket == -1) {
		perror("Error al aceptar");
	} else {
		FD_SET(newSocket, &master);
		FD_SET(newSocket, set);
		if(newSocket > max_fd) max_fd = newSocket;
	}
}

/**
 * @NAME: Inicializaciones
 * @DESC: Inicializo las esctructuras de control de procesos/cpus/etc.
 *		  (Listas y colas.)
 */
void inicializaciones(void){
	sem_init(&sem_cola_ready,0,0);
	sem_init(&sem_cola_new,0,0);
	sem_init(&sem_multi,0,config->grado_MultiProg);
	sem_init(&semCPUs_disponibles, 0, 0);
	sem_init(&mutex_cola_ready,0,1);
	sem_init(&mutex_cola_new,0,1);
	sem_init(&mutex_cola_exec,0,1);
	sem_init(&mutex_lista_CPUs,0,1);
	sem_init(&mutex_dinamico,0,1);
	sem_init(&mutex_datos,0,1);
	sem_init(&mutex_fs,0,1);
	pthread_mutex_init(&mutex_memoria_fd,NULL);
	inicializarColas();
	listaCPUs = list_create();
	listadoEstadistico = list_create();
	cantProcesosSistema = 0;
	planificacionActivada = true;
	pthread_cond_init(&lockCondicionPlanificacion, NULL);
	pthread_mutex_init(&lockPlanificacion, NULL);
	mem_dinamica = list_create();
	bloques = list_create();
	processFileTable = list_create();
	globalFileTable = list_create();
	max_archivo_fd = 3; // 0 1 2 estan reservados
}

/**
 * @NAME: Planificador corto plazo.
 * @DESC: PCP para administrar las CPUs nuevas y planificar.
 */
void lanzarHilosPlanificacion(void){

	int resultado;

	//Se Crear un hilo de PCP que se encargara de planificar los procesos cuando se conecte una CPU nueva.
	resultado = pthread_create(&hiloPCP, NULL, (void*)planificarCortoPlazo, NULL);
	if(resultado){
		printf("El hilo de PCP no pudo crearse\n");
		log_error(logger, "El hilo de PCP no pudo crearse");
		exit(1);
	}
}

