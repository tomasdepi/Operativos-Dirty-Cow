/*
 * funcionesFs.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef FUNCIONESFS_H_
#define FUNCIONESFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/sockets.h>
#include <commons/log.h>
#include <commons/bitarray.h>

#define configuracionFS "../confFileSystem.init"
#define MAX_LEN_PUERTO 6
#define MAX_LEN_IP 20
#define IP "192.168.3.3"
#define BACKLOG 10
#define METADATA_PATH "Metadata"
#define METADATA_ARCHIVO "/Metadata.bin"
#define BITMAP_ARCHIVO "/Bitmap.bin"
#define ARCHIVOS_PATH "Archivos"
#define BLOQUES_PATH "Bloques"
#define SIN_BLOQUES_LIBRES -1

//TADS
typedef struct{
	char* puertoEscucha;
	char* punto_montaje;
	int tamanio_bloque;
	int cantidad_bloques;
	char* ip_fs;
}t_config_FS;

typedef struct
{
	char* path;
	int offset;
	int size;
} pedido_obtener_datos;

typedef struct
{
	char* path;
	int offset;
	int size;
	char* buffer;
}pedido_guardar_datos;

//Prototipos
t_config_FS* levantarConfiguracion(char* archivo);
void destruirConfiguracionFS(t_config_FS* conf);
void esperarConexionKernel();
void crearConfig(int argc, char* argv[]);

void inicializarMetadata();
void procesarMensajesKernel();
void mkdirRecursivo(char* path);
int buscarBloqueLibre();
char** obtenerNumeroBloques(char* path);
int obtenerNumBloque(char* path, int offset);
pedido_obtener_datos* deserializar_pedido_obtener_datos(char* paquete);
pedido_guardar_datos* deserializar_pedido_guardar_datos(char* paquete);
char* generarPathBloque(int num_bloque);
char* generarPathArchivo(char* path);
void escribirValorBitarray(bool valor, int pos);
int reservarNuevoBloque(char* pathArchivo);
int cantidadBloques(char** bloques);
void escribirEnArchivo(int bloque, char* buffer, int size, int offset);
void leerArchivo(int bloque, char* buffer, int size, int offset);
int string_pos_char(char* string, char caracter);
void aumentarTamanioArchivo(pedido_guardar_datos* pedido, char* path);

//Operaciones
void validarArchivo(char* path);
void crearArchivo(void* package);
void borrarArchivo(void* package);
void guardarDatos(void* package);
void obtenerDatos(void* package);

//Variables Globales
t_config_FS* conf;
int socketEscucha;
int socketConexionKernel;
t_log* logger;
t_bitarray* bitarray;
char* pathBloques, *pathArchivos, *pathMetadata, *pathMetadataArchivo, *pathMetadataBitarray;

#endif /* FUNCIONESFS_H_ */
