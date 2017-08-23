#include "funcionesKernel.h"

void inicializarColas(){

	colaNew = queue_create();
	colaReady = queue_create();
	colaFinished = queue_create();

	max_pid = 0;
}

void crearConfig(int32_t argc, char* argv[]){

	const int path_size = 100;
	inotify_path = malloc(path_size);

	if(argc>1){
			if(verificarExistenciaDeArchivo(argv[1])){
				config=levantarConfiguracionKernel(argv[1]);
				log_info(logger, "Configuracion levantada correctamente");

				strcpy(inotify_path,argv[1]);
			}else{
				log_error(logger,"Ruta incorrecta");
				exit(EXIT_FAILURE);
			}
	}
	else if(verificarExistenciaDeArchivo(configuracionKernel)){
		config=levantarConfiguracionKernel(configuracionKernel);
		log_info(logger,"Configuracion levantada correctamente");

		strcpy(inotify_path,configuracionKernel);
	}
	else if(verificarExistenciaDeArchivo(string_substring_from(configuracionKernel,3))){
		config=levantarConfiguracionKernel(string_substring_from(configuracionKernel,3));
		log_info(logger,"Configuracion levantada correctamente");

		strcpy(inotify_path,string_substring_from(configuracionKernel,3));
	}
	else{
		log_error(logger,"No se pudo levantar el archivo de configuracion");
		exit(EXIT_FAILURE);
	}

	inicializarInotify();
}

t_config_kernel* levantarConfiguracionKernel(char* archivo_conf) {

        t_config_kernel* conf = malloc(sizeof(t_config_kernel));
        t_config* configKernel;
        char** varGlob, **semID, **semInit;

        log_debug(logger,"Path config: %s",archivo_conf);

        configKernel = config_create(archivo_conf);
        if(configKernel == NULL)
        	log_error(logger,"ERROR");

        if(!verificarConfig(configKernel))
        	log_error(logger,"CONFIG NO VALIDA");

        conf->puerto_CPU = malloc(MAX_LEN_PUERTO);
        strcpy(conf->puerto_CPU, config_get_string_value(configKernel, "PUERTO_CPU"));

        conf->puerto_PROG = malloc(MAX_LEN_PUERTO);
        strcpy(conf->puerto_PROG, config_get_string_value(configKernel, "PUERTO_PROG"));

        conf->ip_Memoria = malloc(MAX_LEN_IP);
        strcpy(conf->ip_Memoria, config_get_string_value(configKernel, "IP_MEMORIA"));

        conf->ip_kernel = malloc(MAX_LEN_IP);
		strcpy(conf->ip_kernel, config_get_string_value(configKernel, "IP_KERNEL"));

        conf->puerto_Memoria = malloc(MAX_LEN_PUERTO);
        strcpy(conf->puerto_Memoria, config_get_string_value(configKernel, "PUERTO_MEMORIA"));

        conf->ip_FS = malloc(MAX_LEN_IP);
        strcpy(conf->ip_FS, config_get_string_value(configKernel, "IP_FS"));

        conf->puerto_FS = malloc(MAX_LEN_PUERTO);
        strcpy(conf->puerto_FS, config_get_string_value(configKernel, "PUERTO_FS"));

        conf->quantum = config_get_int_value(configKernel, "QUANTUM");
        conf->quantum_Sleep = config_get_int_value(configKernel, "QUANTUM_SLEEP");

        conf->algoritmo = malloc(MAX_LEN_PUERTO);
        strcpy(conf->algoritmo, config_get_string_value(configKernel, "ALGORITMO"));

        conf->grado_MultiProg = config_get_int_value(configKernel, "GRADO_MULTIPROG");

        semID = config_get_array_value(configKernel, "SEM_IDS");
        semInit = config_get_array_value(configKernel, "SEM_INIT");
        conf->semaforos = crearDiccionarioConValue(semID,semInit);
        crearColasBloqueados(semID);

        varGlob = config_get_array_value(configKernel, "SHARED_VARS");
        conf->variablesGlobales = crearDiccionario(varGlob);

        conf->stack_Size = config_get_int_value(configKernel, "STACK_SIZE");

        config_destroy(configKernel);
        printf("Configuracion levantada correctamente.\n");
        return conf;
}

bool verificarConfig(t_config* a){
	return config_has_property(a,"PUERTO_PROG") &&
			config_has_property(a,"PUERTO_CPU") &&
			config_has_property(a,"IP_MEMORIA") &&
			config_has_property(a,"PUERTO_MEMORIA") &&
			config_has_property(a,"IP_FS") &&
			config_has_property(a,"PUERTO_FS") &&
			config_has_property(a,"QUANTUM") &&
			config_has_property(a,"QUANTUM_SLEEP") &&
			config_has_property(a,"GRADO_MULTIPROG") &&
			config_has_property(a,"SEM_IDS") &&
			config_has_property(a,"SEM_INIT") &&
			config_has_property(a,"SHARED_VARS") &&
			config_has_property(a,"STACK_SIZE") &&
			config_has_property(a,"ALGORITMO") &&
			config_has_property(a,"IP_KERNEL");
}

