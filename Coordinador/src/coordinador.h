/*
 * coordinador.h
 *
 *  Created on: 21 abr. 2018
 *      Author: utnso
 */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <sockets.h>
#include <commons/config.h>
#include <lib.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <protocolos.h>
#include <sys/select.h>

#define programa "COORDINADOR"

typedef struct{
	int id;
	int socket;
} t_esi;

typedef struct{
	char* clave;
	char* valor;
	char* id_instancia;
} t_clave;

typedef struct{
	char letra_inicio;
	char letra_fin;
	char* id_instancia;
	int espacio_disponible;
	short int estado;
	int socket;
	int conectado;
} t_instancia;

typedef enum {
	CLAVE_EXISTENTE,
	CLAVE_GENERADA,
	ERROR_CLAVE,
}t_estado_clave;

typedef enum {
	GUARDAR_EN_MEMORIA,
	GUARDAR_EN_LISTA
}t_tipo_guardado;

t_list* lista_instancias;
t_list* lista_claves;
t_list* lista_esis;
t_config* config;
int PUERTO_COORD;
int CANTIDAD_ENTRADAS;
int TAMANO_ENTRADAS;
int RETARDO;
int socket_planificador;
int fd_servidor;
int s_servidor;
int cantidad_de_letras = 27;
char* ALG_DISTRIBUCION;
char letras[27] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','ñ','o','p','q','r','s','t','u','v','w','x','y','z'};

//-------logger------------

char* ruta_log = "logger.log";
char* ruta_log_operaciones = "operaciones.log";
t_log_level level=LOG_LEVEL_INFO;
t_log* logger;
t_log* logger_operaciones;
int contador_esis = 0;
int contador_instancias = 0;
fd_set fds_esis;

void levantar_logger();
void loggear_op_get(char* id_esi,char* clave);
void loggear_op_set(char* id_esi, char* clave, char* valor);
void loggear_op_store(char* id_esi, char* clave);

//--------server------------
t_list* sockets_clientes;
void atender_nueva_conexion();
void iniciar_servidor();
int get_esi_from_socket(int socket, t_esi** esi_encontrado);
int get_instancia_from_socket(int socket, t_instancia** instancia_encontrada);

void crear_esi(int socket);
void crear_planificador(int socket);
void crear_instancia(int socket);

void atender_planificador(int protocolo);
void atender_esi();
void atender_instancia();

pthread_t* t_planificador;
pthread_t* t_instancias;
pthread_t t_espera_conexiones;

//iniciar coordinador
void iniciar_coordinador();
	//-------config------------
	char* ruta_config;// = "/home/utnso/Escritorio/tp-2018-1c-La-Cuarta-Es-La-Vencida/Coordinador/resources/config.cfg";
	void parsear_config();

//-----exit---------
void exit_with_error(char* error_msg);
void exit_gracefully(int return_nr);

//-----algoritmos de guardado-----
t_clave* crear_clave(t_instancia* instancia, char* ingresar_nueva_clave, char* ingresar_valor_clave);
char* agregar_clave_a_instancia(t_instancia* instancia, char* ingresar_nueva_clave, char* ingresar_valor_clave);
	//Equitative_load
int guardar_clave_equitative_load (char * ingresar_nueva_clave, char* ingresar_nuevo_valor,t_tipo_guardado tipo);
	//LSU
int guardar_clave_lsu(char* ingresar_nueva_clave, char* ingresar_nuevo_valor,t_tipo_guardado tipo);
int ordenar_instancia_por_espacio(t_instancia* instancia1, t_instancia* instancia2);
	//Key Explicit
int guardar_clave_key_explicit(char* ingresar_nueva_clave, char* ingresar_nuevo_valor,t_tipo_guardado tipo);

//-----operaciones---------
int enviar_set_a_instancia(char* ingresar_nueva_clave, char* ingresar_nuevo_valor, int socket, char* id_instancia_agregado);
t_clave* buscar_clave(char* clave_a_guardar);
t_instancia* buscar_instancia(t_clave* clave_encontrada);
void definir_operacion(char* operacion_leida);
void compractar_instancias(int pos);
void asignar_letras_a_almacenar();
void reasignar_letras_a_almacenar();
int buscar_posicion_instancia(char* id_instancia);
int ejecutar_get(char* clave);
int existe_clave_guardada(char* clave);
int ejecutar_set(char* ingresar_nueva_clave, char* ingresar_nuevo_valor,t_tipo_guardado tipo);
int ejecutar_storage(char* clave_a_guardar);
int buscar_posicion_clave(char* clave_recibida);

//------select----------

void iniciar_select();
void construir_fds(int* max_actual);
void leer_cambios_select();
void atender_esi(t_esi* esi, int accion);
void recibir_data_de_cliente(int socket);
void finalizar_esi();
void finalizar_instancias();
void dejar_de_escuchar_socket(int socket);
int get_socket_posicion(int posicion);
int get_socket_esi_en_posicion(int posicion);
int definir_deconexion(int socket);
int buscar_socket_instancias(int socket);
int contar_instancias_conectadas();

//-----funciones de estructura----
void liberar_estructuras ();
void liberar_claves ();
void liberar_clave (t_clave* registro_clave);
void liberar_instancias ();
void liberar_instancia (t_instancia* instancia);
int calcular_entradas_clave(char* ingresar_nueva_clave);
void liberar_esis ();
t_instancia* buscar_instancia_socket(int socket);

//GENERICO
int cantidadElementos(char ** array);
int keep_alive(int socket);

//--------variables mock--------
void iniciar_mock();



#endif /* COORDINADOR_H_ */
