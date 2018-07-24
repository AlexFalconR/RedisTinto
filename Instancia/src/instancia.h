/*
 * instancia.h
 *
 *  Created on: 21 abr. 2018
 *      Author: utnso
 */

#ifndef INSTANCIA_H_
#define INSTANCIA_H

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sockets.h>
#include <pthread.h>
#include <protocolos.h>
#include <commons/config.h>
#include <commons/string.h>
#include <lib.h>
#include <commons/collections/list.h>
#include <sys/stat.h>

#define programa "INSTANCIA"

struct stat directorio;
t_dictionary *tabla_entradas;
char* IP_COORDINADOR;
char* ALG_REEMPLAZO;
char* PUNTO_MONTAJE;
char* NOMBRE;
char** array_entradas;
int* array_lru;
char* entradas;
int PUERTO_COORDINADOR;
int INTERVALO;
int s_coordinador;
int cant_entradas;
int tam_entradas;
int circular=0;
int algoritmo=0;
pthread_t t_dump;
pthread_mutex_t mutex_memoria;

typedef struct entrada {
	int numero_entrada;
	int longitud;
	int uso;
}t_entrada;

//-------config------------

char* ruta_config;// = "/home/utnso/Escritorio/tp-2018-1c-La-Cuarta-Es-La-Vencida/Instancia/resources/config.cfg";
t_config* config;
void parsear_config();

//-------logger------------

char* ruta_log = "/home/utnso/Escritorio/tp-2018-1c-La-Cuarta-Es-La-Vencida/Instancia/resources/logger.log";
t_log_level level=LOG_LEVEL_INFO;
t_log* logger;
void levantar_logger();

//-----exit---------
void exit_with_error(char* error_msg);
void exit_gracefully(int return_nr);
int set_key_value(char* ket, char* value);
static t_entrada *entrada_create(int numero, int longitud,int lru);
void store_key(char* key);
int re_asignar_entrada(int largo_nuevo,int entrada, int largo,char* key);
int seleccionar_entrada(int largo);
int cantidad_entradas(int largo);
int str_array_size(char ** array);
void str_array_print(char ** array,int);
void escribir(char* mensaje,int offset);
void escribir_entrada(char* mensaje, int entrada);
char* leer_entrada(char* key);
int longitud_clave(char* key);
int primer_entrada_clave(char* key);
void atender_coord();
static void entrada_destroy(t_entrada *self);
void conectarse_coord();
void mostrar_elem(char* clave, t_entrada* entrada);
void mostrar_dic();
void borrar_clave_array(char* clave);
void borrar_clave(char* clave);
int buscar_espacio_vacio(int largo);
int buscar_espacio_vacio_contiguo(int largo);
void circular_siguiente();
void compactacion();
char* seleccionar_clave_reemplazar();
int es_atomica(char* clave);
char* leer_archivo(char* nombre);
void sumar_uno_lru(char* clave, t_entrada* entrada);
void actualizar_lru(char* clave);
t_list* get_entrada_mayor_lru(t_dictionary *self);
t_list* get_entrada_mayor_longitud(t_dictionary *self);
int cantidadElementos(char ** array);
void liberar_recursos();
void liberar_array();
void liberar_diccionario();
void liberar_lista(t_list* lista_claves_reemplazar);
bool entrada_esta_vacia(char* entrada);
void mostrar_bloques_clave(char* clave, t_entrada* entrada);
void dump();
void compactar_elemento_array(int indice_elemento,char** array);

#endif /* INSTANCIA_H_ */
