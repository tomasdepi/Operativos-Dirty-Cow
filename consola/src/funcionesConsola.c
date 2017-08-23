#include "funcionesConsola.h"

void crearConfig(int argc, char* argv[]) {
	if(argc>1){
			if(verificarExistenciaDeArchivo(argv[1])){
				config=levantarConfiguracionConsola(argv[1]);
				log_info(logger, "Configuracion levantada correctamente");
				log_error(logger,"Ruta incorrecta");
				exit(EXIT_FAILURE);
			}
	}
	else if(verificarExistenciaDeArchivo(configuracionConsola)){
		config=levantarConfiguracionConsola(configuracionConsola);
		log_info(logger,"Configuracion levantada correctamente");
	}
	else if(verificarExistenciaDeArchivo(string_substring_from(configuracionConsola,3))){
		config=levantarConfiguracionConsola(string_substring_from(configuracionConsola,3));
		log_info(logger,"Configuracion levantada correctamente");
	}
	else{
		log_error(logger,"No se pudo levantar el archivo de configuracion");
		exit(EXIT_FAILURE);
	}
}

t_config_consola* levantarConfiguracionConsola(char * archivo) {

	t_config_consola* config = malloc(sizeof(t_config_consola));
	t_config* configConsola;

	configConsola = config_create(archivo);

	config->ip_Kernel = malloc(
			strlen(config_get_string_value(configConsola, "IP_KERNEL")) + 1);
	strcpy(config->ip_Kernel,
			config_get_string_value(configConsola, "IP_KERNEL"));

	config->puerto_Kernel = malloc(
			strlen(config_get_string_value(configConsola, "PUERTO_KERNEL")) + 1);
	strcpy(config->puerto_Kernel,
			config_get_string_value(configConsola, "PUERTO_KERNEL"));

	config->program_path = malloc(
			strlen(config_get_string_value(configConsola, "PROGRAM_PATH")) + 1);
	strcpy(config->program_path,
			config_get_string_value(configConsola, "PROGRAM_PATH"));

	config_destroy(configConsola);
	printf("Configuracion levantada exitosamente\n");
	printf("Ruta por defecto: '%s'\n", config->program_path);
	printf("No es necesario escribir el '.ansisop'\n");
	return config;
}


int enviarArchivo(int kernel_fd, char* path){

	//Verifico existencia archivo (Aguante esta funcion loco!)
 	if( !verificarExistenciaDeArchivo(path) ){
 		log_error(logger, "No existe el archivo");
 		return -1;
 	}

 	FILE* sourceFile;
 	int file_fd, file_size;
 	struct stat stats;

 	//Abro el archivo y le saco los stats
 	sourceFile = fopen(path, "r");
 	//esto nunca deberia fallar porque ya esta verificado, pero por las dudas
 	if(sourceFile == NULL){
 		log_error(logger, "No pudo abrir el archivo");
 		return -1;
 	}
 	file_fd = fileno(sourceFile);

 	fstat(file_fd, &stats);
 	file_size = stats.st_size;
 	header_t header;
 	char* buffer = malloc(file_size + 1 + sizeof(header_t));
 	memset(buffer,'@',file_size + 1 + sizeof(header_t));
 	int offset = 0;

 	if(buffer == NULL){
 		log_error(logger, "No se pudo reservar memoria para enviar archivo");
 		fclose(sourceFile);
 		return -1;
 	}

 	header.type = ENVIO_CODIGO;
 	header.length = file_size + 1;
 	memcpy(buffer, &(header.type),sizeof(header.type)); offset+=sizeof(header.type);
 	memcpy(buffer + offset, &(header.length),sizeof(header.length)); offset+=sizeof(header.length);

 	if(fread(buffer + offset,file_size,1,sourceFile) < 1){
 		log_error(logger, "No es posible leer el archivo");
 		free(buffer);
 		fclose(sourceFile);
 		return -1;
 	}

 	//Agrego \0
 	(buffer + offset)[file_size] = '\0';

 	FILE* dumpFile = fopen("Dump","w");

 	write(fileno(dumpFile),buffer,file_size + sizeof(header_t) + 1);

 	fclose(dumpFile);

 	/*Esto lo hago asi porque de la otra forma habría que reservar MAS espacio para
 	 * enviar el paquete */
 	if(sendAll(kernel_fd, buffer, file_size + sizeof(header_t) + 1, 0) <= 0){
 		log_error(logger, "Error al enviar archivo");
 		free(buffer);
 		fclose(sourceFile);
 		return -1;
 	}
 	free(buffer);
 	fclose(sourceFile);
 	return 0;
}

