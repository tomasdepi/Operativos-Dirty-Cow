#include "funcionesMemoria.h"
#include "hash.h"

void testHash(){

	const int cantPids = 15;
	const int cantPags = 20;

	hashInit(500);

	printf("\nLas seeds son: %d %d\n", getSeed1(), getSeed2() );

	int arrayMagico[cantPids*cantPags];
	int i,j;
	for(i=0;i<cantPids;i++){
		printf("\nValores de las primeras 20 paginas del pid: %d\n", i);
		for(j=0;j<cantPags;j++){
			printf("%d ",getPos(i,j));
			arrayMagico[i*cantPags + j] = getPos(i,j);
		}
	}

	if(getPos(i,j) == getPos(i,j))
		printf("\n\nHash es deterministico\n");
	else printf("\n\nHash NO es deterministico\n");

	int cant_colisiones = 0;
	int valor = -1;
	for(i=0;i<cantPids*cantPags;i++){

		for(j=i+1;j<cantPids*cantPags;j++){
			if(arrayMagico[i] == arrayMagico[j]){
				cant_colisiones++;
				arrayMagico[j] = valor;
				valor--;
			}
		}
	}

	printf("\nCantidad de colisiones: %d\n", cant_colisiones);

	exit(0);
}

void inicializarGlobales(){
	pthread_mutex_init(&cache_mutex,NULL);
	pthread_mutex_init(&tablaPag_mutex,NULL);
}

void crearConfig(int argc, char* argv[]){

	if(argc>1){
			if(verificarExistenciaDeArchivo(argv[1])){
				config=levantarConfiguracionMemoria(argv[1]);
				log_info(logger, "Configuracion levantada correctamente");
			}else{
				log_error(logger,"Ruta incorrecta");
				exit(EXIT_FAILURE);
			}
	}
	else if(verificarExistenciaDeArchivo(configuracionMemoria)){
		config=levantarConfiguracionMemoria(configuracionMemoria);
		log_info(logger,"Configuracion levantada correctamente");
	}
	else if(verificarExistenciaDeArchivo(string_substring_from(configuracionMemoria,3))){
		config=levantarConfiguracionMemoria(string_substring_from(configuracionMemoria,3));
		log_info(logger,"Configuracion levantada correctamente");
	}
	else{
		log_error(logger,"No se pudo levantar el archivo de configuracion");
		exit(EXIT_FAILURE);
	}
}

t_config_memoria* levantarConfiguracionMemoria(char* archivo) {

        t_config_memoria* config = malloc(sizeof(t_config_memoria));
        t_config* configMemoria;

        configMemoria = config_create(archivo);

        config->puerto = malloc(MAX_LEN_PUERTO);
        strcpy(config->puerto, config_get_string_value(configMemoria, "PUERTO"));

        config->ip_memoria = malloc(MAX_LEN_IP);
		strcpy(config->ip_memoria, config_get_string_value(configMemoria, "IP_MEMORIA"));

        config->marcos = config_get_int_value(configMemoria, "MARCOS");
        config->marcos_Size = config_get_int_value(configMemoria, "MARCOS_SIZE");
        config->entradas_Cache = config_get_int_value(configMemoria, "ENTRADAS_CACHE");
        config->cache_x_Proceso = config_get_int_value(configMemoria, "CACHE_X_PROC");

        config->reemplazo_cache = malloc(MAX_LEN_PUERTO);
        strcpy(config->reemplazo_cache, config_get_string_value(configMemoria, "REEMPLAZO_CACHE"));

        config->retardo_Memoria = config_get_int_value(configMemoria, "RETARDO_MEMORIA");

        if( config_get_int_value(configMemoria,"RETARDO_ACTIVADO") == 1 )
        	config->retardoActivado = true;
        else config->retardoActivado = false;

        if( config_get_int_value(configMemoria,"IMPRIMIR_RETARDO") == 1)
        	config->imprimirRetardo = true;
        else config->imprimirRetardo = false;

        config_destroy(configMemoria);
        return config;

}

