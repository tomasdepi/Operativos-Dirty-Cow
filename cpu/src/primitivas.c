/*
 * primitivas.c
 *
 *  Created on: 17/4/2017
 *      Author: utnso
 */
#include "primitivas.h"

int cantDeReservas = 0;

bool esArgumento(t_nombre_variable identificador_variable){
	if(isdigit(identificador_variable)){
		return true;
	}else{
		return false;
	}
}

void setPCB(t_pcb * pcbDeCPU){
	pcb = pcbDeCPU;
}

/*************************************** OPERACIONES *******************************************/

/*
 * DEFINIR VARIABLE
 *
 * Reserva en el Contexto de Ejecución Actual el espacio necesario para una variable llamada identificador_variable y la registra tanto en el Stack como en el Diccionario
 * de Variables. Retornando la posición del valor de esta nueva variable del stack
 * El valor de la variable queda indefinido: no deberá inicializarlo con ningún valor default.
 * Esta función se invoca una vez por variable, a pesar que este varias veces en una línea.
 * Ej: Evaluar "variables a, b, c" llamará tres veces a esta función con los parámetros "a", "b" y "c"
 *
 * @sintax	TEXT_VARIABLE (variables)
 * 			-- nota: Al menos un identificador; separado por comas
 * @param	identificador_variable	Nombre de variable a definir
 * @return	Puntero a la variable recien asignada
 */
t_puntero definirVariable(t_nombre_variable identificador_variable){
	t_entrada_stack* lineaStack;
	uint32_t pag,offset,posAbsoluta;
	if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioStack * tamanioPagina){
		log_error(logger, "StackOverflow. Se finaliza el proceso");
		huboStackOver = true;
		return -1;
	}
	pag = pcb->stackPointer / tamanioPagina + pcb->cantPaginasCodigo;
	offset = pcb->stackPointer % tamanioPagina;

	if(list_size(pcb->indiceStack) == 0){
		lineaStack = crearPosicionStack();
		list_add(pcb->indiceStack, lineaStack);
	}else
		lineaStack = list_get(pcb->indiceStack, list_size(pcb->indiceStack) - 1);

	if(!esArgumento(identificador_variable)){ // Es una variable
		log_debug(logger, "ANSISOP_definirVariable %c", identificador_variable);
		t_var* nuevaVar = malloc(sizeof(t_var));
		nuevaVar->id = identificador_variable;
		nuevaVar->pagina = pag;
		nuevaVar->offset = offset;
		nuevaVar->size = TAMANIO_VARIABLE;
		list_add(lineaStack->variables, nuevaVar);
	}
	else{ // Es un argumento
		log_debug(logger, "ANSISOP_definirVariable (argumento) %c", identificador_variable);
		t_argumento* nuevoArg = malloc(sizeof(t_argumento));
		nuevoArg->pagina = pag;
		nuevoArg->offset = offset;
		nuevoArg->size = TAMANIO_VARIABLE;
		list_add(lineaStack->argumentos, nuevoArg);
	}
	pcb->stackPointer += TAMANIO_VARIABLE;
	posAbsoluta = pag * tamanioPagina + offset;
	log_info(logger, "Posicion relativa de %c: %d %d %d", identificador_variable, pag, offset, TAMANIO_VARIABLE);
	log_info(logger, "Posicion absoluta de %c: %i", identificador_variable, posAbsoluta);

	return posAbsoluta;
}

/*
 * ASIGNAR
 *
 * Inserta una copia del valor en la variable ubicada en direccion_variable.
 *
 * @sintax	TEXT_ASSIGNATION (=)
 * @param	direccion_variable	lugar donde insertar el valor
 * @param	valor	Valor a insertar
 * @return	void
 */
