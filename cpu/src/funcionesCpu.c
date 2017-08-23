#include "funcionesCpu.h"

bool cerrarCPU = false;
bool huboStackOver = false;
bool finPrograma = false;
bool finPorError = false;
int finErrorExitCode = 0;
bool estaEjecutando = false;

AnSISOP_funciones functions = { .AnSISOP_asignar = asignar,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_definirVariable = definirVariable, .AnSISOP_dereferenciar =
				dereferenciar, .AnSISOP_finalizar = finalizar,
		.AnSISOP_irAlLabel = irAlLabel, .AnSISOP_llamarConRetorno = llamarConRetorno,
		.AnSISOP_llamarSinRetorno=llamarSinRetorno,.AnSISOP_obtenerPosicionVariable =
				obtenerPosicionVariable, .AnSISOP_obtenerValorCompartida =
				obtenerValorCompartida, .AnSISOP_retornar = retornar,
		};

AnSISOP_kernel kernel_functions = { .AnSISOP_abrir =abrir, .AnSISOP_borrar = borrar,
		.AnSISOP_cerrar = cerrar, .AnSISOP_escribir = escribir, .AnSISOP_leer = leer,
		.AnSISOP_liberar=liberarMemoria, .AnSISOP_moverCursor = moverCursor,
		.AnSISOP_reservar=reservar, .AnSISOP_signal = signalAnsisop,
		.AnSISOP_wait = wait
};

int crearLog(void){
	logger = log_create("../logCpu","cpu", 0, LOG_LEVEL_TRACE);
	if(logger)
		return 1;
	else
		return 0;
}

void crearConfig(int argc, char* argv[]){
	if(argc>1){
		if(verificarExistenciaDeArchivo(argv[1])){
			config=levantarConfiguracionCPU(argv[1]);
			log_info(logger, "Configuracion levantada correctamente");
		}else{
			log_error(logger,"Ruta incorrecta");
			exit(EXIT_FAILURE);
		}
	}
	else if(verificarExistenciaDeArchivo(configuracionCPU)){
		config=levantarConfiguracionCPU(configuracionCPU);
		log_info(logger,"Configuracion levantada correctamente");
	}
	else if(verificarExistenciaDeArchivo(string_substring_from(configuracionCPU,3))){
		config=levantarConfiguracionCPU(string_substring_from(configuracionCPU,3));
		log_info(logger,"Configuracion levantada correctamente");
	}
	else{
		log_error(logger,"No se pudo levantar el archivo de configuracion");
		exit(EXIT_FAILURE);
	}
}

t_config_cpu* levantarConfiguracionCPU(char* archivo) {
        t_config_cpu* conf = malloc(sizeof(t_config_cpu));
        t_config* configCPU;
        configCPU = config_create(archivo);
        conf->puerto_Kernel = malloc(strlen(config_get_string_value(configCPU, "PUERTO_KERNEL"))+1);
        strcpy(conf->puerto_Kernel, config_get_string_value(configCPU, "PUERTO_KERNEL"));
        conf->puerto_Memoria = malloc(strlen(config_get_string_value(configCPU, "PUERTO_MEMORIA"))+1);
        strcpy(conf->puerto_Memoria, config_get_string_value(configCPU, "PUERTO_MEMORIA"));
        conf->ip_Memoria = malloc(strlen(config_get_string_value(configCPU, "IP_MEMORIA"))+1);
        strcpy(conf->ip_Memoria, config_get_string_value(configCPU, "IP_MEMORIA"));
        conf->ip_Kernel = malloc(strlen(config_get_string_value(configCPU, "IP_KERNEL"))+1);
        strcpy(conf->ip_Kernel, config_get_string_value(configCPU, "IP_KERNEL"));
        config_destroy(configCPU);
        printf("Configuracion levantada exitosamente\n");
        return conf;
}

void freeConf(t_config_cpu* config){
	free(config->ip_Kernel);
	free(config->ip_Memoria);
	free(config->puerto_Kernel);
	free(config->puerto_Memoria);
	free(config);
}