void enviarTamanioStack(int32_t fd){
	header_t* header=malloc(sizeof(header_t));
	header->type=TAMANIO_STACK_PARA_CPU;
	header->length=sizeof(config->stack_Size);
	sendSocket(fd,header,&config->stack_Size);
	free(header);
}

void enviarQuantumSleep(int32_t fd){
	header_t* header=malloc(sizeof(header_t));
	header->type = QUANTUM_SLEEP;
	header->length=sizeof( config->quantum_Sleep);
	sendSocket(fd,header,&config->quantum_Sleep);
	free(header);
}

proceso_en_espera_t* crearProcesoEnEspera(int32_t consola_fd, char* source){

	proceso_en_espera_t* proc = malloc(sizeof(proceso_en_espera_t));
	proc->socketConsola = consola_fd;
	proc->codigo = malloc(strlen(source) + 1);
	strcpy(proc->codigo,source);
	proc->pid = asignarPid();
	//pcb_t* pcb = crearPCB(source, asignarPid() );
	//pcb->consolaFd = consola_fd;

	return proc;
}

int32_t asignarPid(void){
	max_pid++;
	return max_pid;
}

int32_t conexionConFileSystem(void){

	socketConexionFS = createClient(config->ip_FS, config->puerto_FS);

	if(socketConexionFS == -1){
		return -1;
	}

	enviar_paquete_vacio(HANDSHAKE_KERNEL,socketConexionFS);

	log_info(logger, "Conexion con File System establecida");

	return 1;
}

int32_t conexionConMemoria(void){

	socketConexionMemoria = createClient(config->ip_Memoria, config->puerto_Memoria);

	if(socketConexionMemoria == -1){
		return -1;
	}

	enviar_paquete_vacio(HANDSHAKE_KERNEL,socketConexionMemoria);

	int32_t respuesta;
	char* paquete;

	recibir_paquete(socketConexionMemoria, &paquete, &respuesta);
	if(respuesta != HANDSHAKE_MEMORIA){
		return -1;
	}

	recibir_paquete(socketConexionMemoria, &paquete, &respuesta);
	if(respuesta != ENVIAR_TAMANIO_PAGINA) return -1;
	pagina_size =  *(int*)paquete;
	log_info(logger, "Conexion con memoria establecida");
	log_info(logger, "Tamanio de pagina recibido: %d", pagina_size);

	return 1;
}

void destruirConfiguracionKernel(t_config_kernel* config){
	free(config->algoritmo);
	free(config->ip_FS);
	free(config->ip_Memoria);
	free(config->puerto_CPU);
	free(config->puerto_FS);
	free(config->puerto_Memoria);
	free(config->puerto_PROG);
	free(config);
}

t_dictionary* crearDiccionarioConValue(char** array, char** valores){

        t_dictionary* dic = dictionary_create();
        uint16_t j = 0;
        while(array[j] != NULL){
        	valor[j]=atoi(valores[j]);
        	dictionary_put(dic, array[j], &valor[j]);
        	j++;
        }
        return dic;
}

t_dictionary* crearDiccionario(char** array){

        t_dictionary* dic = dictionary_create();
        int32_t j = 0;
        while(array[j] != NULL){
			dictionary_put(dic, array[j], 0);
			j++;
        }
        return dic;
}

/*
 * @NAME: LevantarInterfaz.
 * @DESC: Interfaz de Consola para manejar operaciones del kernel.
 */
void levantarInterfaz(void){
	//creo los comandos y el parametro
	comando* comandos = malloc(sizeof(comando)*7);

	strcpy(comandos[0].comando, "list");
	comandos[0].funcion = listProcesses;
	strcpy(comandos[1].comando, "info");
	comandos[1].funcion = processInfo;
	strcpy(comandos[2].comando, "tablaArchivos");
	comandos[2].funcion = getTablaArchivos;
	strcpy(comandos[3].comando, "grMulti");
	comandos[3].funcion = gradoMultiprogramacion;
	strcpy(comandos[4].comando, "kill");
	comandos[4].funcion = killProcess;
	strcpy(comandos[5].comando, "stopPlan");
	comandos[5].funcion = stopPlanification;
	strcpy(comandos[6].comando, "help");
	comandos[6].funcion = showHelp;


	interface_thread_param* params = malloc(sizeof(interface_thread_param));
	params->comandos = comandos;
	params->cantComandos = 7;

	//Lanzo el thread
	pthread_t threadInterfaz;
	pthread_attr_t atributos;
	pthread_attr_init(&atributos);
	pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

	pthread_create(&threadInterfaz, &atributos, (void*)interface, params);

	return;
}