void destruirConfiguracionMemoria(t_config_memoria* config){
	free(config->puerto);
	free(config->reemplazo_cache);
	free(config);
}

void inicializarMemoria(){

	const int memSize = config->marcos * config->marcos_Size;
	int i;
	bool noEspacioCache = false;

	//Creo el bloque de memoria principal
	memoria = malloc(memSize);
	if(memoria == NULL){
			log_error(logger, "No pude reservar memoria para mp");
			exit(EXIT_FAILURE);
	}

	//Creo la cache
	hayCache = config->entradas_Cache != 0;
	if(hayCache){
		cache = malloc(sizeof(t_entrada_cache) * config->entradas_Cache);

		//Inicializo la cache
		for(i=0;i<config->entradas_Cache;i++){
			cache[i].pid = -1;
			cache[i].pag = -1;
			cache[i].content = malloc(frame_size);
			cache[i].time_used = 0;
			if(cache[i].content == NULL) noEspacioCache = true;
		}

		//Reviso los mallocs
		if(cache == NULL || noEspacioCache){
			log_error(logger, "No pude reservar memoria para cache");
			exit(EXIT_FAILURE);
		}
	}

	//Setteo la memoria con \0
	memset(memoria,'\0',memSize);

	//Creo las entradas de la tabla invertida
	for(i=0;i<config->marcos;i++){
		((t_entrada_tabla*)memoria)[i].pag = -1;
		((t_entrada_tabla*)memoria)[i].pid = -1;
	}

	int cantEntradas = (sizeof(t_entrada_tabla) * config->marcos) / config->marcos_Size;
	if( ((sizeof(t_entrada_tabla) * config->marcos) % config->marcos_Size) > 0)
		cantEntradas++;

	printf("\n--------------------\ncantidad de frames ocupados por tabla: %i\n", cantEntradas);

	//Setteo las paginas ocupadas por la tabla como ya usadas
	for(i=0;i<cantEntradas;i++){
		((t_entrada_tabla*)memoria)[i].pag = 0;
		((t_entrada_tabla*)memoria)[i].pid = 0;
	}

	printf("cant entradas cache: %d\nmax entradas: %d\n",cache_entradas,max_entradas);

	//Inicializo la funcion de hash
	hashInit(cant_frames);

	/*Imprimo el contenido de la memoria en un archivo dump
	FILE* memFile = fopen("memDump","w");
	for(i=0;i<memSize;i++){
		fputc(memoria[i],memFile);
	}
	fclose(memFile); ESTO SE DEBERIA BORRAR UNA VEZ QUE NOS ASEGUREMOS QUE FUNCIONA 10puntos*/

}

void requestHandlerKernel(int fd){

	void* paquete = NULL;
	int bytes;
	int tipo_mensaje;

	for(;;){
		bytes = recibir_paquete(fd, &paquete, &tipo_mensaje);
		if(bytes <= 0){
			log_error(logger, "Desconexion del kernel. Terminando...");
			close(fd);
			exit(1);
		}

		switch(tipo_mensaje){
		case INICIAR_PROGRAMA:
			iniciarPrograma(fd, (t_pedido_iniciar*)paquete);
			break;

		case FINALIZAR_PROGRAMA:
			finalizarPrograma((t_pedido_finalizar*)paquete);
			break;

		case ASIGNAR_PAGINAS:
			asignarPaginas(fd, (t_pedido_asignar*)paquete);
			break;

		case LIBERAR_PAGINA:
			liberarPagina(fd, (t_pedido_liberar*)paquete);
			break;

		case SOLICITUD_BYTES:
			solicitudBytes(fd, (t_pedido_memoria*)paquete);
			break;

		case GRABAR_BYTES:
			grabarBytes(fd, paquete);
			break;

		default:
			log_warning(logger, "Mensaje Recibido Incorrecto cod: %d", tipo_mensaje);
		}

		free(paquete);
		paquete = NULL;
	}
}