int conexionConKernel(void){
	int rta;
	int operacion;
	void* paquete_vacio;
	socketConexionKernel=createClient(config->ip_Kernel,config->puerto_Kernel);
	if(socketConexionKernel == -1){
		log_error(logger,"No pudo conectarse a Kernel");
		return -1;
	}else{
		log_info(logger,"Cliente a Kernel creado");
	}
	enviar_paquete_vacio(HANDSHAKE_CPU, socketConexionKernel);
	recibir_paquete(socketConexionKernel, &paquete_vacio, &operacion);
	if(operacion==HANDSHAKE_KERNEL){
		log_info(logger,"Conexion establecida con Kernel! :D");
	}else{
		log_info(logger,"El Kernel no devolvio handshake :(");
		return -1;
	}
	log_debug(logger, "Esperando tamaño del stack...");
	rta = requestHandlerKernel();
	if(rta == -1){
		log_error(logger,"Error al recibir tamanio de stack");
		return -1;
	}
	rta = requestHandlerKernel();
	if(rta == -1){
		log_error(logger,"Error al recibir el quantum sleep");
		return -1;
	}
	printf("Conexion con Kernel exitosa\n");
	return EXIT_SUCCESS;
}

int conexionConMemoria(void){
	int rta;
	int operacion;
	void* paquete_vacio;
	socketConexionMemoria = createClient(config->ip_Memoria, config->puerto_Memoria);
	if(socketConexionMemoria == -1){
		log_error(logger,"No pudo conectarse a Memoria");
		return -1;
	}else{
		log_info(logger,"Cliente a Memoria creado");
	}
	enviar_paquete_vacio(HANDSHAKE_CPU, socketConexionMemoria);
	recibir_paquete(socketConexionMemoria, &paquete_vacio, &operacion);

	if(operacion == HANDSHAKE_MEMORIA){
		log_info(logger,"Conexion establecida con Memoria! :D");
	}else{
		log_info(logger,"La Memoria no devolvio handshake :(");
		return -1;
	}
	log_debug(logger, "Esperando tamaño de pagina...");
	rta = requestHandlerMemoria();
	if(rta != 0){
		log_error(logger, "Error al recibir tamanio de pagina");
		return -1;
	}
	printf("Conexion exitosa con Memoria\n");
	return EXIT_SUCCESS;
}

int32_t requestHandlerKernel(void){
	header_t header;
	void* paquete;
	paquete=NULL;
	conecFailKernel(recvMsj(socketConexionKernel,&paquete,&header));
	switch(header.type){
		case EXEC_PCB:
			recibirPCB(paquete);
			break;
		case EXEC_QUANTUM:
			comenzarEjecucionDePrograma(paquete);
			break;
		case TAMANIO_STACK_PARA_CPU:
			tamanioStack=*(uint32_t*)paquete;
			log_info(logger, "Tamanio stack: %d", tamanioStack);
			break;
		case QUANTUM_SLEEP:
			quantumSleep = *(uint32_t*) paquete;
			break;
//		RESPUESTAS PRIMITIVAS KERNEL:
		case WAIT_SEGUIR_EJECUCION:
			log_debug(logger,"Proceso NO queda bloqueado");
			break;
		case WAIT_DETENER_EJECUCION:
			log_debug(logger,"Proceso queda bloqueado");
			procesoBloqueado = true;
			break;
		case VALOR_VAR_COMPARTIDA:
			paqueteGlobal=malloc(header.length);
			memcpy(paqueteGlobal,paquete,header.length);
			break;
		case ASIG_VAR_COMPARTIDA_OK:
			log_info(logger, "Se asigno correctamente la variable compartida");
			break;
		case RESERVAR_MEMORIA_OK:
			paqueteGlobal=malloc(header.length);
			memcpy(paqueteGlobal,paquete,header.length);
			break;
		case ABRIR_ARCHIVO_OK:
			paqueteGlobal=malloc(header.length); // guardo el fd
			memcpy(paqueteGlobal,paquete,header.length);
			break;
		case SIGNAL_OK:
		case ESCRITURA_OK:
		case LECTURA_OK:
		case LEER_ARCHIVO_OK:
		case BORRAR_ARCHIVO_OK:
		case CERRAR_ARCHIVO_OK:
		case MOVER_CURSOR_OK:
		case LIBERAR_MEMORIA_OK:
		case ASIGNACION_OK:
			break;
		// errores
		case SEMAFORO_NO_EXISTE:
		case GLOBAL_NO_DEFINIDA:
		case NULL_POINTER:
		case ARCHIVO_INEXISTENTE:
		case SIN_ESPACIO_FS:
		case FALLA_RESERVAR_RECURSOS:
		case LEER_ARCHIVO_SIN_PERMISOS:
		case ESCRIBIR_ARCHIVO_SIN_PERMISOS:
		case ERROR_MEMORIA:
		case FINALIZAR_DESDE_CONSOLA:
		case SUPERO_TAMANIO_PAGINA:
		case SUPERA_LIMITE_ASIGNACION_PAGINAS:
		case IMPOSIBLE_BORRAR_ARCHIVO:
		case MEMORY_CORRUPTION:
			finErrorExitCode = header.type;
			finPorError = true;
			if(paquete) free(paquete);
			return -1;
		default:
			log_warning(logger, "Mensaje recibido incorrecto %d", header.type);
			if(paquete)free(paquete);
			return -1;
		}
	if(paquete)free(paquete);
	return EXIT_SUCCESS;
}