void modificarValorDiccionario(t_dictionary* dic, char* key, void* data){
	// No hago un get porque si es null tira segmentation fault.
	dictionary_remove(dic, key);
	dictionary_put(dic, key, data);
}

int32_t leerVariableGlobal(t_dictionary* dic, char* key){
	int* valor = dictionary_get(dic, key);
	if(valor == NULL) return NULL;
	else return (int) *valor;
}

void escribirVariableGlobal(t_dictionary* dic, char* key, int32_t nuevoValor){
	log_debug(logger, "Se asigna el valor %d a la variable %s", nuevoValor, key);
	modificarValorDiccionario(dic, key, &nuevoValor);
}

int32_t semaforoSignal(t_dictionary* dic, char* key){
	int32_t* previo = dictionary_get(dic, key);
	return ++*previo;
}

int32_t semaforoWait(t_dictionary* dic, char* key){
	int32_t* previo = dictionary_get(dic, key);
	return --*previo;
}

void listProcesses(char* comando, char* param){

	int32_t estado;

	void listarProcesos(info_estadistica_t* info){
		int32_t cant = 0;
		if(info->estado == estado){
			printf("Proceso pid: %d\n", info->pid);
			cant++;
		}
		if(cant == 0) printf("No existen procesos que se encuentren en ese estado\n");
	}

	void listarTodos(info_estadistica_t* info){
		printf("El proceso con pid %d se encuentra en estado ", info->pid);
		switch (info->estado) {
			case NEW:
				printf("NEW\n");
				break;
			case READY:
				printf("READY\n");
				break;
			case EXEC:
				printf("EXEC\n");
				break;
			case FINISH:
				printf("FINISH\n");
				break;
			case BLOQ:
				printf("BLOQ\n");
				break;
		}
	}

	if(!strcmp(param, "")){
		list_iterate(listadoEstadistico, listarTodos);
		return;
	}

	if(!strcmp(param, "new")) estado = NEW;
	if(!strcmp(param, "ready")) estado = READY;
	if(!strcmp(param, "exec")) estado = EXEC;
	if(!strcmp(param, "finish")) estado = FINISH;
	if(!strcmp(param, "bloq")) estado = BLOQ;

	list_iterate(listadoEstadistico, listarProcesos);

}

void processInfo(char* comando, char* param){
	if(param == NULL || strlen(param) == 0 ){
		printf("Se necesida el pid del proceso.\n");
		return;
	}

	if(!esNumero(param)){
		printf("Ingrese un valor numerico valido para el proceso\n");
		return;
	}

	int32_t pid = atoi(param);

	bool buscar(info_estadistica_t* info){
		return info->pid == pid ? true : false;
	}
	info_estadistica_t* info = list_find(listadoEstadistico, buscar);
	if(info == NULL){
		printf("No se encuentra ese poceso en el sistema\n");
	}else{
		printf("Cantidad rafagas: %d\n", info->cantRafagas);
		printf("Cantidad operaciones privilegiadas: %d\n", info->cantOpPrivi);
		printf("Cantidad syscalls: %d\n", info->cantSyscalls);
		printf("Manejo del HEAP\n");
		printf("Cantidad paginas reservadas: %d\n", info->cantPagReservar);
		printf("Cantidad paginas liberadas: %d\n", info->cantPagLiberar);
		printf("Cantidad acciones alocar: %d\n", info->cantAlocar);
		printf("Total bytes alocados: %d\n", info->cantBytesAlocar);
		printf("Cantidad acciones liberar: %d\n", info->cantLiberar);
		printf("Total bytes liberados: %d\n", info->cantBytesLiberar);
		printf("Memory leak: %d bytes\n", info->cantBytesAlocar-info->cantBytesLiberar);
	}

	printf("Archivos Abiertos: \n");

	bool buscarArchivos(entrada_tabla_archivo_proceso* entrada){
		return entrada->proceso == pid ? true : false;
	}

	entrada_tabla_archivo_proceso* entrada = list_find(processFileTable, buscarArchivos);

	if(entrada != NULL && !list_is_empty(entrada->archivos)){

		void mostrarArchivos(t_archivo* archivo){
			printf("fd: %d , ", archivo->fd);
			printf("flags : %s \n", archivo->flags);
		}

		list_iterate(entrada->archivos, mostrarArchivos);
	}else{
		printf("El proceso no tiene archivos abiertos.\n");
	}

}

