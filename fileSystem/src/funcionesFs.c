#include "funcionesFs.h"

void crearConfig(int argc, char* argv[]){
	if(argc>1){
		if(verificarExistenciaDeArchivo(argv[1])){
			conf=levantarConfiguracion(argv[1]);
			log_info(logger, "Configuracion levantada correctamente");
		}else{
			log_error(logger,"La ruta especificada es incorrecta");
			exit(EXIT_FAILURE);
		}
	}
	else if(verificarExistenciaDeArchivo(configuracionFS)){
		conf=levantarConfiguracion(configuracionFS);
		log_info(logger,"Configuracion levantada correctamente");
	}
	else if(verificarExistenciaDeArchivo(string_substring_from(configuracionFS,3))){
		conf=levantarConfiguracion(string_substring_from(configuracionFS,3));
		log_info(logger,"Configuracion levantada correctamente");
	}
	else{
		log_error(logger,"No pudo levantarse el archivo de configuracion");
		exit(EXIT_FAILURE);
	}
}

t_config_FS* levantarConfiguracion(char* archivo){

	t_config_FS* conf = malloc(sizeof(t_config_FS));

	t_config* configFS = config_create(archivo);

	conf->puertoEscucha = malloc(strlen(config_get_string_value(configFS, "PUERTO"))+1);
	strcpy(conf->puertoEscucha, config_get_string_value(configFS, "PUERTO"));

	conf->ip_fs = malloc(MAX_LEN_IP);
	strcpy(conf->ip_fs, config_get_string_value(configFS, "IP_FS"));

	conf->punto_montaje = malloc(strlen(config_get_string_value(configFS, "PUNTO_MONTAJE"))+1);
	strcpy(conf->punto_montaje, config_get_string_value(configFS, "PUNTO_MONTAJE"));
	if(!string_ends_with(conf->punto_montaje, "/") ) string_append(&conf->punto_montaje,"/");

	conf->tamanio_bloque = config_get_int_value(configFS, "TAMANIO_BLOQUE");

	conf->cantidad_bloques = config_get_int_value(configFS, "CANTIDAD_BLOQUES");

	config_destroy(configFS);

	return conf;
}

void destruirConfiguracionFS(t_config_FS* conf){
	free(conf->puertoEscucha);
	free(conf->punto_montaje);
	free(conf);
}

void procesarMensajesKernel(){
	int tipo_mensaje; //Para que la funcion recibir_string lo reciba
	void* paquete;
	int check;

	while(1){

		check = recibir_paquete(socketConexionKernel, &paquete, &tipo_mensaje);

		if (check <= 0) {
			log_error(logger, "El kernel se desconecto");
			close(socketConexionKernel);
			exit(1);
		}
		switch (tipo_mensaje) {
			case VALIDAR_ARCHIVO:
				validarArchivo(paquete);
				break;
			case CREAR_ARCHIVO:
				crearArchivo(paquete);
				break;
			case BORRAR_ARCHIVO:
				borrarArchivo(paquete);
				break;
			case GUARDAR_DATOS:
				guardarDatos(paquete);
				break;
			case OBTENER_DATOS:
				obtenerDatos(paquete);
				break;
			default:
				log_warning(logger, "Se recibio una operacion invalida: %d", tipo_mensaje);
				break;
		}

	}

}

void validarArchivo(char* path){

	char* pathAbsoluto = generarPathArchivo(path);

	bool existe = verificarExistenciaDeArchivo(pathAbsoluto);

	free(pathAbsoluto);

	if(existe)
		enviar_paquete_vacio(ARCHIVO_EXISTE, socketConexionKernel);
	else
		enviar_paquete_vacio(ARCHIVO_INEXISTENTE, socketConexionKernel);

	return;
}