void conecFailKernel(int cant){
	if(cant <= 0){
		log_error(logger, "Kernel caido... Terminando...");
		close(socketConexionKernel);
		finalizarCPU();
		exit(EXIT_FAILURE);
	}
}

void conecFailMemoria(int cant){
	if(cant <= 0){
		log_error(logger, "Memoria se vino abajo... Terminando...");
		close(socketConexionMemoria);
		finalizarCPU();
		exit(EXIT_FAILURE);
	}
}

int32_t requestHandlerMemoria(void){
	header_t header;
	void* paquete;
	paquete=NULL;
	conecFailMemoria(recvMsj(socketConexionMemoria,&paquete,&header));
	switch(header.type){
		// respuesta grabarBytes - asignarCompartida
	case OP_OK:
		// aca no quiero hacer un free del paquete porque lo voy a usar. hacer free en la funcion que pide el paquete
		break;
	case RESPUESTA_BYTES:
		paqueteGlobal = malloc(header.length);
		memcpy(paqueteGlobal,paquete,header.length);
		break;
	case ENVIAR_TAMANIO_PAGINA:
		tamanioPagina=*(uint32_t*)paquete;
		log_info(logger,"Tamaño de pagina: %d",tamanioPagina);
		break;
	case SEGMENTATION_FAULT:
		log_error(logger,"Segmentation Fault");
		finErrorExitCode = header.type;
		finPorError = true;
		if(paquete) free(paquete);
		return -1;
	case STACKOVERFLOW:
		finErrorExitCode = header.type;
		finPorError = true;
		if(paquete) free(paquete);
		return -1;
	default:
		log_error(logger, "Mensaje Recibido Incorrecto");
		if(paquete)free(paquete);
		return -1;
	}
	if(paquete)free(paquete);
	return EXIT_SUCCESS;
}

void recibirPCB(void* paquete){
	pcb = deserializar_pcb(paquete);
	setPCB(pcb);
	log_info(logger, "PCB #%d recibido", pcb->pid);
	printf("Ejecutar proceso #%d\n", pcb->pid);
}

int16_t solicitarBytes(t_pedido_bytes* pedido){
	header_t header;
	header.type=SOLICITUD_BYTES;
	header.length=sizeof(t_pedido_bytes);
	if(sendSocket(socketConexionMemoria,&header,(void*)pedido) <= 0 ){
		log_error(logger,"Error al enviar. Desconexion...");
		finalizarCPU();
	}
	return requestHandlerMemoria();
}

int16_t almacenarBytes(t_pedido_bytes* pedido, void* paquete){
	char* buffer;
	uint32_t size;
	header_t header;
	size = sizeof(t_pedido_bytes);
	header.type=GRABAR_BYTES;
	header.length=size+pedido->size;
	buffer=malloc(header.length);
	memcpy(buffer,pedido,size);
	memcpy(buffer+size,paquete,pedido->size);
	log_debug(logger, "Pedido escritura a Memoria -> Pid: %d - Pagina: %d - Offset: %d - Size: %d",
			pedido->pid, pedido->pag, pedido->offset, pedido->size);
	if(sendSocket(socketConexionMemoria,&header,(void*)buffer) <= 0 ){
		log_error(logger,"Error al enviar pedido para almacenar bytes en memoria");
		free(buffer);
		finalizarCPU();
	}
	free(buffer);
	if(requestHandlerMemoria() != 0){
		log_error(logger,"No se pudo almacenar en memoria");
		return -1;
	}
	return EXIT_SUCCESS;
}

void revisarSigusR1(int signo){
	if(signo == SIGUSR1){
		printf("Signal SIGUSR1\n");
		log_info(logger, "Se recibe SIGUSR1");
		if(!estaEjecutando) finalizarCPU();
		cerrarCPU = true;
	}
}