void getTablaArchivos(char* comando, char* param){
        imprimirTablaGlobal();
}
void gradoMultiprogramacion(char* comando, char* param){
	if(!esNumero(param)){
		printf("Ingrese un valor valido para el grado de multiporgramacion\n");
		return;
	}
	config->grado_MultiProg = atoi(param);
	printf("Grado de Multiprogramacion cambiado con exito, ahora es %d\n", config->grado_MultiProg);
	
	planificarLargoPlazo();
}
void killProcess(char* comando, char* param){
        if(!esNumero(param)){
        	printf("Ingrese un valor valido\n");
        	return;
        }
        int32_t pid = atoi(param);
        info_estadistica_t* info = buscarInformacion(pid);
        if(info == NULL){
        	printf("Ese proceso no se encuentra en el sistema\n");
        	return;
        }
        if(info->estado != EXEC){ //como no esta ejecutando no tengo que esperar a que cpu lo devuelva
        	eliminarEstadistica(info->pid);
        	enviar_paquete_vacio(FINALIZAR_EJECUCION, info->socketConsola);
        }else{ //debo esperar a que el cpu devuelva el pcb;
        	info->matarSiguienteRafaga = true;
        }

}
void stopPlanification(char* comando, char* param){
       if(planificacionActivada){
    	   pthread_mutex_lock(&lockPlanificacion);
    	   planificacionActivada = false;
    	   pthread_mutex_unlock(&lockPlanificacion);
    	   printf("Planificacion desactiviada\n");
       }else{
    	   pthread_mutex_lock(&lockPlanificacion);
    	   planificacionActivada = true;
    	   pthread_cond_signal(&lockCondicionPlanificacion);
    	   pthread_mutex_unlock(&lockPlanificacion);
    	   printf("Planificacion Activada\n");
       }
}

void showHelp(char* comando, char* param){
	puts("list [<status>] - muestra todos los programas en un determinado estado (new, ready, exec, bloq, finish)");
	puts("tablaArchivos   - muestra la tabla de archivos");
 	puts("info <pid>      - muestra información del programa asociado al PID ingresado");
 	puts("kill <pid>      - finaliza el programa correspondiente al PID ingresado");
 	puts("grMulti <grado> - cambia del grado de multiprogramación del sistema");
 	puts("stopPlan        - inicia/detiene la planificación de los procesos");
 	puts("help            - muestra comandos y descripciones");
}

void agregarNuevaCPU(t_list* lista, int socketCPU){
	cpu_t* nuevaCPU = malloc(sizeof(cpu_t));
	nuevaCPU->socket = socketCPU;
	nuevaCPU->pcb = NULL;
	nuevaCPU->disponible = true;

	list_add(lista, nuevaCPU);
	sem_post(&semCPUs_disponibles);
}

void liberarCPU(cpu_t* cpu){
	freePCB(cpu->pcb);
	free(cpu);
}

void eliminarCPU(t_list* lista, int32_t socketCPU){

	bool condicion(cpu_t* cpu){
		return cpu->socket == socketCPU ? true : false;
	}

	list_remove_and_destroy_by_condition(lista, condicion, liberarCPU);
}

void actualizarReferenciaPCB(int32_t id, t_pcb* pcb){

	bool condicion(cpu_t* cpu){
		return cpu->socket == id ? true : false;
	}

	cpu_t* cpu = list_find(listaCPUs, condicion);
	cpu->pcb = pcb;
}

cpu_t* obtenerCpuLibre(void){

	bool estaLibre(cpu_t* cpu){
		return cpu->disponible;// != NULL ? false : true;
	}
	return list_find(listaCPUs, estaLibre);
}

void planificarCortoPlazo(void){

	while(1){
		//Espera procesos para ejecutar
		sem_wait(&sem_cola_ready);

		//evaluo si la planificacion esta activada
		pthread_mutex_lock(&lockPlanificacion);
		while(!planificacionActivada){
			pthread_cond_wait(&lockCondicionPlanificacion, &lockPlanificacion);
		}
		pthread_mutex_unlock(&lockPlanificacion);

		//Espera que halla CPUs Disponibless
		sem_wait(&semCPUs_disponibles);
		cpu_t* cpu = obtenerCpuLibre();

		sem_wait(&mutex_cola_ready);
		t_pcb* pcb = queue_pop(colaReady);
		sem_post(&mutex_cola_ready);

		cpu->pcb = pcb;
		cpu->disponible = false;
		enviarPcbCPU(pcb, cpu->socket);
		estadisticaCambiarEstado(pcb->pid, EXEC);
		printf("Proceso #%d agregado a cola de EJECUCION\n", pcb->pid);
	}
}

