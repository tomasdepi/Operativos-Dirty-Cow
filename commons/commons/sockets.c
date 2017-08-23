/*
 * sockets.c
 *
 *  Created on: 4/4/2017
 *      Author: utnso
 */

#include "sockets.h"

/**
 * @NAME: getSocket
 * @DESC: Crea un socket y retorna su descriptor. Retorna -1 en caso de error.
 */
int getSocket(void){
	int yes = 1;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		close(sockfd);
		return -1;
	}

	return sockfd;
}
int createServer2(char *addr, char *port, int backlog) {
	int puerto = atoi(port);
	struct sockaddr_in socketInfo;
		int socketEscucha;
		int optval = 1;

		// Crear un socket
		socketEscucha = socket (AF_INET, SOCK_STREAM, 0);
		if (socketEscucha == -1)
		 	return -1;

		setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &optval,
				sizeof(optval));
		socketInfo.sin_family = AF_INET;
		socketInfo.sin_port = htons(puerto);
		socketInfo.sin_addr.s_addr = INADDR_ANY;
		if (bind (socketEscucha,(struct sockaddr *)&socketInfo,sizeof (socketInfo)) != 0)
		{
			close (socketEscucha);
			return -1;
		}

		/*
		* Se avisa al sistema que comience a atender llamadas de clientes
		*/
		if (listen (socketEscucha, 10) == -1)
		{
			close (socketEscucha);
			return -1;
		}
		/*
		* Se devuelve el descriptor del socket servidor
		*/
		return socketEscucha;
}
/**
 * @NAME: bindSocket
 * @DESC: Bindea el socket a la dirección ip y puerto pasados por parámetro.
 * Retorna -1 en caso de error.
 */
int bindSocket(int sockfd, char *addr, char *port) {
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(port));
	my_addr.sin_addr.s_addr = inet_addr(addr);
	memset(&(my_addr.sin_zero), '\0', 8);

	return bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));
}

/**
 * @NAME: listenSocket
 * @DESC: Pone a la escucha el socket pasado por parámetro con el backlog pasado por parámetro.
 * Retorna -1 en caso de error.
 */
int listenSocket(int sockfd, int backlog) {
	return listen(sockfd, backlog);
}

/**
 * @NAME: acceptSocket
 * @DESC: Acepta una conexion al socket enviado por parámetro y retorna el nuevo descriptor de socket.
 * Retorna -1 en caso de error.
 */
int acceptSocket(int sockfd) {
	socklen_t longitudCliente;//esta variable tiene inicialmente el tamaño de la estructura cliente que se le pase
		struct sockaddr cliente;
		int socketNuevaConexion;//esta variable va a tener la descripcion del nuevo socket que estaria creando
		longitudCliente = sizeof(cliente);
		socketNuevaConexion = accept (sockfd, &cliente, &longitudCliente);//acepto la conexion del cliente
		if (socketNuevaConexion < 0)
			return -1;

		return socketNuevaConexion;
}

/**
 * @NAME: connectSocket
 * @DESC: Conecta el socket a la dirección y puerto pasados por parámetro.
 * Retorna -1 en caso de error.
 */
int connectSocket(int sockfd, char *addr, char *port) {
	struct sockaddr_in their_addr;

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(atoi(port));
	their_addr.sin_addr.s_addr = inet_addr(addr);
	memset(&(their_addr.sin_zero), '\0', 8);

	return connect(sockfd, (struct sockaddr*) &their_addr,
			sizeof(struct sockaddr));
}

/**
 * @NAME: sendSocket
 * @DESC: Serializa el header y el string de datos y lo envía al scoket pasado por parámetro.
 * Retorna la cantidad de bytes enviados o -1 en caso de error.
 */
int sendSocket(int sockfd, header_t *header, void *data) {
	int bytesEnviados, offset = 0, tmp_len = 0;
	void *packet = malloc(sizeof(header_t) + header->length);

	memcpy(packet, &header->type, tmp_len = sizeof(int8_t));
	offset = tmp_len;

	memcpy(packet + offset, &header->length, tmp_len = sizeof(header->length));
	offset += tmp_len;

	memcpy(packet + offset, data, tmp_len = header->length);
	offset += tmp_len;

	bytesEnviados = sendallSocket(sockfd, packet, offset);//El offset representa la cantidad total de bytes del paquete a enviar
	free(packet);

	
	return bytesEnviados;
}