//funciones interfaz
void levantarInterfaz() {
	//creo los comandos y el parametro
	comando* comandos = malloc(sizeof(comando) * 5);

	strcpy(comandos[0].comando, "start");
	comandos[0].funcion = iniciarPrograma;
	strcpy(comandos[1].comando, "stop");
	comandos[1].funcion = finalizarPrograma;
	strcpy(comandos[2].comando, "disconnect");
	comandos[2].funcion = desconectarConsola;
	strcpy(comandos[3].comando, "clean");
	comandos[3].funcion = limpiarMensajes;
	strcpy(comandos[4].comando, "help");
	comandos[4].funcion = showHelp;

	interface_thread_param* params = malloc(sizeof(interface_thread_param));
	params->comandos = comandos;
	params->cantComandos = 5;

	//Lanzo el thread
	pthread_attr_t atributos;
	pthread_attr_init(&atributos);
	pthread_create(&threadInterfaz, &atributos, (void*)interface, params);

	return;
}

void showHelp(char* comando, char* param){
	puts("start <path>	- envia a ejecutar el codigo del programa ingresado");
	puts("stop <pid>	- detiene la ejecucion del programa asociado al PID ingresado");
 	puts("disconnect	- desconecta la consola y finaliza los programas");
 	puts("clean			- limpia la pantalla");
 	puts("help			- muestra comandos y descripciones");
}

void iniciarPrograma(char* comando, char* param) {

	int socket_cliente;

	char* absolute_path = crearPath(param);

	if(!verificarExistenciaDeArchivo(absolute_path)){
		log_warning(logger, "No se encontro el archivo ingresado");
		printf("No se encontro el archivo ingresado\n");
		return;
	}

	socket_cliente = createClient(config->ip_Kernel, config->puerto_Kernel);
	if (socket_cliente != -1) {
		log_info(logger, "Cliente creado satisfactoriamente");
	}else{
		log_error(logger, "No se pudo crear el cliente");
	}
	enviar_paquete_vacio(HANDSHAKE_PROGRAMA, socket_cliente);
	int operacion = 0;
	void* paquete_vacio;

	if(recibir_paquete(socket_cliente, &paquete_vacio, &operacion) <= 0){
		log_error(logger, "El kernel se desconecto");
		close(socket_cliente);
		exit(EXIT_FAILURE);
	}

	if (operacion == HANDSHAKE_KERNEL) {
		log_info(logger, "Conexion con Kernel establecida");
		printf("Conexion exitosa con Kernel\n");
		log_debug(logger, "Se procede a mandar el archivo: '%s'", param);
	} else {
		log_error(logger, "El Kernel no devolvio handshake");
	}

	dataHilo* data = malloc(sizeof(dataHilo));
	data->pathAnsisop = malloc(strlen(absolute_path)+1);
	memcpy(data->pathAnsisop, absolute_path, strlen(absolute_path)+1);
	data->socket = socket_cliente;

	pthread_t thread;
	pthread_create(&thread, NULL, (void*)threadPrograma, data);
	pthread_detach(thread);
}

t_proceso* crearProceso(int socketProceso, pthread_t threadPrograma, int pid){
	t_proceso* proc = malloc(sizeof(t_proceso));
	proc->socket = socketProceso;
	proc->thread = threadPrograma;
	proc->pid = pid;
	proc->impresiones = 0;

	time_t tiempo = time(0);
    struct tm * inicio = localtime(&tiempo);
    proc->fechaInicio = inicio;

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    proc->start = start;

	list_add(procesos, proc);
	return proc;
}