void asignar(t_puntero direccion, t_valor_variable valor){
	int resultado;
	if(direccion == -1) return;
	header_t header;
	log_debug(logger, "ANSISOP_asignar -> posicion var: %d - valor: %d", direccion, valor);

	t_pedido_bytes pedidoEscritura;
	pedidoEscritura.pag = direccion / tamanioPagina;
	pedidoEscritura.offset = direccion % tamanioPagina;
	pedidoEscritura.size = TAMANIO_VARIABLE;
	pedidoEscritura.pid = pcb->pid;

	if(pedidoEscritura.pag >= pcb->cantPaginasCodigo + tamanioStack){
		header.type = VERIFICAR_ASIGNAR;
		header.length = sizeof(t_pedido_bytes);
		if(sendSocket(socketConexionKernel,&header,&pedidoEscritura) <= 0){
			log_error(logger, "Conexion con kernel perdida...");
			finalizarCPU();
		}
		resultado = requestHandlerKernel();

		if(resultado == -1){
			log_error(logger, "La variable no pudo asignarse");
			return;
		}
		else{
			if(almacenarBytes(&pedidoEscritura, &valor) != 0) log_error(logger, "La variable no pudo asignarse");
			else log_info(logger, "Variable asignada");
			return;
		}
	}
	else{
		if(almacenarBytes(&pedidoEscritura, &valor) != 0) log_error(logger, "La variable no pudo asignarse");
		else log_info(logger, "Variable asignada");
	}
}

/*
 * ASIGNAR VALOR a variable COMPARTIDA
 *
 * Pide al kernel asignar el valor a la variable compartida.
 * Devuelve el valor asignado.
 *
 * @sintax	TEXT_VAR_START_GLOBAL (!) IDENTIFICADOR TEXT_ASSIGNATION (=) EXPRESION
 * @param	variable	Nombre (sin el '!') de la variable a pedir
 * @param	valor	Valor que se le quire asignar
 * @return	Valor que se asigno
 */
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	log_debug(logger, "ANSISOP_asignarValorCompartida var: %s, valor: %d", variable, valor);
	uint32_t offset = 0;
	int16_t var;
	void* buffer; // Contiene el size del nombre, el nombre y el valor.
	uint32_t sizeVariable = strlen(variable) + 1;
	uint32_t sizeTotal = sizeof(sizeVariable) + sizeVariable + sizeof(valor);
	header_t header;
	header.type = ASIG_VAR_COMPARTIDA;
	header.length = sizeTotal;
	buffer = malloc(sizeTotal);
	memcpy(buffer, &sizeVariable, sizeof(sizeVariable));
	offset+=sizeof(sizeVariable);
	memcpy(buffer + offset, variable, strlen(variable)+1);
	offset+=strlen(variable)+1;
	memcpy(buffer+offset, &valor, sizeof(valor));
	if(sendSocket(socketConexionKernel,&header,buffer) <= 0){
		log_error(logger,"Error al enviar");
		free(buffer);
		return -1;
	}
	log_info(logger, "Se solicito al kernel asignar el valor %d a la variable %s", valor, variable);
	var=requestHandlerKernel();
	if(var == -1){
		log_error(logger, "No se pudo asignar la variable %s", variable);
		if(buffer)free(buffer);
		return -1;
	}
	log_info(logger, "Valor asignado");
	if(buffer)free(buffer);
	return valor;
}

/*
 * DEREFERENCIAR
 *
 * Obtiene el valor resultante de leer a partir de direccion_variable, sin importar cual fuera el contexto actual
 *
 * @sintax	TEXT_DEREFERENCE_OP (*)
 * @param	direccion_variable	Lugar donde buscar
 * @return	Valor que se encuentra en esa posicion
 */
t_valor_variable dereferenciar(t_puntero direccion_variable){
	t_valor_variable valor;
	log_debug(logger, "ANSISOP_dereferenciar posicion: %d", direccion_variable);
	//calculo la posicion de la variable en el stack mediante el desplazamiento
	t_pedido_bytes solicitar;
	solicitar.pag = direccion_variable / tamanioPagina;
	solicitar.offset = direccion_variable % tamanioPagina;
	solicitar.size = TAMANIO_VARIABLE;
	solicitar.pid = pcb->pid;
	if(solicitarBytes(&solicitar) != 0){
		log_error(logger,"La variable no pudo dereferenciarse");
		return -1;
	}
	if(!paqueteGlobal){
		log_error(logger,"La variable no pudo dereferenciarse");
		return -1;
	}
	valor = *(t_valor_variable*)paqueteGlobal;
	log_info(logger, "Variable dereferenciada. Valor: %d", valor);
	free(paqueteGlobal);
	return valor;
}

/*
 * FINALIZAR
 *
 * Cambia el Contexto de Ejecución Actual para volver al Contexto anterior al que se está ejecutando,
 * recuperando el Cursor de Contexto Actual y el Program Counter previamente apilados en el Stack.
 * En caso de estar finalizando el Contexto principal (el ubicado al inicio del Stack), deberá
 * finalizar la ejecución del programa.
 *
 * @sintax	TEXT_END (end)
 * @param	void
 * @return	void
 */