void requestHandlerCpu(int fd){

	void* paquete;
	int tipo_mensaje;
	int bytes;

	for(;;){
		//Recibo mensajes de cpu y hago el switch
		bytes = recibir_paquete(fd, &paquete, &tipo_mensaje);
		if(bytes <= 0){
			log_info(logger, "Desconexion de cpu con socket %d", fd);
			close(fd);
			return;
		}else{

			switch(tipo_mensaje){
				case SOLICITUD_BYTES:
					solicitudBytes(fd, (t_pedido_memoria*)paquete);
					break;

				case GRABAR_BYTES:
					grabarBytes(fd, paquete);
					break;
				case OK:
					break;
				default:
					log_warning(logger, "Mensaje Recibido Incorrecto");
			}
		}

		free(paquete);
		paquete = NULL;
	}
}

int iniciarPrograma(int fd, t_pedido_iniciar* pedido){

	log_info(logger, "Iniciar Programa. pid: %d cant_pag: %d.",pedido->pid,pedido->cant_pag);

	if( reservarFrames(pedido->pid,pedido->cant_pag) == -1){
		//No se puede, aviso a kernel que no hay lugar
		enviarRespuesta(fd, SIN_ESPACIO);
		log_warning(logger, "No hay espacio");
		return -1;
	}

	//Envio el ok al kernel y espero el resto del codigo
	enviarRespuesta(fd, OP_OK);

/*	char* codigo = NULL;
	int tipo;

	if( recibir_paquete(fd, &codigo, &tipo) <= 0 ){
		log_error(logger, "Error al recibir el codigo");
		free(codigo);
		return -1;
	}

	//Recibi el codigo ok. Copio el codigo en las paginas correspondientes
	int i;
	for(i=0;i<pedido->cant_pag;i++){
		memcpy(memoria + buscarFrame(pedido->pid,i)*frame_size,
				codigo + i*frame_size,
				frame_size);
	}
*/
	return 0;
}

int finalizarPrograma(t_pedido_finalizar* pid){

	log_info(logger,"Pedido finalizar. pid: %d",*pid);

	pthread_mutex_lock(&tablaPag_mutex);
	esperar();//Accedo a tabla pag
	int i;
	for(i=0;i<cant_frames;i++){
		if(tabla_pag[i].pid == *pid){
			tabla_pag[i].pid = -1;
			tabla_pag[i].pag = -1;
		}
	}
	pthread_mutex_unlock(&tablaPag_mutex);

	return 0;
}

int asignarPaginas(int fd, t_pedido_asignar* pedido){

	log_info(logger,"Pedido reservar frames. Pid: %d cant_frames: %d",pedido->pid,pedido->cant_pag);

	if( reservarFrames(pedido->pid,pedido->cant_pag) == -1){
		//No se puede, aviso a kernel que no hay lugar
		enviarRespuesta(fd, SIN_ESPACIO);
		log_warning(logger, "No hay espacio");
		return -1;
	}

	//Se pudo reservar
	enviarRespuesta(fd,OP_OK);

	return 0;
}

int liberarPagina(int fd, t_pedido_liberar* pedido){

	log_info(logger, "Pedido liberar pid: %d pag: %d",pedido->pid,pedido->nroPag);

	if( liberarFrame(pedido->pid,pedido->nroPag) == -1 ){
		log_error(logger, "No existe la pagina pedida.");
		enviarRespuesta(fd, QUILOMBO);
		return -1;
	}

	enviarRespuesta(fd,OP_OK);
	return 0;
}

/* Asigna la cantidad de paginas al proceso especifico
 * Devuelve 0 en caso de poder asignar las paginas
 * -1 en caso de no haber frames disponibles para realizar la operacion */