void threadPrograma(dataHilo* data){

	int operacion;
	void* paquete;
	bool procesoActivo = true;
	int pidAsignado;
	int socketProceso = data->socket;
	pthread_t thread = pthread_self();

	if((enviarArchivo(socketProceso, data->pathAnsisop))==-1){
		log_error(logger,"No pudo enviarse el archivo");
		return;
	}
	log_info(logger,"Archivo enviado correctamente");
	printf("Archivo '%s' enviado exitosamente\n", data->pathAnsisop);

	if(recibir_paquete(socketProceso, &paquete, &operacion) <= 0){
		log_error(logger, "El kernel se desconecto");
		close(socketProceso);
		exit(EXIT_FAILURE);
	}
	int32_t exitCode;
	switch(operacion){
	case PROCESO_RECHAZADO:
		pidAsignado = *(int*) paquete;
		printf("PID asignado: #%d\n", pidAsignado);
		exitCode = *(int*) (paquete + sizeof(int));
		free(paquete);
		t_proceso* proc = crearProceso(socketProceso,thread,pidAsignado);
		terminarProceso(proc, exitCode);
		return;
	case PID_PROGRAMA:
		pidAsignado = *(int*)paquete;
		printf("PID asignado: #%d\n", pidAsignado);
		free(paquete);
		crearProceso(socketProceso,thread,pidAsignado);
		log_info(logger, "Programa #%d aceptado por el kernel", pidAsignado);
		break;
	default:
		log_warning(logger, "Se recibio una operacion invalida");
		return;
	}

	while(procesoActivo){
		/*ambos se quedan esperando una respuesta del otro*/
		if(recibir_paquete(socketProceso, (void*)&paquete, &operacion) <= 0){
			log_error(logger, "El kernel se desconecto");
			if(paquete)free(paquete);
			exit(EXIT_FAILURE);
		}else{

			switch (operacion) {
			case FINALIZAR_EJECUCION:
				finalizarEjecucionProceso(&procesoActivo, data, *(int32_t*) paquete);
				break;
			case IMPRIMIR_POR_PANTALLA:
				imprimirPorPantalla(paquete);
				break;
			default:
				break;
			}

		}
		if(paquete)free(paquete);

	}
}

void finalizarEjecucionProceso(bool* procesoActivo, dataHilo* data, int32_t exitCode){
	bool buscarPorSocket(t_proceso* proc){
		return proc->socket == data->socket ? true : false;
	}

	t_proceso* proc = list_find(procesos, buscarPorSocket);

	terminarProceso(proc, exitCode);

	procesoActivo = false;
	free(data);
}

void cargarFechaFin(t_proceso* proc){
	time_t tiempo = time(0);
	struct tm * fin = localtime(&tiempo);
	proc->fechaFin = fin;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	proc->end = end;
}

void imprimirInformacion(t_proceso* proceso, int32_t exitCode){
	uint32_t msInicio = proceso->start.tv_nsec / 1000000 + proceso->start.tv_sec * 1000;
	uint32_t msFin = proceso->end.tv_nsec / 1000000 + proceso->end.tv_sec * 1000;
	int segDuracion = proceso->end.tv_sec - proceso->start.tv_sec;
	int msDuracion = msFin - msInicio - (segDuracion * 1000);
	printf("-----FIN PROGRAMA-----\n");
	printf("Pid #%d\n", proceso->pid);
	printf("Inicio: %d/%d/%d %d:%d:%d\n",  proceso->fechaInicio->tm_mday, proceso->fechaInicio->tm_mon + 1, proceso->fechaInicio->tm_year + 1900, proceso->fechaInicio->tm_hour, proceso->fechaInicio->tm_min, proceso->fechaInicio->tm_sec);
	int varAuxSegundos = proceso->fechaInicio->tm_sec + segDuracion;
	if(varAuxSegundos + segDuracion > 60){
		int minutos = (varAuxSegundos + segDuracion) / 60;
		proceso->fechaFin->tm_min += minutos;
		proceso->fechaFin->tm_sec = (varAuxSegundos + segDuracion) % 60;
	}else{
		proceso->fechaFin->tm_sec = varAuxSegundos;
	}
	printf("Fin:  %d/%d/%d %d:%d:%d\n", proceso->fechaFin->tm_mday, proceso->fechaFin->tm_mon + 1,proceso->fechaFin->tm_year + 1900, proceso->fechaFin->tm_hour, proceso->fechaFin->tm_min, proceso->fechaFin->tm_sec);
	printf("Cantidad de impresiones por pantalla: %d\n", proceso->impresiones);
	if(msDuracion < 0) msDuracion += 1000;
	printf("Duracion: %d seg - %d ms\n", segDuracion, msDuracion);
	char* exitCodeString = obtenerExitCode(exitCode);
	printf("Exit code: %s\n", exitCodeString);
	printf("----------------------\n");
}