void enviarPcbCPU(t_pcb* pcb, int32_t socketCPU){
	t_buffer_tamanio* buffer = serializar_pcb(pcb);
	header_t header;
	header.type = EXEC_PCB;
	header.length = buffer->tamanioBuffer;
	sendSocket(socketCPU, &header, buffer->buffer);

	int32_t quantum = 0;
	header.type=EXEC_QUANTUM;
	if(!strcmp(config->algoritmo, "RR")){
		quantum = config->quantum;
	}
	header.length = sizeof(int);
	sendSocket(socketCPU, &header, &quantum);
}

void planificarLargoPlazo(void){
	if(cantProcesosSistema < config->grado_MultiProg && queue_size(colaNew) != 0){
		bool finCola;
		int32_t cantProcChequeados = 0;
		int32_t cantidadDeProcesosEnNew = queue_size(colaNew);
		while(!finCola && (cantProcesosSistema < config->grado_MultiProg) ){

			cantProcChequeados++;
			finCola = (cantProcChequeados == cantidadDeProcesosEnNew);

			sem_wait(&mutex_cola_new);
			proceso_en_espera_t* proc = queue_pop(colaNew);
			sem_post(&mutex_cola_new);
			printf("Proceso #%d agregado a la cola de NEW\n", proc->pid);

			//hago peticion a memoria, si se rechaza alerto a consola y el grado de multiProg sigue igual
			//si acepta pongo en cola ready y creo pcb;

			//creo el pedido para la memoria
			t_pedido_iniciar pedido;
			int32_t pid = proc->pid;
			t_pcb* pcb = crearPCB(proc->codigo,pid,proc->socketConsola);

			int32_t cant_pag_cod = strlen(proc->codigo) / pagina_size;
			if(strlen(proc->codigo) % pagina_size > 0)
				cant_pag_cod++;

			pedido.pid = pid;
			pedido.cant_pag = config->stack_Size + cant_pag_cod;

			header_t header;
			header.type = INICIAR_PROGRAMA;
			header.length = sizeof(t_pedido_iniciar);

			void* paquete;
			int32_t resultado;
			pthread_mutex_lock(&mutex_memoria_fd);
			sendSocket(socketConexionMemoria, &header, &pedido);
			log_info(logger, "Pedido de paginas a memoria. pid:%d, cantPags:%d", pid, pedido.cant_pag);
			recibir_paquete(socketConexionMemoria, &paquete, &resultado);
			pthread_mutex_unlock(&mutex_memoria_fd);

			if(resultado == SIN_ESPACIO){
				//aviso a consola que se rechazo
				log_error(logger, "Proceso %d rechazado porque no hay espacio en memoria", pid);
				int32_t exitCode = FALLA_RESERVAR_RECURSOS;
				header_t header;
				header.type = PROCESO_RECHAZADO;
				header.length = sizeof(int) * 2;
				void* paquete = malloc(sizeof(int) * 2);
				memcpy(paquete, &pid, sizeof(int));
				memcpy(paquete + sizeof(int), &exitCode, sizeof(int));
				sendSocket(proc->socketConsola, &header, paquete);
				free(paquete);
				info_estadistica_t* info = buscarInformacion(pid);
				info->estado = FINISH;
				info->exitCode = exitCode;
				queue_push(colaFinished, pcb);
				log_info(logger, "Proceso #%d agregado a la cola de FINISHED", pid);
				printf("Proceso #%d agregado a la cola de FINISHED\n", pid);
				freePCB(pcb);
			}
			else if(resultado == OP_OK){
				log_info(logger, "Paginas reservadas para el proceso %d", pid);
				sem_wait(&mutex_cola_ready);
				queue_push(colaReady, pcb);
				sem_post(&mutex_cola_ready);
				sem_post(&sem_cola_ready);
				printf("Proceso #%d agregado a la cola de READY\n", pid);
				estadisticaCambiarEstado(pid, READY);
				//aviso a consola que se acepto
				alertarConsolaProcesoAceptado(&pid, proc->socketConsola);

				//mando a memoria el codigo
				envioCodigoMemoria(proc->codigo, pid, cant_pag_cod);

				cantProcesosSistema++;

				//destruyo el proceso en espera;
				free(proc->codigo);
				free(proc);
			}
		}
	}
}

int16_t solicitudBytes(int socket, t_pedido_bytes* pedido, char**buf){
	header_t header;
	void* paquete;
	int resultado;
	header.type=SOLICITUD_BYTES;
	header.length=sizeof(t_pedido_bytes);

	if(sendSocket(socket,&header,pedido) <= 0 ){
		log_error(logger,"Error al enviar. Desconexion...");
		return -1;
	}

	*buf = malloc(pedido->size);
	recibir_paquete(socket, &paquete, &resultado);
	memcpy(*buf, paquete, pedido->size);

	return resultado;
}

