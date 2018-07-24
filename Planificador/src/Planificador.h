#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#define programa "PLANIFICADOR"


#include <lib.h>
#include <sockets.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "Shared.h"
#include "Consola.h"
#include "ManejoColas.h"
#include "protocolos.h"

int s_coordinador;
t_esi* planificar_sjf();
t_esi* planificar(int algoritmo);
t_esi* planificar_hrrn();
//-------Estructuras------------

t_list* sockets_clientes;
pthread_t t_espera_conexiones;
pthread_t* t_planificador;

int s_servidor;

bool resultado_ultima_operacion;

void levantar_logger();
void liberar_lista_recursos();
int definir_algoritmo();
int cantidad_Elementos(char ** array);

//-------config------------

char* ruta_config;// = "/home/utnso/Escritorio/tp-2018-1c-La-Cuarta-Es-La-Vencida/Planificador/resources/config.cfg";

void parsear_config();
void levantar_logger_consola();

//-------logger------------

char* ruta_log = "logger.log";
char* ruta_log_consola = "loggerConsola.log";
t_log_level level=LOG_LEVEL_INFO;

//--------server------------
int fd_servidor;
void levantar_server();
void crear_servidor();

//------select----------
int contador_clientes = 0;
fd_set fds_cliente;
t_list* sockets_clientes;

void select_clientes();

//-----exit---------
void exit_with_error(char* error_msg);
void exit_gracefully(int return_nr);

//-----funciones----
void crear_flags_para_consola();
void crear_colas();
void conectarse_coord();
void crear_servidor();;
void construir_fds(int* max_actual);
void atender_nueva_conexion();
void recibir_data_de_cliente(int socket);
void atender_esi(int protocolo, int socket);
t_esi* planificar(int algoritmo);
void notificar_planificador_resultado_operacion(bool resultado);
void enviar_mensaje_a_esi(t_esi* esi);
void atender_coordinador(int protocolo);
void enviar_mensaje_a_coordinador(bool respuesta);
void enviar_respuesta_store(t_proto proto_respuesta);
void leer_cambios_select();
int get_socket_posicion(int posicion);



#endif /* PLANIFICADOR_H_ */