void crearArchivo(void* package){

	char* pathArchivo = generarPathArchivo(package);

	log_debug(logger, "creando archivo : %s", pathArchivo);

	if(!verificarExistenciaDeArchivo(pathArchivo)){

		int bloqueLibre = buscarBloqueLibre();

		if(bloqueLibre == SIN_BLOQUES_LIBRES){
			//aviso a kernel que no hay bloques libres
			enviar_paquete_vacio(SIN_ESPACIO_FS, socketConexionKernel);

			return;
		}

		escribirValorBitarray(1, bloqueLibre); //pongo el bloque como ocupado

		char* subCarpetas = string_substring_until(pathArchivo, string_pos_char(pathArchivo, '/'));
		log_debug(logger, "subcarpetas: %s", subCarpetas);
		mkdirRecursivo(subCarpetas);
		free(subCarpetas);

		FILE* archivo = fopen(pathArchivo, "a");

		fprintf(archivo, "TAMANIO=0\n");
		fprintf(archivo, "BLOQUES=[%d]\n", bloqueLibre);

		fclose(archivo);

		log_info(logger, "Se creo el archivo %s\n", pathArchivo);
	}
	free(pathArchivo);
	enviar_paquete_vacio(ABRIR_ARCHIVO_OK, socketConexionKernel);
}

void borrarArchivo(void* package){

	char* path_archivo = generarPathArchivo(package);

	if(!verificarExistenciaDeArchivo(path_archivo)){
		enviar_paquete_vacio(ARCHIVO_INEXISTENTE, socketConexionKernel);
		log_error(logger, "Se intento borrar un archivo inexistente");
	}else{

		t_config* data = config_create(path_archivo);
		char** bloques = config_get_array_value(data, "BLOQUES");

		int j = 0;
		while(bloques[j] != NULL){
			printf("bloque a liberar: %d\n", atoi(bloques[j]));
			escribirValorBitarray(0, atoi(bloques[j])); //libero los bloques
			j++;
		}

		//borro el archivo
		unlink(path_archivo);
		log_debug(logger, "Archivo borrado con exito");
		enviar_paquete_vacio(BORRAR_ARCHIVO_OK, socketConexionKernel);

		config_destroy(data);
		free(path_archivo);
	}
}

void guardarDatos(void* package){
	int cursor = *(int*) package;
	int sizeEscritura = *(int*) (package + sizeof(int));
	int sizePath = *(int*) (package + sizeof(int) * 2);
	char* pathAux = package + sizeof(int) * 3;
	char* escritura = package + sizeof(int) * 3 + sizePath;
	pedido_guardar_datos* pedido = malloc(sizeof(int) * 3 + sizeEscritura + sizePath); //=deserializar_pedido_guardar_datos(package);
	pedido->offset = cursor;
	pedido->buffer = escritura;
	pedido->path = pathAux;
	pedido->size = sizeEscritura;
	char* path = generarPathArchivo(pedido->path);

	if(!verificarExistenciaDeArchivo(path)){
		enviar_paquete_vacio(ARCHIVO_INEXISTENTE, socketConexionKernel);
		return;
	}

	t_config* c = config_create(path);

	char** bloques = config_get_array_value(c, "BLOQUES");
	int cantBloques = cantidadBloques(bloques);

	config_destroy(c);

	int offsetBloque;
	int restoBloque, bloque;

	offsetBloque = (pedido->offset % conf->tamanio_bloque);
	int numBloque = (pedido->offset / conf->tamanio_bloque);
	int j = numBloque, bytesEscritos = 0;

	bloque = atoi(bloques[numBloque]);

	while(bytesEscritos < pedido->size){
		restoBloque = pedido->size - bytesEscritos;

		if(restoBloque > conf->tamanio_bloque)
			restoBloque = conf->tamanio_bloque;

		log_info(logger, "Accedo al bloque %d", bloque);

		escribirEnArchivo(bloque, pedido->buffer+bytesEscritos, restoBloque, offsetBloque);

		bytesEscritos += restoBloque;

		offsetBloque = 0;

		if(bytesEscritos < pedido->size){//aun faltan cosas por escribir
			if(numBloque+1 == cantBloques){
				log_info(logger, "reservo nuevo bloque");
				bloque = reservarNuevoBloque(path);
				if(bloque == SIN_BLOQUES_LIBRES){
					enviar_paquete_vacio(SIN_ESPACIO_FS, socketConexionKernel);
					return;
				}
			}else{
				j++;
				bloque = atoi(bloques[numBloque + j]);
			}
		}

	}

	aumentarTamanioArchivo(pedido, path);

	//free(pedido->path);
	//free(pedido->buffer);
	free(pedido);
	free(path);

	enviar_paquete_vacio(ESCRITURA_OK, socketConexionKernel);

}