int16_t grabarBytes(int socket, t_pedido_bytes* pedido, void*buf){
	header_t header;
	void* paquete;
	int resultado;
	header.type=GRABAR_BYTES;
	header.length = sizeof(t_pedido_bytes) + pedido->size;

	paquete = malloc(header.length);
	memcpy(paquete, pedido, sizeof(t_pedido_bytes));
	memcpy(paquete+sizeof(t_pedido_bytes), buf, pedido->size);


	if(sendSocket(socket,&header,paquete) <= 0 ){
		log_error(logger,"Error al enviar. Desconexion...");
		free(paquete);
		return -1;
	}
	free(paquete);

	recibir_paquete(socket, &paquete, &resultado);


	return resultado;
}

void alertarConsolaProcesoAceptado(int* pid, int32_t socketConsola){
	header_t header;
	header.type = PID_PROGRAMA;
	header.length = sizeof(int);
	sendSocket(socketConsola, &header, pid);
}

void envioCodigoMemoria(char* codigo, int32_t pid, int32_t cant_pag){
	int32_t cod_size = strlen(codigo) + 1;

	header_t header;
	header.type = GRABAR_BYTES;

	void* buf = malloc(pagina_size + sizeof(t_pedido_bytes));

	((t_pedido_bytes*)buf)->pid = pid;
	((t_pedido_bytes*)buf)->offset = 0;

	char ptr[20];
	int rta;
	int32_t i, size;
	for(i=0;i<cant_pag;i++){

		//Cuanto voy a enviar
		size = min(cod_size - i * pagina_size, pagina_size);

		//Termino de armar el header
		header.length = sizeof(t_pedido_bytes) + size;
		((t_pedido_bytes*)buf)->pag = i;
		((t_pedido_bytes*)buf)->size = size;

		memcpy(buf + sizeof(t_pedido_bytes),codigo + i*pagina_size, size);

		pthread_mutex_lock(&mutex_memoria_fd);
		sendSocket(socketConexionMemoria, &header, buf);
		recibir_paquete(socketConexionMemoria,&ptr,&rta);
		pthread_mutex_unlock(&mutex_memoria_fd);

		if(rta != OP_OK)
			log_warning(logger,"No pude escribir la pagina en memoria");
	}

	free(buf);
}

void crearInfoEstadistica(int32_t pid, uint32_t socketConsola){
	info_estadistica_t* info = malloc(sizeof(info_estadistica_t));
	info->pid = pid;
	info->cantLiberar = 0;
	info->cantAlocar = 0;
	info->cantOpPrivi = 0;
	info->cantRafagas = 0;
	info->cantSyscalls = 0;
	info->estado = NEW;
	info->socketConsola = socketConsola;
	info->matarSiguienteRafaga = false;
	info->cantBytesAlocar = 0;
	info->cantBytesLiberar = 0;
	info->cantPagLiberar = 0;
	info->cantPagReservar = 0;
	info->exitCode = NULL;
	list_add(listadoEstadistico, info);
}

info_estadistica_t* buscarInformacion(int32_t pid){

	bool buscar(info_estadistica_t* info){
		return info->pid == pid ? true : false;
	}
	return list_find(listadoEstadistico, buscar);
}

void estadisticaAumentarRafaga(int32_t pid){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantRafagas++;
}
void estadisticaAumentarSyscall(int32_t pid){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantSyscalls++;
}
void estadisticaAumentarOpPriviligiada(int32_t pid){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantOpPrivi++;
}
void estadisticaAumentarAlocar(int32_t pid){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantAlocar++;
}
void estadisticaAumentarLiberar(int32_t pid){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantLiberar++;
}
void estadisticaAlocarBytes(int32_t pid, int32_t cant){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantBytesAlocar+=cant;
}
void estadisticaLiberarBytes(int32_t pid, int32_t cant){
	info_estadistica_t* info = buscarInformacion(pid);
	info->cantBytesLiberar+=cant;
}
void estadisticaCambiarEstado(int32_t pid, uint8_t nuevoEstado){
	info_estadistica_t* info = buscarInformacion(pid);
	info->estado = nuevoEstado;
}
void aumentarEstadisticaPorSocketAsociado(int32_t socket, void(*estadistica)(int32_t pid)){
	cpu_t* cpu = obtener_cpu_por_socket_asociado(socket);
	estadistica(cpu->pcb->pid);
}
void eliminarEstadistica(int32_t pid){

	bool buscar(info_estadistica_t* info){
		return info->pid == pid ? true : false;
	}

	list_remove_and_destroy_by_condition(listadoEstadistico, buscar, free);
}


void crearColasBloqueados(char** semaforos){
	int32_t j = 0;

	bloqueos = dictionary_create();

	while(semaforos[j] != NULL){
		dictionary_put(bloqueos, semaforos[j], queue_create());
		j++;
	}

}