void comenzarEjecucionDePrograma(void* paquete){
	estaEjecutando = true;
	quantum = *(uint32_t*)paquete;
	if(quantum == 0){
		log_debug(logger, "Ejecutar - Algoritmo FIFO");
	}else{
		log_debug(logger, "Ejecutar - Algoritmo RR con Q = %d", quantum);
	}
	uint16_t i = 1;
	procesoBloqueado = false;
	while(i <= quantum || quantum == 0){ // Si el quantum es 0 significa que es FIFO ---> ejecuto hasta terminar.
		int16_t sizeInstruccion = solicitarProximaInstruccion(); // carga la instruccion en el paquete global bytes
		if(sizeInstruccion == -1){
			log_error(logger, "No se pudo recibir la instruccion de memoria.");
			finalizarPor(ERROR_MEMORIA);
			return;
		}
		char* instruccion = obtenerInstruccion(paqueteGlobal, sizeInstruccion);
		free(paqueteGlobal);
		log_info(logger, "Instruccion recibida: %s", instruccion);
		printf("Instruccion recibida: '%s'\n", instruccion);
		analizadorLinea(instruccion, &functions, &kernel_functions);
		free(instruccion);

		if(verificarTerminarEjecucion() == -1){
			if(cerrarCPU) finalizarCPU();
			return;
		}

		printf("Instruccion ejecutada\n");
		usleep(quantumSleep * 1000);

		pcb->programCounter++;
		if(procesoBloqueado)break;
		i++;
	}

	if(procesoBloqueado){
		finalizarPor(PROC_BLOCKED);
		log_info(logger, "Finalizo ejecucion por proceso bloqueado");
	}else{
		finalizarPor(FIN_EJECUCION);
		log_info(logger, "Finalizo ejecucion por fin de Quantum");
	}

	if(cerrarCPU) finalizarCPU();

}

int verificarTerminarEjecucion(){
	if(huboStackOver) {
		finalizarPor(SUPERA_LIMITE_ASIGNACION_PAGINAS);
		huboStackOver = false;
		return -1;
	}
	else if(finPorError){
		finalizarPor(finErrorExitCode);
		finPorError = false;
		return -1;
	}
	else if(finPrograma){
		finalizarPor(FIN_PROCESO);
		finPrograma = false;
		return -1;
	}
	else
		return 0;
}

char* obtenerInstruccion(char* paquete, int16_t sizeInstruccion){
	char* instruccion = malloc(sizeInstruccion);
	memcpy(instruccion,paquete,sizeInstruccion);
	int pos_ultimo_caracter = sizeInstruccion - 1;
	char salto_linea = '\n';
	char fin_string = '\0';
	char last_char;
	memcpy(&last_char,instruccion + pos_ultimo_caracter,1);
	if(last_char == salto_linea)
		memcpy(instruccion + pos_ultimo_caracter,&fin_string,1);
	return instruccion;
}

int16_t solicitarProximaInstruccion(void) {
	t_indice_codigo *indice = list_get(pcb->indiceCodigo, pcb->programCounter);
	uint32_t requestStart = indice->offset;
	uint32_t requestSize = indice->size;
	t_pedido_bytes* solicitar = malloc(sizeof(t_pedido_bytes));
	solicitar->pag = requestStart / tamanioPagina;
	solicitar->offset = requestStart % tamanioPagina;
	solicitar->size = requestSize;
	solicitar->pid = pcb->pid;
	log_debug(logger, "Pido instruccion a Memoria -> Pid: %d - Pagina: %d - Offset: %d - Size: %d",
		solicitar->pid, solicitar->pag, solicitar->offset, requestSize);
	if(solicitarBytes(solicitar) != 0 ){
		free(solicitar);
		log_error(logger, "Error al solicitar bytes (instruccion) a memoria.");
		return -1;
	}
	free(solicitar);
	return requestSize;
}

void finalizarPor(int type) {
	t_buffer_tamanio* paquete = serializar_pcb(pcb);
	header_t header;
	header.type = type;
	header.length = paquete->tamanioBuffer;
	if(sendSocket(socketConexionKernel, &header, (void*)paquete->buffer) <= 0){
		log_error(logger,"Error al notificar kernel el fin de ejecucion");
		finalizarCPU();
	}
	free(paquete->buffer);
	free(paquete);
	freePCB(pcb);
	quantum = -1;
	estaEjecutando = false;
}

void finalizarCPU(void){
	enviar_paquete_vacio(DESCONEXION_CPU, socketConexionKernel);
	finalizarConexion(socketConexionKernel);
	finalizarConexion(socketConexionMemoria);
	log_info(logger, "CPU desconectada!");
	log_destroy(logger);
	freeConf(config);
	exit(EXIT_FAILURE);
}