/**
 * @NAME: createServer
 * @DESC: Crea y devuelve un socket para ser utilizado como servidor.
 * En caso de error retorna -1.
 */
int createServer(char *addr, char *port, int backlog) {
	int puerto = atoi(port);
	struct sockaddr_in socketInfo;
		int socketEscucha;
		int optval = 1;

		// Crear un socket
		socketEscucha = socket (AF_INET, SOCK_STREAM, 0);
		if (socketEscucha == -1)
		 	return -1;

		setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &optval,
				sizeof(optval));
		socketInfo.sin_family = AF_INET;
		socketInfo.sin_port = htons(puerto);
		socketInfo.sin_addr.s_addr = INADDR_ANY;
		if (bind (socketEscucha,(struct sockaddr *)&socketInfo,sizeof (socketInfo)) != 0)
		{
			close (socketEscucha);
			return -1;
		}

		/*
		* Se avisa al sistema que comience a atender llamadas de clientes
		*/
		if (listen (socketEscucha, 10) == -1)
		{
			close (socketEscucha);
			return -1;
		}
		/*
		* Se devuelve el descriptor del socket servidor
		*/
		return socketEscucha;
}

/**
 * @NAME: finalizarConexion
 * @DESC: Finaliza la conexion de un socket.
 */
int finalizarConexion(int socket) {
	close(socket);
	return 0;
}


/**
 * @NAME: createClient
 * @DESC: Crea y devuelve un socket para ser utilizado como cliente.
 * En caso de error retorna -1.
 */