int reservarFrames(int pid, int cantPag){

	if(framesLibres() < cantPag)
		return -1;

	pthread_mutex_lock(&tablaPag_mutex);

	int maxPag = -1;
	int i;

	esperar();//Accedo a tabla de paginas->memoria

	//Saco la max pagina de un frame
	for(i=0;i<cant_frames;i++){
		if(tabla_pag[i].pid == pid && tabla_pag[i].pag > maxPag)
			maxPag = tabla_pag[i].pag;
	}

	maxPag++;

	//Asigno las paginas que me piden
	int asignadas = 0;
	int posInicial;
	bool asigne;
	while(asignadas < cantPag){

		posInicial = getPos(pid,maxPag);
		asigne = false;

		for(i=0;!asigne && i<cant_frames; i++){
			if(tabla_pag[posInicial + i].pid == -1 || tabla_pag[posInicial + i].pag == -1){
				tabla_pag[posInicial + i].pid = pid;
				tabla_pag[posInicial + i].pag = maxPag;
				maxPag++;
				asignadas++;
				asigne = true;
			}
		}

		if(asigne == false){
			log_error(logger,"Error al reservar frames");
			return -1;
		}
	}

	pthread_mutex_unlock(&tablaPag_mutex);
	return 0;
}

int liberarFrame(int pid, int nroPag){

	int frame = buscarFrame(pid,nroPag);

	if(frame == -1){
		log_error(logger,"No existe la pagina pid: %d pag: %d\n",pid,nroPag);
		return -1;
	}

	//Cambio los valores de la tabla de paginas
	pthread_mutex_lock(&tablaPag_mutex);
	esperar();//Acceso a tabla pag
	tabla_pag[frame].pag = -1;
	tabla_pag[frame].pid = -1;
	pthread_mutex_unlock(&tablaPag_mutex); //TODO: REVISAR ESTO!!!

	return 0;
}

int solicitudBytes(int fd, t_pedido_memoria* pedido){

	log_info("Pedido lectura. pid: %d pag: %d offset: %d size: %d",
				pedido->pid,pedido->pag,pedido->offset,pedido->size);

	if(pedidoIncorrecto(pedido)){
		enviarRespuesta(fd, SEGMENTATION_FAULT);
		return -1;
	}
	char* buf = malloc(pedido->size);

	if(buf == NULL){
		log_error(logger,"no pude reservar memoria");
		enviarRespuesta(fd, QUILOMBO);
		return -1;
	}

	if( leer(pedido->pid,pedido->pag,pedido->offset,pedido->size,buf) == 0 ){
		header_t header;
		header.type = RESPUESTA_BYTES;
		header.length = pedido->size;

		if( sendSocket(fd, &header, buf) <= 0 ){
			log_error(logger,"error al enviar respuesta");
			enviarRespuesta(fd, SEGMENTATION_FAULT);
			free(buf);
			return -1;
		}
	}else{
		log_error(logger,"No se pudo leer de memoria");
		enviarRespuesta(fd, SEGMENTATION_FAULT);
		free(buf);
		return -1;
	}

	free(buf);
	return 0;
}

int grabarBytes(int fd, char* paquete){

	t_pedido_memoria* pedido = paquete;
	char* buf = paquete + sizeof(t_pedido_memoria);

	if(pedidoIncorrecto(pedido)){
		enviarRespuesta(fd, SEGMENTATION_FAULT);
		return -1;
	}

/*	char* buf = malloc(pedido->size);

	if(buf == NULL){
		log_error(logger,"no pude reservar memoria");
		enviarRespuesta(fd, QUILOMBO);
		return -1;
	}

	if( recvAll(fd,buf,pedido->size,0) <= 0 ){
		log_error(logger, "Error al recibir info a escribir");
		free(buf);
		enviarRespuesta(fd, SEGMENTATION_FAULT);
		return -1;
	}*/

	if( escribir(pedido->pid,pedido->pag,pedido->offset,buf,pedido->size) == -1){
		log_error(logger, "Error al intentar escribir en memoria");
		enviarRespuesta(fd, SEGMENTATION_FAULT);
		return -1;
	}

	enviarRespuesta(fd, OP_OK);
	return 0;
}