void finalizar(void){
	log_debug(logger,"ANSISOP_finalizar");
	//Obtengo contexto quitado de la lista y lo limpio.
	t_entrada_stack* contexto = list_remove(pcb->indiceStack, list_size(pcb->indiceStack) - 1);
	if(contexto != NULL){
		pcb->stackPointer -= TAMANIO_VARIABLE * (list_size(contexto->argumentos) + list_size(contexto->variables)); // Disminuyo stackPointer del pcb
		if(pcb->stackPointer >= 0){
			uint16_t i;
			for(i=0; i<list_size(contexto->argumentos); i++){ // Limpio lista de argumentos del contexto
				free(list_remove(contexto->argumentos,i));
			}
			for(i=0; i<list_size(contexto->variables); i++){
				free(list_remove(contexto->variables, i));
			}
		}
		list_destroy(contexto->argumentos);
		list_destroy(contexto->variables);
		if(contexto->retVar)free(contexto->retVar);
	}
	if(list_size(pcb->indiceStack) == 0){
		finPrograma = true;
		log_info(logger, "Finalizó la ejecucion del programa.");
		//finalizarPor(FIN_PROCESO);
	}else{
		pcb->programCounter = contexto->direcretorno;
	}
	free(contexto);
	return;
}

/*
 * IR a la ETIQUETA
 * Cambia la linea de ejecucion a la correspondiente de la etiqueta buscada.
 * @sintax	TEXT_GOTO (goto)
 * @param	t_nombre_etiqueta	Nombre de la etiqueta
 * @return	void
 */
void irAlLabel(t_nombre_etiqueta etiqueta){
		log_debug(logger,"ANSISOP_irALabel %s", etiqueta);
		t_puntero_instruccion numeroInstr = metadata_buscar_etiqueta(etiqueta, pcb->etiquetas, pcb->tamanioEtiquetas);
		log_debug(logger, "Instruccion del irALAbel: %d", numeroInstr);
		if(numeroInstr == -1){
			log_error(logger,"No se encontro la etiqueta");
			return;
		}
		pcb->programCounter = numeroInstr - 1;
}

/*
 * LLAMAR CON RETORNO
 *
 * Preserva el contexto de ejecución actual para poder retornar luego al mismo, junto con la posicion de la variable entregada por donde_retornar.
 * Modifica las estructuras correspondientes para mostrar un nuevo contexto vacío.
 *
 * Los parámetros serán definidos luego de esta instrucción de la misma manera que una variable local, con identificadores numéricos empezando por el 0.
 *
 * @sintax	TEXT_CALL (<-)
 * @param	etiqueta	Nombre de la funcion
 * @param	donde_retornar	Posicion donde insertar el valor de retorno
 * @return	void
 */
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	log_debug(logger, "ANSISOP_llamarConRetorno etiqueta: %s, retornar: %d", etiqueta, donde_retornar);
	t_entrada_stack * nuevaLineaStackEjecucionActual;
	t_posicion* varRetorno = malloc(sizeof(t_posicion));
	varRetorno->pagina = donde_retornar / tamanioPagina;
	varRetorno->offset = donde_retornar % tamanioPagina;
	varRetorno->size = TAMANIO_VARIABLE;
	nuevaLineaStackEjecucionActual = crearPosicionStack();
	nuevaLineaStackEjecucionActual->retVar = varRetorno;
	nuevaLineaStackEjecucionActual->direcretorno = pcb->programCounter;

	// La agrego a la lista, se encuentra en la ultima posicion.
	list_add(pcb->indiceStack, nuevaLineaStackEjecucionActual);

	irAlLabel(etiqueta);
}

/*
 * LLAMAR SIN RETORNO
 * Preserva el contexto de ejecución actual para poder retornar luego al mismo.
 * Modifica las estructuras correspondientes para mostrar un nuevo contexto vacío.
 *
 * Los parámetros serán definidos luego de esta instrucción de la misma manera que una variable local, con identificadores numéricos empezando por el 0.
 *
 * @sintax	Sin sintaxis particular, se invoca cuando no coresponde a ninguna de las otras reglas sintacticas
 * @param	etiqueta	Nombre de la funcion
 * @return	void
 */