void escribirEnArchivo(int bloque, char* buffer, int size, int offset){
	FILE* archivo = fopen(generarPathBloque(bloque), "r+");

	fseek(archivo, offset, SEEK_SET);

	fwrite(buffer, size, 1, archivo);

	fclose(archivo);
}

void obtenerDatos(void* package){
	pedido_obtener_datos* pedido = deserializar_pedido_obtener_datos(package);

	char* path = generarPathArchivo(pedido->path);

	if(!verificarExistenciaDeArchivo(path)){
		enviar_paquete_vacio(ARCHIVO_INEXISTENTE, socketConexionKernel);
		return;
	}

	char* buffer = malloc(sizeof(pedido->size));

	t_config* c = config_create(path);
	char** bloques = config_get_array_value(c, "BLOQUES");

	int offsetBloque, bytesLeidos = 0, restoBloque;

	offsetBloque = (pedido->offset % conf->tamanio_bloque);
	int numBloque = (pedido->offset / conf->tamanio_bloque);
	int j = numBloque;

	int bloque = atoi(bloques[numBloque]);

	while(bytesLeidos != pedido->size){
		restoBloque = pedido->size - bytesLeidos;

		if(restoBloque > conf->tamanio_bloque)
			restoBloque = conf->tamanio_bloque;

		log_info(logger, "accedo al bloque %d", bloque);

		leerArchivo(bloque, buffer+bytesLeidos, restoBloque, offsetBloque);
		bytesLeidos += restoBloque;

		j++;
		if(bloques[j] != NULL)
			bloque = atoi(bloques[j]);

		offsetBloque = 0;
	}

	header_t header;
	header.length = pedido->size;
	header.type = LECTURA_OK;
	sendSocket(socketConexionKernel, &header, buffer);

	free(pedido->path);
	free(pedido);

	config_destroy(c);
	free(path);
}

void leerArchivo(int bloque, char* buffer, int size, int offset){
	FILE* archivo = fopen(generarPathBloque(bloque), "r+");

	fseek(archivo, offset, SEEK_SET);

	fread(buffer, size, 1, archivo);

	fclose(archivo);
}

void mkdirRecursivo(char* path){

	char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",path);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
        if(*p == '/') {
        	*p = 0;
            mkdir(tmp, 0777);
            *p = '/';
        }
    mkdir(tmp, 0777);
}

int buscarBloqueLibre(){

	int bloqueLibre;

	for(bloqueLibre=0; bitarray_test_bit(bitarray, bloqueLibre) && bloqueLibre<conf->cantidad_bloques; bloqueLibre++){

	}

	if(bloqueLibre >= conf->cantidad_bloques)
		return SIN_BLOQUES_LIBRES;

	return bloqueLibre;
}

void escribirValorBitarray(bool valor, int pos){

	if(valor)
		bitarray_set_bit(bitarray, pos);
	else
		bitarray_clean_bit(bitarray, pos);

	FILE* bitmap = fopen(pathMetadataBitarray, "w");
	fwrite(bitarray->bitarray, bitarray->size, 1, bitmap);
	fclose(bitmap);

}

char** obtenerNumeroBloques(char* path){
	t_config* c = config_create(path);
	char** bloques = config_get_array_value(c, "BLOQUES");
	config_destroy(c);

	return bloques;
}