void enviarRespuesta(int fd, int respuesta){

	header_t header;
	header.type = respuesta;
	header.length = 0;
	sendSocket(fd, &header, &header);
}

bool pedidoIncorrecto(t_pedido_memoria* pedido){
	return pedido->offset > frame_size;
}

int framesLibres(){

	pthread_mutex_lock(&tablaPag_mutex);

	int i, cant = 0;
	for(i=0;i<cant_frames;i++){
		if( tabla_pag[i].pid == -1 &&
			tabla_pag[i].pag == -1  )
			cant++;
	}

	pthread_mutex_unlock(&tablaPag_mutex);
	return cant;
}

/* Busqueda secuencial, despues implementamos hash */
int buscarFrame(int pid, int pag){

	esperar();

	pthread_mutex_lock(&tablaPag_mutex);

	int posInicial = getPos(pid,pag);
	int i;

	int iterador(){
		return (posInicial + i) % cant_frames;
	}

	for(i=0;i<cant_frames;i++){
		if( tabla_pag[iterador()].pid == pid &&
			tabla_pag[iterador()].pag == pag	){
			pthread_mutex_unlock(&tablaPag_mutex);
			return iterador();
		}
	}

/*	int i;
	for(i=0;i<cant_frames;i++){
		if( tabla_pag[i].pid == pid &&
			tabla_pag[i].pag == pag  ){
			pthread_mutex_unlock(&tablaPag_mutex);
			printf("ENCONTRE!!\n");
			return i;
		}
	}*/

	pthread_mutex_unlock(&tablaPag_mutex);
	printf("NO ENCONTRE :(\n");
	return -1; //No encontro en la tabla de paginas la entrada
}

/* Esta funcion necesita que le pasen en resultado, un puntero con al menos
 * al menos size bytes reservados. El invocador debe hacer el free despues de utilizado.
 */
int leer(int pid, int pag, int offset, int size, char* resultado){

	log_info(logger,"Leer pid: %d pag:%d offset: %d size: %d",pid,pag,offset,size);

	int frame;
	int cant_leida = 0;
	int cant_a_leer;
	char* pos_leer;

	while(cant_leida < size){
		if( leerCache(pid,pag,&pos_leer) == -1 ){//No Esta en cache, debo leer de memoria

			//Si no esta en cache, debo leer de memoria, 1 acceso mas
			esperar();

			frame = buscarFrame(pid,pag);
			if(frame == -1)
				return -1;

			pos_leer = memoria + frame * frame_size;

			actualizarEntradaCache(pid, pag, pos_leer);
		}/* Al salir de este if pos_leer apunta o bien al frame de donde tengo que leer,
		  * o a donde esta cacheado el frame */

		//log_info(logger,"Antes de leer. cant_leida: %d, size: %d, frame_size: %d, offset: %d",
		//		cant_leida,size,frame_size,offset);

		//Me fijo cuanto tengo que leer y copio lo que esta en memoria/cache en resultado
		cant_a_leer = min(size - cant_leida, frame_size - offset);
		memcpy(resultado + cant_leida, pos_leer + offset, cant_a_leer);

		cant_leida += cant_a_leer;
		offset = 0;
		pag++;
	}

	return 0;
}