void llamarSinRetorno(t_nombre_etiqueta etiqueta){
	log_debug(logger, "ANSISOP_llamarSinRetorno %s", etiqueta);
	t_entrada_stack* nuevaLineaStack = crearPosicionStack();
	nuevaLineaStack->direcretorno = pcb->programCounter;
	list_add(pcb->indiceStack, nuevaLineaStack);
	irAlLabel(etiqueta);
}

/*
 * OBTENER POSICION de una VARIABLE
 *
 * Devuelve el desplazamiento respecto al inicio del segmento Stacken que se encuentra el valor de la variable identificador_variable del contexto actual.
 * En caso de error, retorna -1.
 *
 * @sintax	TEXT_REFERENCE_OP (&)
 * @param	identificador_variable 		Nombre de la variable a buscar (De ser un parametro, se invocara sin el '$')
 * @return	Donde se encuentre la variable buscada
 */
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	log_debug(logger, "ANSISOP_obtenerPosicion %c", identificador_variable);
	if(list_size(pcb->indiceStack) == 0){
		log_error(logger, "No hay nada en el indice de stack");
		return -1;
	}
	uint32_t i;
	t_puntero posicionAbsoluta;
	t_entrada_stack* contexto = list_get(pcb->indiceStack, list_size(pcb->indiceStack) - 1);

	if(!esArgumento(identificador_variable)){ // es una variable
		t_var* var_local;
		bool notFound = true;
		for(i=0; i < list_size(contexto->variables); i++){
			var_local = list_get(contexto->variables, i);
			if(var_local->id == identificador_variable){
				notFound = false;
				break;
			}
		}
		if(notFound){
			log_error(logger, "No se encontro la variable %c en el stack", identificador_variable);
			return -1;
		}
		else{
			posicionAbsoluta = var_local->pagina * tamanioPagina + var_local->offset;
		}
	} // es un argumento
	else{
		if(identificador_variable -'0'> list_size(contexto->argumentos)){
			return -1;
		}else{
			t_argumento* argumento = list_get(contexto->argumentos, identificador_variable-'0');
			posicionAbsoluta = argumento->pagina * tamanioPagina + argumento->offset;
		}
	}
	log_info(logger, "Posicion absoluta de %c: %d", identificador_variable, posicionAbsoluta);
	return posicionAbsoluta;
}

/*
 * OBTENER VALOR de una variable COMPARTIDA
 *
 * Pide al kernel el valor (copia, no puntero) de la variable compartida por parametro.
 *
 * @sintax	TEXT_VAR_START_GLOBAL (!)
 * @param	variable	Nombre de la variable compartida a buscar
 * @return	El valor de la variable compartida
 */
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	log_debug(logger, "ANSISOP_obtenerValorCompartida %s", variable);
	header_t header;
	header.type = LEER_VAR_COMPARTIDA;
	header.length = strlen(variable)+1;
	sendSocket(socketConexionKernel, &header, variable);
	if (requestHandlerKernel() == -1) {
		log_error(logger, "Variable %s no definida");
		return -1;
	}
	int32_t valor = *(int32_t*)paqueteGlobal;
	if(paqueteGlobal) free(paqueteGlobal);
	log_info(logger, "Valor de %s: %d", variable, valor);
	return valor;
}

/*
 * RETORNAR
 *
 * Cambia el Contexto de Ejecución Actual para volver al Contexto anterior al que se está ejecutando,
 * recuperando el Cursor de Contexto Actual, el Program Counter y la direccion donde retornar, asignando el
 * valor de retorno en esta, previamente apilados en el Stack.
 *
 * @sintax	TEXT_RETURN (return)
 * @param	retorno	Valor a ingresar en la posicion corespondiente
 * @return	void
 */