void desbloquearProceso(char* semaforo){
	t_queue* cola = (t_queue*)dictionary_get(bloqueos, semaforo);

	if(!queue_is_empty(cola)){

		t_pcb* pcb = queue_pop(cola);

		queue_push(colaReady, pcb);
		estadisticaCambiarEstado(pcb->pid, READY);
		sem_post(&sem_cola_ready);
		printf("Proceso #%d agregado a la cola de READY\n", pcb->pid);
	}
}

void bloquearProceso(char* semaforo, t_pcb* pcb){
	t_queue* cola = (t_queue*)dictionary_get(bloqueos, semaforo);
	if(cola == NULL) log_warning(logger, "No se encontro la cola con el semaforo %s", semaforo);
	else queue_push(cola, pcb);
	estadisticaAumentarRafaga(pcb->pid);
	estadisticaCambiarEstado(pcb->pid, BLOQ);
	printf("Proceso #%d agregado a cola de BLOQUEADOS\n",pcb->pid);
}

int32_t getArchivoFdMax(void){
	max_archivo_fd++;
	return max_archivo_fd;
}

void crearEntradaArchivoProceso(int32_t proceso){
	printf("creo entrada archivo proceso %d\n", proceso);
	entrada_tabla_archivo_proceso* entrada = malloc(sizeof(entrada_tabla_archivo_proceso));
	entrada->proceso = proceso;
	entrada->archivos = list_create();

	list_add(processFileTable, entrada);
}

int32_t agregarArchivo_aProceso(int32_t proceso, char* file, char* permisos){
	bool buscar(entrada_tabla_archivo_proceso* entrada){
		return entrada->proceso == proceso;
	}
	bool buscarArchivo(entrada_tabla_globlal_archivo* entrada){
		return !strcmp(entrada->archivo, file);
	}
	entrada_tabla_archivo_proceso* entrada = list_find(processFileTable, buscar);
	entrada_tabla_globlal_archivo* entradaGlobal = list_find(globalFileTable, buscarArchivo);
	int fd = getArchivoFdMax();
	if(entradaGlobal == NULL){ // no existe
		uint32_t sizeEntrada = strlen(file) + 1 + sizeof(int32_t) * 2;
		entradaGlobal = malloc(sizeEntrada);
		entradaGlobal->archivo = file;
		entradaGlobal->vecesAbierto = 1;
		entradaGlobal->ubicacion = proceso + fd;

		list_add(globalFileTable, entradaGlobal);
	}else{ //existe en la tabla global
		entradaGlobal->vecesAbierto++;

	}

	t_archivo* archivo = malloc(sizeof(t_archivo));
	archivo->flags = permisos;
	archivo->fd = fd;
	archivo->globalFD = entradaGlobal->ubicacion; //ver esto que es una paja
	archivo->cursor = 0;
	list_add(entrada->archivos, archivo);

	return archivo->fd;
}

int eliminarFd(int fd, int proceso){
	bool buscarPorProceso(entrada_tabla_archivo_proceso* entrada){
		return entrada->proceso == proceso ? true : false;
	}
	entrada_tabla_archivo_proceso* entrada = list_find(processFileTable, buscarPorProceso);
	if(entrada == NULL) return -1;
	int i;
	t_archivo* archivo = buscarArchivo(proceso, fd);
	if(archivo == NULL) return -1;
	char* path = buscarPathDeArchivo(archivo->globalFD);
	if(path == NULL) return -1;
	free(archivo);

	for(i=0; i< list_size(entrada->archivos);i++){
		t_archivo* archivo = list_get(entrada->archivos, i);
		if(archivo->fd == fd){
			list_remove(entrada->archivos, i);
			free(archivo);
		}
	}

	free(entrada);

	for(i = 0; i<list_size(globalFileTable);i++){
		entrada_tabla_globlal_archivo* entradaGlobal = list_get(globalFileTable,i);
		if(strcmp(entradaGlobal->archivo, path) == 0){
			entradaGlobal->vecesAbierto--;
			if(entradaGlobal->vecesAbierto == 0){
				list_remove(globalFileTable,i);
				free(entradaGlobal);
			}
		}
	}

	return 0;
}

void imprimirTablaGlobal(void){

	void imprimirData(entrada_tabla_globlal_archivo* entrada){
		printf("Nombre del file: %s\n", entrada->archivo);
		printf("Cantidad de veces abierto: %d\n", entrada->vecesAbierto);
	}

	list_iterate(globalFileTable, imprimirData);

}

char* buscarPathDeArchivo(int globalFD){
	bool buscarPorUbicacion(entrada_tabla_globlal_archivo* entrada){
		return entrada->ubicacion == globalFD ? true : false;
	}

	entrada_tabla_globlal_archivo* entrada = list_find(globalFileTable, buscarPorUbicacion);
	return entrada->archivo;
}