int escribir(int pid, int pag, int offset, char* contenido, int size){
	log_info(logger, "Escribir pid->%d - pag->%d - offset->%d - size->%d", pid, pag, offset, size);
	int frame;
	int cant_escrita = 0;
	int cant_a_escribir;

	while(cant_escrita < size){

		frame = buscarFrame(pid,pag);
		if(frame == -1)
			return -1;

		//Cada frame escrito es un acceso
		esperar();

		cant_a_escribir = min(size - cant_escrita, frame_size - offset);
		memcpy(memoria + frame * frame_size + offset, contenido + cant_escrita, cant_a_escribir);

		actualizarEntradaCache(pid, pag, memoria + frame * frame_size);

		cant_escrita += cant_a_escribir;
		offset = 0;
		pag++;
	}

	return 0;
}

//Funciones Cache
void increaseOpCount(){
	op_count++;
}
int cantEntradas(int pid){
	int i, cant = 0;
	for(i=0;i<cache_entradas;i++){
		if(cache[i].pid == pid)
			cant++;
	}

	return cant;
}

/* Busca la entrada de cache que coincida con pid y pag
 * Retorna -1 si no existe */
int buscarEntrada(int pid, int pag){
	int i;
	for(i=0;i<cache_entradas;i++){
		if(cache[i].pid == pid && cache[i].pag == pag){
			return i;
		}
	}
	return -1;
}

int entradaAReemplazar(int pid){

	if( cantEntradas(pid) == max_entradas ){
		return reemplazoLocal(pid);
	}else return reemplazoGlobal();
}

int reemplazoLocal(int pid){
	log_debug(logger,"Reemplazo local");
	int i;
	int entrada; //A reemplazar
	int minTime = ULONG_MAX;

	for(i=0;i<cache_entradas;i++){
		if( cache[i].time_used < minTime && cache[i].pid == pid){
			entrada = i;
			minTime = cache[i].time_used;
		}
	}

	return entrada;
}

int reemplazoGlobal(){
	log_debug(logger,"Reemplazo global");
	//Recorro buscando una entrada libre
	int i;
	for(i=0;i<cache_entradas;i++){
		if(cache[i].pid == -1 || cache[i].pag == -1){
			return i;
		}
	}

	//Sali del for => no hay entrada libre. Debo reemplazar
	int entrada; //A reemplazar
	int minTime = ULONG_MAX;

	for(i=0;i<cache_entradas;i++){
		if( cache[i].time_used < minTime){
			entrada = i;
			minTime = cache[i].time_used;
		}
	}

	return entrada;
}

int leerCache(int pid, int pag, char** contenido){
	if(!hayCache) return -1;
	pthread_mutex_lock(&cache_mutex);

	increaseOpCount();

	int i;
	for(i=0;i<cache_entradas;i++){//Si alguna entrada coincide, pongo el valor de content en contenido
		if(cache[i].pid == pid && cache[i].pag == pag){
			*contenido = cache[i].content;
			cache[i].time_used = op_count;
			pthread_mutex_unlock(&cache_mutex);
			log_info(logger, "CACHE HIT.",pid,pag);
			return 0;
		}
	}
	pthread_mutex_unlock(&cache_mutex);
	log_info(logger, "CACHE MISS",pid,pag);
	return -1;
}

void actualizarEntradaCache(int pid, int pag, char* frame){

	if(!hayCache)
		return;

	log_info(logger,"Actualizar entrada cache. pid: %d pag: %d", pid,pag);

	pthread_mutex_lock(&cache_mutex);

	increaseOpCount();

	int entrada = buscarEntrada(pid, pag);

	if(entrada != -1){
		memcpy(cache[entrada].content,frame,frame_size);
	}else{ //tengo que reemplazar una entrada
		entrada = entradaAReemplazar(pid);

		log_info(logger,"Reemplazo entrada n° %d con pid %d pag %d",entrada,pid,pag);

		cache[entrada].pid = pid;
		cache[entrada].pag = pag;
		memcpy(cache[entrada].content,frame,frame_size);
	}

	cache[entrada].time_used = op_count;

	pthread_mutex_unlock(&cache_mutex);
}