void retornar(t_valor_variable retorno){
	log_debug(logger, "ANSISOP_retornar");
	t_entrada_stack* contextoEjecucionActual = list_remove(pcb->indiceStack, list_size(pcb->indiceStack) - 1);
	//Limpio el contexto actual
	uint32_t i;
	if(contextoEjecucionActual == NULL){
		log_error(logger, "No se encontro el contexto de ejecucion actual");
		return;
	}
	for(i=0; i < list_size(contextoEjecucionActual->argumentos); i++){
		t_argumento* arg = list_get(contextoEjecucionActual->argumentos, i);
		free(arg);
		pcb->stackPointer = pcb->stackPointer - TAMANIO_VARIABLE;
	}
	for(i=0; i < list_size(contextoEjecucionActual->variables); i++){
		t_var* var = list_get(contextoEjecucionActual->variables, i);
		free(var);
		pcb->stackPointer = pcb->stackPointer-TAMANIO_VARIABLE;
	}
	//calculo la direccion a la que tengo que retornar mediante la direccion de pagina start y offset que esta en el campo retvar
	t_posicion* retVar = contextoEjecucionActual->retVar;
	t_puntero direcVariable = retVar->pagina * tamanioPagina + retVar->offset;
	log_info(logger, "Se asigna el valor a la posicion de retorno");
	asignar(direcVariable, retorno);

	//Seteo el contexto de ejecucion actual en el anterior
	pcb->programCounter = contextoEjecucionActual->direcretorno;

	//elimino el contexto actual del indice del stack
	free(contextoEjecucionActual->retVar);
	list_destroy(contextoEjecucionActual->argumentos);
	list_destroy(contextoEjecucionActual->variables);
	free(contextoEjecucionActual);
	log_info(logger, "Contexto limpiado");
	return;
}

/*************************** OPERACIONES DEL KERNEL *************************************/