int createClient(char *addr, char *port) {
	int sockfd = getSocket();

	if (connectSocket(sockfd, addr, port) == -1) {
		perror("connect");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

/**
 * @NAME: sendallSocket
 * @DESC: procura enviar to-do el paquete entero. Devuelve la cantidad de bytes enviados o -1 en caso de error.
 *
 */
int sendallSocket(int socket, void* paquete_a_enviar, int tamanio_paquete) {

	int total = 0;
	int bytesleft = tamanio_paquete;
	int n;

	while(total < tamanio_paquete) {
		n = send(socket, paquete_a_enviar+total, bytesleft, 0);
		if(n == -1) {break; }
		total += n;
		bytesleft -= n;
	}

	return n==-1?-1:total;

}

/*
 * @NAME: recibir_paquete
 *
 * @DESC: Recibe un paquete por el socket que le llega por parametro, devuelve la cantidad de bytes recibidos, 0 si se cerro el socket o -1 si hubo algun error.
 *
 */
int recibir_paquete(int socket, void** paquete, int* tipo){

	int bytes_recibidos;
		header_t cabecera;

		bytes_recibidos = recvAll(socket, (char*)&cabecera, sizeof(header_t), MSG_WAITALL);

		if(bytes_recibidos == 0) return 0;
		if(bytes_recibidos == -1) return -1;

		*tipo=cabecera.type;

		if(cabecera.length){
			*paquete = malloc(cabecera.length);
			bytes_recibidos = recvAll(socket, (char*)*paquete, cabecera.length, MSG_WAITALL);
			if(bytes_recibidos == 0) return 0;
			if(bytes_recibidos == -1) return -1;
		}
		else
			*paquete=NULL;

		return bytes_recibidos;
}

/*
 * @NAME: recibir_string
 *
 * @DESC: Recibe un paquete que contiene un string, utiliza la funcion recibir_paquete y le agrega el caracter nulo (\0) al final.
 */
int recibir_string(int socket,void** puntero_buffer,int* tipo){
	int tamanio;

	if((tamanio=recibir_paquete(socket,puntero_buffer,tipo))>0){
		agregar_caracter_nulo(*puntero_buffer, tamanio);
		return tamanio+1;
	}
	return tamanio;
}

/*
 * @NAME: agregar_caracter_nulo
 *
 * @DESC: Agrega el caracter nulo (\0) al final de un stream de bytes recibido.
 */
int agregar_caracter_nulo(void* stream, int tamanio){

	void* otro_puntero;

	otro_puntero=stream;
	otro_puntero=realloc(otro_puntero,tamanio+1);
	char aux='\0';
	memcpy(otro_puntero+tamanio,&aux,1);
	stream=otro_puntero;

	return tamanio+1;

}

/*
 * @NAME: deserializar_string
 *
 * @DESC: Recibe un puntero a un stream de bytes, deserializa un string y devuelve la cantidad total de bytes leidos del stream.
 */
int deserializar_string(void* paquete, char** string){

	int bytes_totales_leidos = 0;
	int bytes_string;
	void* puntero_buffer = paquete;

	memcpy(&bytes_string, puntero_buffer, sizeof(int));

	puntero_buffer += sizeof(int);

	string = malloc(bytes_string);

	memcpy(*string, puntero_buffer, bytes_string);

	agregar_caracter_nulo(*string, bytes_string);

	bytes_totales_leidos = sizeof(int) + bytes_string;

	return bytes_totales_leidos;

}

t_buffer_tamanio* serializarIndiceStack(t_list* indiceStack) {
	uint16_t a,p;
	uint32_t tamanioTotalBuffer;
	t_list* tamanioStackStack=list_create();
	uint32_t tamanioStackParticular;
	tamanioTotalBuffer=0;
	uint32_t pos;

	for(a=0; a<list_size(indiceStack); a++){
		t_entrada_stack* linea=list_get(indiceStack, a);
		tamanioStackParticular=0;

		uint16_t cantidadArgumentos=list_size(linea->argumentos);
		tamanioTotalBuffer+=cantidadArgumentos*sizeof(t_argumento);
		tamanioStackParticular+=cantidadArgumentos*sizeof(t_argumento);

		uint16_t cantidadVariables=list_size(linea->variables);
		tamanioTotalBuffer+=cantidadVariables*sizeof(t_var);
		tamanioStackParticular+=cantidadVariables*sizeof(t_var);

		tamanioTotalBuffer+=sizeof(uint32_t);
		tamanioStackParticular+=sizeof(uint32_t);
		if(linea->retVar){
			tamanioTotalBuffer+=sizeof(t_posicion);
			tamanioStackParticular+=sizeof(t_posicion);
		}
		tamanioTotalBuffer+=sizeof(uint32_t)*4;
		t_tamanio_stack *auxiliar=malloc(sizeof(t_tamanio_stack));
		auxiliar->stack=linea;
		auxiliar->tamanioStack=tamanioStackParticular;
		list_add(tamanioStackStack, auxiliar);
	}

	char* buffer=malloc(tamanioTotalBuffer + sizeof(uint32_t));
	int tamanioUint32=sizeof(uint32_t),offset=0;
	uint32_t cantidadItemsEnStack=list_size(indiceStack);
	memcpy(buffer + offset, &cantidadItemsEnStack, tamanioUint32);
	offset+=tamanioUint32;

	for(a=0; a<list_size(tamanioStackStack); a++){
		t_tamanio_stack* linea=list_get(tamanioStackStack, a);
		memcpy(buffer + offset, &linea->tamanioStack, tamanioUint32);
		offset+=tamanioUint32;
		t_entrada_stack* stack=linea->stack;
		uint32_t cantidadArgumentos=list_size(stack->argumentos);
		memcpy(buffer + offset, &cantidadArgumentos, tamanioUint32);
		offset+=tamanioUint32;

		for(p=0; p<cantidadArgumentos; p++){
			t_argumento *argumento=list_get(stack->argumentos, p);
			memcpy(buffer + offset, &argumento->pagina, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &argumento->offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &argumento->size, tamanioUint32);
			offset+=tamanioUint32;
		}
		uint32_t cantidadVariables=list_size(stack->variables);
		memcpy(buffer + offset, &cantidadVariables, tamanioUint32);
		offset+=tamanioUint32;

		for(p=0; p<cantidadVariables; p++){
			t_var *variable=list_get(stack->variables, p);
			memcpy(buffer + offset, &variable->id, sizeof(char));
			offset+=sizeof(char);
			memcpy(buffer + offset, &variable->pagina, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &variable->offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &variable->size, tamanioUint32);
			offset+=tamanioUint32;
		}
		memcpy(buffer + offset, &stack->direcretorno, tamanioUint32);
		offset+=tamanioUint32;
		t_posicion* retVarStack=stack->retVar;

		if(!retVarStack){
			pos=0;
			memcpy(buffer + offset, &pos, tamanioUint32);
			offset+=tamanioUint32;
		}else{
			pos=1;
			memcpy(buffer + offset, &pos, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &retVarStack->pagina, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &retVarStack->offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(buffer + offset, &retVarStack->size, tamanioUint32);
			offset+=tamanioUint32;
		}
	}

	for(a=0; a<list_size(tamanioStackStack); a++){
		free(list_remove(tamanioStackStack, a));
	}

	free(tamanioStackStack);
	t_buffer_tamanio* bufferTamanio=malloc(sizeof(t_buffer_tamanio));
	bufferTamanio->tamanioBuffer=tamanioTotalBuffer + sizeof(uint32_t);
	bufferTamanio->buffer=buffer;

	return bufferTamanio;
}

t_list* deserializarIndiceStack(char* buffer){
	uint16_t i,p;
	t_list* stack=list_create();
	uint16_t offset=0,tamanioUint32=sizeof(uint32_t);
	uint32_t cantidadElementosEnStack;
	memcpy(&cantidadElementosEnStack, buffer + offset, tamanioUint32);
	offset+=tamanioUint32;

	for(i=0; i<cantidadElementosEnStack; i++){
		uint32_t tamanioItemStack;
		memcpy(&tamanioItemStack, buffer + offset, tamanioUint32);
		offset+=tamanioUint32;
		t_entrada_stack* stack_item=malloc(sizeof(t_entrada_stack));
		t_list* argumentosStack=list_create();
		uint32_t cantidadArgumentosStack;
		memcpy(&cantidadArgumentosStack, buffer + offset, tamanioUint32);
		offset+=tamanioUint32;

		for(p=0; p<cantidadArgumentosStack; p++){
			t_argumento* argStack=malloc(sizeof(t_argumento));
			memcpy(&argStack->pagina, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(&argStack->offset, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(&argStack->size, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			list_add(argumentosStack, argStack);
		}
		stack_item->argumentos=argumentosStack;
		t_list* variablesStack=list_create();
		uint32_t cantidadVariablesStack;
		memcpy(&cantidadVariablesStack, buffer + offset, tamanioUint32);
		offset+=tamanioUint32;

		for(p=0; p<cantidadVariablesStack; p++){
			t_var* varStack=malloc(sizeof(t_var));
			memcpy(&varStack->id, buffer + offset, sizeof(char));
			offset+=sizeof(char);
			memcpy(&varStack->pagina, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(&varStack->offset, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(&varStack->size, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			list_add(variablesStack, varStack);
		}
		stack_item->variables = variablesStack;
		uint32_t direcRetorno;
		memcpy(&direcRetorno, buffer + offset, tamanioUint32);
		stack_item->direcretorno=direcRetorno;
		offset+=tamanioUint32;
		memcpy(&direcRetorno, buffer + offset, tamanioUint32);
		offset+=tamanioUint32;
		t_posicion* retVarStack;
		if(!direcRetorno)
			retVarStack=NULL;
		else{
			retVarStack=malloc(sizeof(t_posicion));
			memcpy(&retVarStack->pagina, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(&retVarStack->offset, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
			memcpy(&retVarStack->size, buffer + offset, tamanioUint32);
			offset+=tamanioUint32;
		}
		stack_item->retVar=retVarStack;
		list_add(stack, stack_item);
	}
	return stack;
}

t_buffer_tamanio* serializarIndiceDeCodigo(t_list* indiceCodigo) {
	uint32_t offset = 0, m = 0;
	int tmp_size = sizeof(uint32_t);
	char* bufferIndCod = malloc(list_size(indiceCodigo) * 2 * sizeof(uint32_t));
	uint32_t itemsEnLista = list_size(indiceCodigo);
//	memcpy(bufferIndCod + offset, &itemsEnLista, tmp_size);
//	offset += tmp_size;

	t_indice_codigo* linea;
	for (m = 0; m < itemsEnLista; m++) {
		linea = list_get(indiceCodigo, m);
		memcpy(bufferIndCod + offset, &linea->offset, tmp_size);
		offset += tmp_size;
		memcpy(bufferIndCod + offset, &linea->size, tmp_size);
		offset += tmp_size;
	}

	t_buffer_tamanio * tamanioBuffer = malloc(sizeof(uint32_t)*2);
	tamanioBuffer->tamanioBuffer = offset;
	tamanioBuffer->buffer = malloc(tamanioBuffer->tamanioBuffer);
	memcpy(tamanioBuffer->buffer,bufferIndCod,tamanioBuffer->tamanioBuffer);

	return tamanioBuffer;
}

t_list* deserializarIndiceCodigo(char* indice, uint32_t tam) {
	int unit;
	t_list* ret = list_create();
	uint32_t offset = 0;
	uint32_t tmp_size = sizeof(uint32_t);
	unit=tam/(sizeof(uint32_t)*2);
	int i;
	for(i = 0; i < unit; i++){
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		memcpy(&linea->offset, indice + offset, tmp_size);
		offset += tmp_size;
		memcpy(&linea->size, indice + offset, tmp_size);
		offset += tmp_size;
		list_add(ret, linea);
	}
	return ret;
}

t_buffer_tamanio* serializar_pcb(t_pcb* pcb){
	t_buffer_tamanio* indcod = serializarIndiceDeCodigo(pcb->indiceCodigo);

	char* paqueteSerializado;
	t_buffer_tamanio* stackSerializado = serializarIndiceStack(pcb->indiceStack);

	uint32_t tamanioPCB=0;
	tamanioPCB+=sizeof(uint32_t);
	tamanioPCB+=indcod->tamanioBuffer;
	tamanioPCB+=sizeof(uint32_t);
	tamanioPCB+=stackSerializado->tamanioBuffer;
	tamanioPCB+=sizeof(uint32_t);

	if (pcb->etiquetas != NULL && pcb->tamanioEtiquetas > 0)
		tamanioPCB+=pcb->tamanioEtiquetas;
	tamanioPCB+=sizeof(uint32_t) * 7;
	paqueteSerializado = malloc(tamanioPCB);
	uint32_t offset = 0;
	uint32_t tmp_size = sizeof(uint32_t);

	memcpy(paqueteSerializado+offset, &pcb->pid, tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado+offset, &pcb->codigo, tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado+offset, &pcb->programCounter, tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado+offset, &pcb->stackPointer, tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado+offset, &pcb->cantPaginasCodigo, tmp_size);
 	offset += tmp_size;
 	memcpy(paqueteSerializado+offset, &pcb->exitCode, tmp_size);
 	offset += tmp_size;
	memcpy(paqueteSerializado+offset, &pcb->consolaFd, tmp_size);
 	offset += tmp_size;
 	memcpy(paqueteSerializado+offset, &stackSerializado->tamanioBuffer, tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado+offset, (void*)stackSerializado->buffer, stackSerializado->tamanioBuffer);
	offset += stackSerializado->tamanioBuffer;

 	if(pcb->etiquetas != NULL && pcb->tamanioEtiquetas > 0){
		memcpy(paqueteSerializado+offset, &pcb->tamanioEtiquetas, tmp_size);
		offset += tmp_size;
		memcpy(paqueteSerializado+offset, pcb->etiquetas, pcb->tamanioEtiquetas);
		offset += pcb->tamanioEtiquetas;
	}else{
		uint32_t longitudIndiceEtiquetas = 0;
		memcpy(paqueteSerializado+offset, &longitudIndiceEtiquetas, tmp_size);
		offset += tmp_size;
	}
	memcpy(paqueteSerializado+offset, &indcod->tamanioBuffer, tmp_size);
	offset += tmp_size;
	memcpy(paqueteSerializado+offset, (void*)indcod->buffer, indcod->tamanioBuffer);
	offset += indcod->tamanioBuffer;
	free(indcod->buffer);
	free(indcod);
	free(stackSerializado->buffer);
	free(stackSerializado);

	t_buffer_tamanio * buffer_tamanio = malloc(sizeof(t_buffer_tamanio));
	buffer_tamanio->tamanioBuffer = tamanioPCB;
	buffer_tamanio->buffer = malloc(buffer_tamanio->tamanioBuffer);
	memcpy(buffer_tamanio->buffer,paqueteSerializado,buffer_tamanio->tamanioBuffer);

	return buffer_tamanio;
}

t_pcb* deserializar_pcb(char* package){
 	uint32_t offset = 0, tmp_size = sizeof(uint32_t);
 	char* indiceEtiquetas;
 	char* bufferIndiceDeCodigo;
 	t_pcb* pcb;
 	t_list* indiceDeCodigo;
 	pcb = malloc (sizeof(t_pcb));

 	memcpy(&pcb->pid, package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&pcb->codigo, package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&pcb->programCounter, package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&pcb->stackPointer, package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&pcb->cantPaginasCodigo, package + offset, tmp_size);
 	offset += tmp_size;
 	memcpy(&pcb->exitCode, package + offset, tmp_size);
 	offset += tmp_size;
	memcpy(&pcb->consolaFd, package + offset, tmp_size);
 	offset += tmp_size;

 	uint32_t tamanioStack;
	memcpy(&tamanioStack, package + offset, tmp_size);
	offset += tmp_size;
	char * bufferIndiceStack = malloc(tamanioStack);
	memcpy(bufferIndiceStack, package + offset, tamanioStack);
	offset += tamanioStack;
	t_list * indiceStack = deserializarIndiceStack(bufferIndiceStack);
	pcb->indiceStack = indiceStack;
 	memcpy(&pcb->tamanioEtiquetas, package + offset, tmp_size);
	offset += tmp_size;

	if(pcb->tamanioEtiquetas){
		indiceEtiquetas = malloc(pcb->tamanioEtiquetas);
		memcpy(indiceEtiquetas, package + offset, pcb->tamanioEtiquetas);
		pcb->etiquetas = indiceEtiquetas;
		offset += pcb->tamanioEtiquetas;
	}else
		pcb->etiquetas = NULL;

	uint32_t itemsIndiceDeCodigo;
 	memcpy(&itemsIndiceDeCodigo, package + offset, tmp_size);
 	offset += tmp_size;
 	if(itemsIndiceDeCodigo){
		bufferIndiceDeCodigo=malloc(itemsIndiceDeCodigo);
		memcpy(bufferIndiceDeCodigo, package + offset, itemsIndiceDeCodigo);
		offset += itemsIndiceDeCodigo;
 	indiceDeCodigo = deserializarIndiceCodigo(bufferIndiceDeCodigo, itemsIndiceDeCodigo);
 	pcb->indiceCodigo = indiceDeCodigo;
 	}
 	else
 		pcb->indiceCodigo=list_create();
 	free(bufferIndiceDeCodigo);
 	free(bufferIndiceStack);

 	return pcb;
 }

header_t crear_cabecera(int codigo, int length){

	header_t cabecera;

	cabecera.type = codigo;
	cabecera.length = length;

	return cabecera;
}

int enviar_paquete_vacio(int codigo_operacion, int socket){

	int bytes_enviados;
	bytes_enviados=enviar_info(socket, codigo_operacion, 0, NULL);

	return bytes_enviados;
}

/* sendAll y recvAll: Te aseguran que se envio o recibio tod-o.
 * Respetan la misma interfaz que send y recv originales*/
int sendAll(int fd, char *cosa, int size, int flags){

	int cant_enviada = 0;
	int aux;

	while(cant_enviada < size){

		aux = send(fd, cosa + cant_enviada, size - cant_enviada, flags);

		if(aux == -1){
			return -1;
		}else if(aux == 0){
			return -1;
		}

		cant_enviada += aux;
	}

	//Ya envie correctamente
	return cant_enviada;
}

int recvAll(int fd, char *buffer, int size, int flags){

	int cant_recibida = 0;
	int aux;

	while(cant_recibida < size){

		aux = recv(fd, buffer + cant_recibida, size - cant_recibida, flags);

		if(aux == -1){
			return -1;
		}else if(aux == 0)
			return 0;

		cant_recibida += aux;
	}

	return cant_recibida;
}

int enviar_info(int sockfd, int codigo_operacion, int length, void* buff){

	int bytes_enviados;
	header_t cabecera = crear_cabecera(codigo_operacion, length);

	bytes_enviados = sendAll(sockfd, (char*)&cabecera, sizeof(cabecera), 0);

	if(bytes_enviados == 0) return 0;
	if(bytes_enviados == -1) return -1;

	if(cabecera.length!=0){
		bytes_enviados += sendAll(sockfd, (char*)buff, cabecera.length, 0);
		if(bytes_enviados == 0) return 0;
		if(bytes_enviados == -1) return -1;
	}

	return bytes_enviados;
}

int recvMsj(int socket, void** paquete, header_t *header){

	int bytes;
	bytes=recvAll(socket, (char*)header, sizeof(header_t), MSG_WAITALL);
	if(bytes == 0) return 0;
	if(bytes == -1) return -1;
	if(header->length){
		*paquete = malloc(header->length);
		bytes = recvAll(socket, (char*)*paquete, header->length, MSG_WAITALL);
		if(bytes == 0) return 0;
		if(bytes == -1) return -1;
	}
	else *paquete=NULL;
	return bytes;
}