//funciones interfaz
void levantarInterfaz(){
	//creo los comandos y el parametro
	comando* comandos = malloc(sizeof(comando)*5);

	strcpy(comandos[0].comando, "retardo");
	comandos[0].funcion = retardo;
	strcpy(comandos[1].comando, "dump");
	comandos[1].funcion = dump;
	strcpy(comandos[2].comando, "flush");
	comandos[2].funcion = flush;
	strcpy(comandos[3].comando, "size");
	comandos[3].funcion = size;
	strcpy(comandos[4].comando, "help");
	comandos[4].funcion = help;

	interface_thread_param* params = malloc(sizeof(interface_thread_param));
	params->comandos = comandos;
	params->cantComandos = 5;

	//Lanzo el thread
	pthread_t threadInterfaz;
	pthread_attr_t atributos;
	pthread_attr_init(&atributos);
	pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

	pthread_create(&threadInterfaz, &atributos, (void*)interface, params);

	return;
}
void retardo(char* comando, char* param){

	int retardo = atoi(param);
	config->retardo_Memoria = retardo;

    log_info(logger,"Cambio retardo exitoso\n");
}
void dump(char* comando, char* param){
	//printf("dump\n");

	if(strlen(param) == 0){
		//Dump de todas las estructuras
		dumpAll();
		return;
	}

	if( !strcmp(param, "cache") ){
		//Dump de cache
		dumpCache();
		return;
	}

	if( !strcmp(param,"table") ){
		//Dump de tabla de paginas
		dumpTable();
		return;
	}

	//Creo un buffer para checkear con memory
	char buf[7];
	memcpy(buf,param,6);
	buf[6] = 0;

	//Miro, si arranca con memory
	if( !strcmp(buf, "memory") ){
		//Ahora miro si hay un parametro numerico

		if(param[6] == '-'){
			//compruebo si hay numero
			if(atoi(param + 7) == 0){
				printf("Ingrese un valor valido para el numero de proceso\n");
				return;
			}else
			dumpMemory( atoi(param + 7) );
		}else dumpMemory(-1);
	}else{
		printf("\nParametro invalido.\nPara dump los parametros validos son: \n+sin param\n+cache\n+tabla\n+memory\n+memory-\"nro_proceso\"\n");
	}

}
void dumpAll(){
	dumpCache();
	dumpMemory(-1);
	dumpTable();
}
void dumpCache(){

	FILE* dumpFile = fopen("cacheDump","a");

	if(dumpFile == NULL){
		log_error(logger, "No se pudo abrir el archivo de dump");
		return;
	}

	//Escribo el header del dump
	fprintf(dumpFile,"\n----Dump de cache: %s----\n",getTimeStamp());
	fprintf(dumpFile,"Tiempo actual: %lu\n", op_count);

	//Escribo el contenido de cada entrada
	int i,j;
	for(i=0;i<cache_entradas;i++){
		fprintf(dumpFile,"entrada n°: %i, pid: %i, pag: %i, time_used: %lu\ncontenido: ",
				i,cache[i].pid,cache[i].pag,cache[i].time_used);

		for(j=0;j<frame_size;j++)
			fputc(cache[i].content[j],dumpFile);
		fputc('\n',dumpFile);
	}

	fclose(dumpFile);

	printf("Dump cache exitoso.\n");
	return;
}
void dumpTable(){

	FILE* dumpFile = fopen("tableDump","a");

	if(dumpFile == NULL){
		log_error(logger, "No se pudo abrir el archivo de dump");
		return;
	}

	//Escribo el header del dump
	fprintf(dumpFile,"\n----Dump de tabla: %s----\n",getTimeStamp());

	int i;
	for(i=0;i<cant_frames;i++){
		fprintf(dumpFile,"Entrada n°: %i, pid: %i, pag: %i\n",
				i,tabla_pag[i].pid,tabla_pag[i].pag);
	}

	fclose(dumpFile);
	printf("dump tabla pag exitoso.\n");
	return;
}
void dumpMemory(int pid){

	//printf("dump Memory pid: %i\n", pid);

	FILE* dumpFile = fopen("memoryDump","a");

	if(dumpFile == NULL){
		log_error(logger, "No se pudo abrir el archivo de dump");
		return;
	}

	//Escribo el header del dump
	fprintf(dumpFile,"\n----Dump de memoria: %s----\n",getTimeStamp());

	//Me fijo si tengo que imprimir todos los procesos o solo 1
	bool todosLosProcesos;
	if(pid==-1){
		fprintf(dumpFile,"Dump de todos los procesos.\n");
		todosLosProcesos = true;
	}else{
		fprintf(dumpFile,"Dump del proceso pid=%i.\n",pid);
		todosLosProcesos = false;
	}

	int i,j;
	for(i=0;i<cant_frames;i++){

		if(tabla_pag[i].pid == pid || todosLosProcesos){
			fprintf(dumpFile,"Frame n°: %i, pid: %i, pag: %i, contenido:\n",
					i,tabla_pag[i].pid,tabla_pag[i].pid);

			for(j=0;j<frame_size;j++){
				fputc(memoria[i * frame_size + j], dumpFile);
			}
			fputc('\n',dumpFile);
		}
	}

	fclose(dumpFile);
	printf("dump memoria exitoso.\n");
	return;
}
void flush(char* comando, char* param){

	int i;
    for(i=0;i<cache_entradas;i++){
      	cache[i].pag = -1;
       	cache[i].pid = -1;
       	cache[i].time_used = 0;
    }

    printf("Flush exitoso.\n");

    return;
}
void size(char* comando, char* param){

	if( !strcmp(param,"memory") ){
		int free = framesLibres();

		printf("Size memory:\nCant frames: %d (%d bytes), framesOcupados: %d (%d bytes), framesLibres: %d (%d bytes).\n",
				cant_frames,cant_frames * frame_size,
				cant_frames - free, (cant_frames - free) * frame_size,
				free, free * frame_size);
		return;
	}

	int pid = atoi(param);
	if(pid<=0)
		pid = -1;

	if( !strcmp(param,"table") )
		pid = 0;

	int i, cant = 0;
	for(i=0;i<cant_frames;i++){
		if( tabla_pag[i].pid == pid  )
			cant++;
	}

	if(pid == -1){
		printf("Cantidad de marcos libres: %d (%d bytes)\n", cant, cant * frame_size);
	}else if(pid == 0){
		printf("Cantidad de marcos de la tabla de paginas: %d (%d bytes)\n",cant,cant*frame_size);
	}else{
		printf("Cantidad de marcos ocupados por el proceso n° %d: %d (%d bytes)\n", pid, cant, cant * frame_size);
	}
	return;
}

void help(char* comando, char* param){

	printf("\n\nComandos Soportados:\n+retardo \"milis\" -> Cambia el retardo\n+flush -> Limpia la cache\n");
	printf("+size -> Cantidad de marcos libres\n+size table -> Cantidad de marcos tabla de paginas\n");
	printf("+size memory -> Tamaño de la memoria\n+size \"pid\" -> Tamaño del proceso\n");
	printf("+dump -> Dump de todas las estructuras\n+dump cache -> Dump de la cache\n");
	printf("+dump memory -> Dump de las paginas de todos los procesos y su contenido\n");
	printf("+dump memory-pid -> Dump de las paginas de un proceso\n+dump table -> Dump de la tabla de paginas\n");
}

char* getTimeStamp(){
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char* string = asctime(timeinfo);
	string[strlen(string) - 1] = '\0'; //saco el \n del string

	return string;
}

void esperar(){

	if( config->retardoActivado ){
		//printf("retardo %d\n\n",config->retardo_Memoria);
		usleep(config->retardo_Memoria * 1000);

		if( config->imprimirRetardo )
			log_debug(logger,"RETARDO");
	}
}