t_archivo* buscarArchivo(int32_t pid, int32_t fd){

	bool buscarPorPid(entrada_tabla_archivo_proceso* entrada){
		return entrada->proceso == pid ? true : false;
	}

	entrada_tabla_archivo_proceso* entrada = list_find(processFileTable, buscarPorPid);
	if(entrada == NULL) return NULL;

	bool buscarPorArchivo(t_archivo* arch){
		return arch->fd == fd ? true : false;
	}

	t_archivo* archivo = list_find(entrada->archivos, buscarPorArchivo);
	return archivo;

}

bool existeArchivo(char* path){

	bool buscarPorPath(entrada_tabla_globlal_archivo* entrada){
		return !strcmp(path, entrada->archivo);
	}

	return list_any_satisfy(globalFileTable, buscarPorPath);
}

bool archivoPuedeSerBorrado(int globalFD){

	bool buscarPorGlobalFD(entrada_tabla_globlal_archivo* entrada){
		return entrada->ubicacion == globalFD ? true : false;
	}

	entrada_tabla_globlal_archivo* entrada =  list_find(globalFileTable, buscarPorGlobalFD);

	return entrada->vecesAbierto == 1 ? true : false;
}

void verificarProcesosEnCpuCaida(int socketCPU){
		int i;
		for(i = 0; i<list_size(listaCPUs); i++){
			sem_wait(&mutex_lista_CPUs);
			cpu_t* cpu = list_get(listaCPUs, i);
			sem_post(&mutex_lista_CPUs);
			if(cpu->socket == socketCPU){
				list_remove(listaCPUs, i);
				log_info(logger, "CPU %d quitado de la lista", cpu->socket);
				if(cpu->disponible) sem_wait(&semCPUs_disponibles);
				// si esta disponible es porque no tiene nada corriendo
				if(!(cpu->disponible) && cpu->pcb != NULL){
					log_info(logger, "Se termina la ejecucion del proceso #%d por desconexion de la CPU", cpu->pcb->pid);
					cpu->pcb->exitCode = DESCONEXION_CPU;
					terminarProceso(cpu->pcb, socketCPU);
				}
				free(cpu);
			}
		}
}

void verificarProcesosConsolaCaida(uint32_t socketConsola){
	info_estadistica_t* info = buscarInformacionPorSocketConsola(socketConsola);
	if(info->estado != FINISH){
		info->matarSiguienteRafaga = true;
		info->exitCode = DESCONEXION_CONSOLA;
		log_info(logger, "Se termina la ejecucion del proceso %d por desconexion de la consola", info->pid);
	}
}

//////////////////////////////
//Quilombo del inotify
void inicializarInotify(){
	inotify_fd = inotify_init();
	create_wd = inotify_add_watch(inotify_fd,inotify_path,IN_CREATE);
	delete_wd = inotify_add_watch(inotify_fd,inotify_path,IN_DELETE);
	modify_wd = inotify_add_watch(inotify_fd,inotify_path,IN_MODIFY);
}

void cambiarConfig()
{
	struct inotify_event* evento = malloc(sizeof(struct inotify_event));

	if( read(inotify_fd, evento, sizeof(struct inotify_event)) <= 0)
	{
		log_error(logger,"Error al leer el inotify");
	}

	if(evento->mask == IN_MODIFY || evento->mask == IN_CREATE || evento->mask == IN_DELETE)
	{
		//Guardo los valores viejos
		int quantumViejo = config->quantum;
		int sleepViejo = config->quantum_Sleep;

		t_config* configNueva = config_create(inotify_path);

		if(configNueva == NULL)
			log_error(logger, "Error al crear nueva configuracion");
		if(configNueva->properties == NULL)
			log_error(logger, "Error al crear nueva configuracion");

		if(config_has_property(configNueva,"QUANTUM") && config_has_property(configNueva,"QUANTUM_SLEEP")){
			config->quantum = config_get_int_value(configNueva, "QUANTUM");
			config->quantum_Sleep = config_get_int_value(configNueva, "QUANTUM_SLEEP");

			if(config->quantum != quantumViejo || config->quantum_Sleep != sleepViejo)
				log_info(logger,"Se detecto un cambio de configuracion\nNuevo Quantum Sleep: %d Quantum: %d",config->quantum_Sleep,config->quantum);
		}else
			log_info(logger,"No pude crear Config");

		config_destroy(configNueva);

		//config = levantarConfiguracionKernel(inotify_path);

		log_info(logger,"Se detecto un cambio de configuracion\nNuevo Quantum Sleep: %d Quantum: %d",config->quantum_Sleep,config->quantum);
	}

	free(evento);
}