char* obtenerExitCode(int32_t exitCode){
	switch(exitCode){
	case FINALIZO_BIEN: return "FINALIZO_BIEN";
	case FALLA_RESERVAR_RECURSOS: return "FALLA_RESERVAR_RECURSOS";
	case ARCHIVO_INEXISTENTE: return "ARCHIVO_INEXISTENTE";
	case LEER_ARCHIVO_SIN_PERMISOS: return "LEER_ARCHIVO_SIN_PERMISOS";
	case ESCRIBIR_ARCHIVO_SIN_PERMISOS: return "ESCRIBIR_ARCHIVO_SIN_PERMISOS";
	case ERROR_MEMORIA: return "ERROR_MEMORIA";
	case DESCONEXION_CONSOLA: return "DESCONEXION_CONSOLA";
	case FINALIZAR_DESDE_CONSOLA: return "FINALIZAR_DESDE_CONSOLA";
	case SUPERO_TAMANIO_PAGINA: return "SUPERO_TAMANIO_PAGINA";
	case SUPERA_LIMITE_ASIGNACION_PAGINAS: return "SUPERA_LIMITE_ASIGNACION_PAGINAS";
	case SEMAFORO_NO_EXISTE: return "ERROR_SEMAFORO_NO_INICIALIZADO";
	case GLOBAL_NO_DEFINIDA: return "ERROR_VAR_GLOBAL_NO_DEFINDIDA";
	case ERROR_SIN_DEFINICION: return "ERROR_SIN_DEFINICION";
	case NULL_POINTER: return "NULL_POINTER";
	case DESCONEXION_CPU: return "DESCONEXION_CPU";
	case SEGMENTATION_FAULT: return "SEGMENTATION_FAULT";
	case SIN_ESPACIO_FS: return "SIN_ESPACIO_FS";
	case IMPOSIBLE_BORRAR_ARCHIVO: return "IMPOSIBLE_BORRAR_ARCHIVO";
	case MEMORY_CORRUPTION: return "MEMORY_CORRUPTION";
	default: return "ERROR DESCONOCIDO";
	}
}

void finalizarPrograma(char* comando, char* param){

	if(!esNumero(param)){
		log_warning(logger, "Pid invalido");
		printf("PID invalido\n");
		return;
	}

	int pid = strtol(param, NULL, 10);

	bool buscarProceso(t_proceso* p){
		return p->pid == pid ? true : false;
	}

	t_proceso* proceso = list_find(procesos, buscarProceso);

	if(proceso == NULL){
		log_warning(logger, "El proceso #%d no se encuentra", pid);
		printf("El proceso #%d no se encuentra\n", pid);
		return;
	}

	header_t* header=malloc(sizeof(header_t));
	header->type=FINALIZAR_PROGRAMA;
	header->length= sizeof(pid);
	sendSocket(proceso->socket, header, (void*) &pid);
	free(header);

}

void desconectarConsola(char* comando, char* param) {
	log_debug(logger, "Finalizando conexion threads...");
	log_debug(logger, "Abortando programas (que fueron aceptados)...");
	printf("Finalizando conexion threads...\n");
	printf("Abortando programas...\n");
	void finish(t_proceso* proc){
		header_t header;
		header.type=FINALIZAR_PROGRAMA;
		header.length= sizeof(proc->pid);
		sendSocket(proc->socket, &header, (void*) &proc->pid);
		terminarProceso(proc, DESCONEXION_CONSOLA);
	}
	list_iterate(procesos, finish);
	log_info(logger, "Consola desconectada.");
	exit(0);
}

void terminarProceso(t_proceso* proc, int32_t exitCode){
	cargarFechaFin(proc);
	imprimirInformacion(proc, exitCode);
	bool buscarPorPid(t_proceso* p){
		return p->pid == proc->pid ? true : false;
	}
	list_remove_by_condition(procesos, buscarPorPid);
	pthread_cancel(proc->thread);
	free(proc);
}

void limpiarMensajes(char* comando, char* param) {
	system("clear");
}

int crearLog() {
	logger = log_create("../logConsola","consola", 0, LOG_LEVEL_TRACE);
	if (logger) {
		return 1;
	} else {
		return 0;
	}
}

void imprimirPorPantalla(void* buffer){
	int pid = *(int*) buffer;

	bool buscarProceso(t_proceso* p){
			return p->pid == pid ? true : false;
		}

	t_proceso* proceso = list_find(procesos, buscarProceso);

	proceso->impresiones++;
	char* impresion = buffer + sizeof(int);
	printf("-- IMPRESION PROGRAMA #%d: %s\n", pid, impresion);
}

char* crearPath(char* program_name){
	char* absolute_path = string_new();
	if(string_starts_with(program_name,"/")){
		string_append(&absolute_path,program_name);
		if(!string_ends_with(program_name, ".ansisop")) string_append(&absolute_path,".ansisop");
		return absolute_path;
	}

	string_append(&absolute_path,config->program_path);

	if( !string_ends_with(absolute_path, "/") )
		string_append(&absolute_path,"/");

	string_append(&absolute_path,program_name);
	if(!string_ends_with(program_name, ".ansisop")) string_append(&absolute_path,".ansisop");

	log_debug(logger, "Path absoluto: %s\n",absolute_path);

	return absolute_path;
}