/*
 * ABRIR ARCHIVO
 * Informa al Kernel que el proceso requiere que se abra un archivo.
 *
 * @syntax 	TEXT_OPEN_FILE (abrir)
 * @param	direccion		Ruta al archivo a abrir
 * @param	banderas		String que contiene los permisos con los que se abre el archivo
 * @return	El valor del descriptor de archivo abierto por el sistema
 */
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){
	log_debug(logger, "ANSISOP_abrir '%s'", direccion);
	uint32_t offset = 0;

	//armo el header
	uint32_t sizeDireccion = strlen(direccion) + 1;
	uint32_t sizeTotal = sizeDireccion + sizeof(uint32_t) * 2 + sizeof(t_banderas); // un int es el pid y el otro el strlen de direccion

	header_t header;;
	header.type = ABRIR_ARCHIVO;
	header.length = sizeTotal;

	//armo el paquete con la direccion del archivo, las banderas y el pid del proceso
	char* paquete = malloc(sizeTotal);
	memcpy(paquete+offset, (void*)&pcb->pid, sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(paquete+offset, &sizeDireccion, sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(paquete+offset, direccion, sizeDireccion);
	offset+=sizeDireccion;
	memcpy(paquete+offset, &flags, sizeof(t_banderas));

	sendSocket(socketConexionKernel,&header,(void*)paquete);
	log_info(logger, "Flags enviadas: lectura:%d - escritura:%d - creacion: %d", flags.lectura, flags.escritura, flags.creacion);
	free(paquete);
	if(requestHandlerKernel() == -1){
		log_error(logger, "Error al abrir el archivo");
		return -1;
	}
	//recibo el descriptor del archivo abierto
	t_descriptor_archivo fd = *(t_descriptor_archivo*)paqueteGlobal;
	free(paqueteGlobal);
	log_info(logger, "File descriptor recibido: %d", fd);
	return fd;
}

/*
 * BORRAR ARCHIVO
 *
 * Informa al Kernel que el proceso requiere que se borre un archivo.
 *
 * @syntax 	TEXT_DELETE_FILE (borrar)
 * @param	direccion		Ruta al archivo a abrir
 * @return	void
 */
void borrar(t_descriptor_archivo direccion){
	log_debug(logger, "ANSISOP_borrar -> direccion: %d", direccion);
	header_t header;
	uint32_t sizeTotal = sizeof(uint32_t) * 2;
	char* paquete = malloc(sizeTotal);
	header.type =BORRAR_ARCHIVO;
	header.length = sizeTotal;
	uint32_t offset = 0;
	memcpy(paquete, &(pcb->pid), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete+offset,&direccion, sizeof(uint32_t));
	sendSocket(socketConexionKernel,&header,paquete);
	free(paquete);
	if(requestHandlerKernel() != 0) log_error(logger, "No se pudo borrar");
	else log_info(logger, "Borrado con exito");
}

/*
 * CERRAR ARCHIVO
 *
 * Informa al Kernel que el proceso requiere que se cierre un archivo.
 *
 * @syntax 	TEXT_CLOSE_FILE (cerrar)
 * @param	descriptor_archivo		Descriptor de archivo del archivo abierto
 * @return	void
 */
void cerrar(t_descriptor_archivo descriptor_archivo){
	log_debug(logger, "ANSISOP_cerrar -> fd: %d", descriptor_archivo);
	header_t header;
	uint32_t sizeTotal = sizeof(int) * 2;
	char* paquete = malloc(sizeTotal);
	header.type = CERRAR_ARCHIVO;
	header.length = sizeTotal;
	uint32_t offset = 0;
	memcpy(paquete, &(pcb->pid), sizeof(int));
	offset += sizeof(int);
	memcpy(paquete+offset,&descriptor_archivo, sizeof(int));
	sendSocket(socketConexionKernel,&header,paquete);
	free(paquete);
	if(requestHandlerKernel() != 0) log_error(logger, "Error al cerrar el archivo");
	else log_info(logger, "Cerrado con exito");
}

/*
 * ESCRIBIR ARCHIVO
 * Informa al Kernel que el proceso requiere que se escriba un archivo previamente abierto.
 * El mismo escribira "tamanio" de bytes de "informacion" luego del cursor
 * No es necesario mover el cursor luego de esta operación
 *
 * @syntax 	TEXT_WRITE_FILE (escribir)
 * @param	descriptor_archivo		Descriptor de archivo del archivo abierto
 * @param	informacion			Informacion a ser escrita
 * @param	tamanio				Tamanio de la informacion a enviar
 * @return	void
 */
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	log_debug(logger, "ANSISOP_escribir -> fd: %d", descriptor_archivo);
	header_t header;
	header.type = ESCRIBIR;

	size_t size = sizeof(pcb->pid) + sizeof(uint32_t) * 2 + tamanio + 1;
	void* buffer = malloc(size);
	header.length = size;

	uint32_t offset = 0;

	memcpy(buffer, &descriptor_archivo, sizeof(descriptor_archivo));
	offset += sizeof(descriptor_archivo);
	memcpy(buffer + offset, &pcb->pid, sizeof(pcb->pid));
	offset += sizeof(pcb->pid);
	memcpy(buffer + offset , &tamanio, sizeof(tamanio));
	offset += sizeof(tamanio);
	memcpy(buffer + offset, informacion, tamanio + 1);

	if(sendSocket(socketConexionKernel, &header, buffer) <= 0){
		if(buffer)free(buffer);
		log_error(logger, "Conexion con kernel perdida...");
		finalizarCPU();
	}
	free(buffer);
	if(requestHandlerKernel() != 0) log_error(logger, "No se pudo escribir");
	else log_info(logger, "Escritura exitosa");
}

/*
 * LEER ARCHIVO
 *
 * Informa al Kernel que el proceso requiere que se lea un archivo previamente abierto.
 * El mismo leera "tamanio" de bytes luego del cursor.
 * No es necesario mover el cursor luego de esta operación
 *
 * @syntax 	TEXT_READ_FILE (leer)
 * @param	descriptor_archivo		Descriptor de archivo del archivo abierto
 * @param	informacion			Puntero que indica donde se guarda la informacion leida
 * @param	tamanio				Tamanio de la informacion a leer
 * @return	void
 */
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	log_debug(logger, "ANSISOP_leer -> fd: %d - informacion: %d - tamanio: %d", descriptor_archivo, informacion, tamanio);
	header_t header;
	t_lectura lectura;
	lectura.pid = pcb->pid;
	lectura.descriptor = descriptor_archivo;
	lectura.informacion = informacion;
	lectura.size = tamanio;
	header.type = LEER_ARCHIVO;
	header.length = sizeof(t_lectura);

	if(sendSocket(socketConexionKernel, &header, &lectura) <= 0){
		finalizarCPU();
	}
	if(requestHandlerKernel() != 0) log_error(logger, "Error al leer");
	else log_info(logger, "Lectura exitosa");
}

/*
 * MOVER CURSOR DE ARCHIVO
 *
 * Informa al Kernel que el proceso requiere que se mueva el cursor a la posicion indicada.
 *
 * @syntax 	TEXT_SEEK_FILE (buscar)
 * @param	descriptor_archivo		Descriptor de archivo del archivo abierto
 * @param	posicion			Posicion a donde mover el cursor
 * @return	void
 */
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	log_debug(logger, "ANSISOP_moverCursor");
	header_t header;
	t_cursor cursor;
	header.type = MOVER_CURSOR;
	header.length = sizeof(t_cursor);
	cursor.pid = pcb->pid;
	cursor.posicion = posicion;
	cursor.descriptor = descriptor_archivo;
	if(sendSocket(socketConexionKernel, &header, &cursor) <= 0){
		finalizarCPU();
	}
	if(requestHandlerKernel() == -1) log_error(logger, "No se pudo mover el cursor");
	else log_info(logger, "Cursor movido");
}