int obtenerNumBloque(char* path, int offset){
	char** bloques = obtenerNumeroBloques(generarPathArchivo(path));

	int numBloque = (offset / conf->tamanio_bloque);
	return atoi(bloques[numBloque]);
}

int reservarNuevoBloque(char* pathArchivo){

	int bloqueLibre = buscarBloqueLibre();

	if(bloqueLibre == SIN_BLOQUES_LIBRES)
		return SIN_BLOQUES_LIBRES;

	escribirValorBitarray(1, bloqueLibre);

	t_config* c = config_create(pathArchivo);
	char* bloques = string_new();
	string_append(&bloques, config_get_string_value(c, "BLOQUES"));

	bloques[strlen(bloques)-1] = '\0';
	string_append(&bloques, ",");
	string_append(&bloques, string_itoa(bloqueLibre));
	string_append(&bloques, "]");
	config_set_value(c, "BLOQUES", bloques);
	config_save(c);
	config_destroy(c);
	free(bloques);

	return bloqueLibre;
}

int cantidadBloques(char** bloques){
	int j=0;

	while(bloques[j] != NULL)
		j++;

	return j;
}

pedido_obtener_datos* deserializar_pedido_obtener_datos(char* paquete){

	pedido_obtener_datos* pedido = malloc(sizeof(pedido_obtener_datos));
	memcpy(&(pedido->offset), paquete, sizeof(int));
	memcpy(&(pedido->size), paquete+sizeof(int), sizeof(int));
	int len_path;
	memcpy(&len_path, paquete+2*sizeof(int), sizeof(int));
	pedido->path = malloc(len_path+1);
	memcpy(pedido->path, paquete+3*sizeof(int), len_path);

	return pedido;
}

pedido_guardar_datos* deserializar_pedido_guardar_datos(char* paquete){
	int offset = 0;

	pedido_guardar_datos* pedido = malloc(sizeof(pedido_guardar_datos));
	memcpy(&(pedido->offset), paquete, sizeof(int));
	offset += sizeof(int);
	memcpy(&(pedido->size), paquete+offset, sizeof(int));
	offset += sizeof(int);
	int len_path;
	memcpy(&len_path, paquete+offset, sizeof(int));
	offset += sizeof(int);
	pedido->path = malloc(len_path+1);
	memcpy(pedido->path, paquete+offset, len_path);
	pedido->path[len_path] = '\0';
	offset += len_path;

	int len_buffer;
	memcpy(&len_buffer, paquete+offset, sizeof(int));
	offset += sizeof(int);
	pedido->buffer = malloc(len_buffer+1);
	memcpy(pedido->buffer, paquete+offset, len_buffer);
	pedido->buffer[len_buffer] = '\0';

	return pedido;
}

char* generarPathBloque(int num_bloque){
	char* path_bloque = string_new();
	string_append(&path_bloque, conf->punto_montaje);
	string_append(&path_bloque, "Bloques/");
	string_append(&path_bloque, string_itoa(num_bloque));
	string_append(&path_bloque, ".bin");

	return path_bloque;
}

char* generarPathArchivo(char* path){
	char* pathArchivo=string_new();
	string_append(&pathArchivo, conf->punto_montaje);
	string_append(&pathArchivo, "Archivos");

	if(!string_starts_with(path, "/")) string_append(&pathArchivo, "/");

	string_append(&pathArchivo, path);

	return pathArchivo;
}

int string_pos_char(char* string, char caracter){
	int len = strlen(string), j;

	for(j=0; *(string+len-j) != caracter; j++){
	}

	return len-j;
}

void aumentarTamanioArchivo(pedido_guardar_datos* pedido, char* path){
	t_config* c = config_create(path);
 	int tamanio = config_get_int_value(c, "TAMANIO");

 	int bytesEscritos = pedido->offset + pedido->size - tamanio;
 	tamanio += bytesEscritos;
 	if(bytesEscritos > 0){
 		config_set_value(c, "TAMANIO", string_itoa(tamanio));
 		config_save(c);
 	}

 	config_destroy(c);
 }