/*
 * RESERVAR MEMORIA
 *
 * Informa al kernel que reserve en el Heap una cantidad de memoria
 * acorde al espacio recibido como parametro.
 *
 * @sintax	TEXT_MALLOC (alocar)
 * @param	valor_variable Cantidad de espacio
 * @return	puntero a donde esta reservada la memoria
 */
t_puntero reservar(t_valor_variable espacio){
	log_debug(logger, "ANSISOP_reservar -> espacio: %d", espacio);
	header_t header;
	char* paquete;
	size_t size = sizeof(t_valor_variable);
	t_valor_variable valor;

	header.type = RESERVAR_MEMORIA;
	header.length = sizeof(t_valor_variable)*3;
	paquete = malloc(header.length);
	memcpy(paquete, &pcb->pid, sizeof(t_valor_variable));
	memcpy(paquete+size, &pcb->cantPaginasCodigo, sizeof(t_valor_variable));size += size;
	memcpy(paquete+size, &espacio, sizeof(t_valor_variable));

	if(sendSocket(socketConexionKernel,&header,paquete) <= 0){
		log_error(logger, "problemas de conexion");
		finalizarCPU();
	}
	free(paquete);
	if(requestHandlerKernel() == -1){
		log_error(logger, "No se pudo realizar la reserva. Se finaliza el proceso");
		return -1;
	}
	valor = *(t_valor_variable*)paqueteGlobal;
	log_info(logger, "Reserva exitosa. Posicion %d", valor);
	free(paqueteGlobal);
	return valor;
}

/*
 * LIBERAR MEMORIA
 *
 * Informa al kernel que libera la memoria previamente reservada con RESERVAR.
 * Solo se podra liberar memoria previamente asignada con RESERVAR.
 *
 * @sintax	TEXT_FREE (liberar)
 * @param	puntero Inicio de espacio de memoria a liberar (previamente retornado por RESERVAR)
 * @return	void
 */
void liberarMemoria(t_puntero posicion){
	log_debug(logger, "ANSISOP_liberarMemoria -> posicion: %d", posicion);
	header_t header;
	header.type= LIBERAR_MEMORIA;
	header.length = sizeof(t_liberar);
	t_liberar pedido;
	pedido.pid = pcb->pid;
	pedido.pos = posicion;
	if(sendSocket(socketConexionKernel, &header, &pedido) <= 0 ){
		log_error(logger,"Error al soliciar liberar memoria. Desconexion...");
		finalizarCPU();
	}
	if(requestHandlerKernel() == -1) log_error(logger, "Error al liberar");
	else log_info(logger, "Memoria liberada");

}

/*
 * SIGNAL
 *
 * Informa al kernel que ejecute la función signal para el semáforo con el nombre identificador_semaforo.
 * El kernel deberá decidir si desbloquear otros procesos o no.
 *
 * @sintax	TEXT_SIGNAL (signal)
 * @param	identificador_semaforo	Semaforo a aplicar SIGNAL
 * @return	void
 */
void signalAnsisop(t_nombre_semaforo identificador_semaforo){
	log_debug(logger, "signal a semaforo '%s'", identificador_semaforo);
	header_t header;
	header.type=SEM_SIGNAL;
	header.length=strlen(identificador_semaforo)+1;
	sendSocket(socketConexionKernel,&header,identificador_semaforo);
	requestHandlerKernel(); // PARA QUE ME DEVUELVA SIGNAL OK
}

/*
 * WAIT
 * Informa al kernel que ejecute la función wait para el semáforo con el nombre identificador_semaforo.
 * El kernel deberá decidir si bloquearlo o no.
 *
 * @sintax	TEXT_WAIT (wait)
 * @param	identificador_semaforo	Semaforo a aplicar WAIT
 * @return	void
 */
void wait(t_nombre_semaforo identificador_semaforo){
	log_debug(logger, "wait a semaforo '%s'", identificador_semaforo);
	header_t header;
	header.type=SEM_WAIT;
	header.length = strlen(identificador_semaforo)+1;
	sendSocket(socketConexionKernel,&header,identificador_semaforo);
	requestHandlerKernel();
}
